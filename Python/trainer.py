import time
import threading
import os
from os import fdopen
import sys
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout, LSTM
from tensorflow.keras.utils import plot_model
import tensorflow as tf
from tensorflow.keras import backend as kerasBE
from collections import deque
#TESTING
#import zc.lockfile as lf
import numpy as np

import gspread
from oauth2client.service_account import ServiceAccountCredentials
from time import sleep
import random
"""
Author: Chase M. Craig
Purpose: For being able to accept input from an application to train a neural network.

Using https://towardsdatascience.com/reinforcement-learning-w-keras-openai-dqns-1eed3a5338c 

as a template

used 
https://michaelblogscode.wordpress.com/2017/10/10/reducing-and-profiling-gpu-memory-usage-in-keras-with-tensorflow-backend/

as a GPU limiting witchcraft helper.

used
https://www.twilio.com/blog/2017/02/an-easy-way-to-read-and-write-to-a-google-spreadsheet-in-python.html

to use google sheets to collect information about training
"""
STARTING_TIME = time.time()

# Make sure the pipes are open!!!
# KEEP AT TOP
stdoin = open(0, "r")
stdout = open(1, "w")
stderr = open(2, "w")

"""
Argument Updates by Nara
1:parentFile - if 0, spawn new model
2:childFile - if 0, load in predict mode
3:GPU_Partition
"""
parentFile = sys.argv[1]
childFile = sys.argv[2]
GPU_Partition = sys.argv[3]

# This will properly fall through and load the mode we desire!
modelMode = 0
if len(parentFile) == 0:
	modelMode += 1 # new Model
if len(childFile) == 0:
	modelMode += 2 # Prediction

"""
GLOBALS
"""
INPUT_RATE = 4 # This means that if we get input at 60 TPS, we only make predictions for every xth item. 
#FILENAME = sys.argv[1] # To be read latter
SUICIDE_REWARD = -50 # Negative is bad
KILL_REWARD = 1 # Reward score for calculating instant scores (should be less than suicide to prevent suiciding for score)
KILL_SCORE = 50 # Actual score for calculating end of round score
MAX_FRAMES_RECORDING = 120//INPUT_RATE # 120 = 2 seconds saved for inputs if 60 TPS (/divided by input dropout)
MAX_MEMORY_FRAMES = 120//INPUT_RATE # 60 = 1 second for every batch training if 60 TPS (/divided by input dropout)
MAX_BATCH_SIZE = 80//INPUT_RATE # Drop half of the examples from memory frames and only train on x of them...
MY_HP_PENALTY = 0.1
THEIR_HP_REWARD = 10
# 0:ai.health, 1:ai.dir, 2:ai.pos_x, 3:ai.pos_y, 4:ai.action, 5:ai.action_frame,
# 6:enemy.health, 7:enemy.dir, 8:enemy.pos_x, 9:enemy.pos_y, 10:enemy.action, 11:enemy.action_frame
INPUT_SIZE = 12
MY_HP_IDX = 0
MY_X = 2
MY_Y = 3
ENEMY_HP_IDX = 6
ENEMY_X = 8
ENEMY_Y = 9


POSSIBLE_ACTIONS = [[0 for i in range(INPUT_SIZE)]]*(5*3*6)
c = 0
# 5 stick.x positions
for p in [-1,-.5,0,.5,1]:
	# 3 stick.y positions
	for u in [-1,0,1]:
		# 6 possible buttons states, all off or only 1 on
		for ab in [0, 1, 2, 3, 4, 5]:
			POSSIBLE_ACTIONS[c] = [p, u,1 if ab ==1 else 0,1 if ab ==2 else 0,1 if ab ==3 else 0,1 if ab ==4 else 0,1 if ab ==5 else 0]
			c = c + 1
MODEL_PARAMETERS = [(MAX_FRAMES_RECORDING,INPUT_SIZE), 30, 30, 15, len(POSSIBLE_ACTIONS)]
MODEL_DROPOUT = [0.2, 0.5, 0.5]


# When writing to the pipe, we're using 
# '\0' + ("pred: " + <str>msg) + '\0'
# as the identifier for the pipe parser. by using "pred: " at the beginning we 
# can guarantee that we know what to look for

stderr.write('\0' + "GO GO TENSORFLOW!" + '\0')
stderr.flush()

def debugPrint(msg):
	stdout.write('\0' + ("pred: " + msg) + '\0')
	stdout.flush()
	stderr.flush()

def PipePrint(*vars):
	vLen = len(vars)
	if vLen > 0:
		msg = str(vars[0])
		for i in range(1, vLen):
			msg += " %s" % str(vars[i])
		stdout.write('\0' + ("pred: " + msg) + '\0')
		stdout.flush()
		stderr.flush()

def getInput(n):
	stderr.flush()
	return os.read(0, n).decode("utf-8")

choice = getInput(1)
if "0" in choice:
	# SHUT UP!
	debugPrint = lambda o: None # :D

fallbackLSTM = LSTM
try:
	from tensorflow.keras.layers import CuDNNLSTM
	fallbackLSTM = lambda *args, **kw: CuDNNLSTM(*args, **{k:kw[k] for k in kw if k != "activation"})
	debugPrint("Using CuDNNLSTM for faster evaluation of LSTMs.\n")
except:
	debugPrint("Unable to import CuDNNLSTM, using fallback of LSTM with tanh activator.\n")
	
debugPrint("Initialization: (0) Load Model (1) New Model (2) Load in Prediction-Only (3) New in Prediction-Only\n")
stderr.flush()

class DQN:
	def __init__(self, GPUNUM):
		self.configStuff = tf.ConfigProto()
		self.configStuff.gpu_options.allow_growth = True
		self.configStuff.gpu_options.per_process_gpu_memory_fraction = 1.0 / GPUNUM
		kerasBE.set_session(tf.Session(config=self.configStuff))
		self.memory  = deque(maxlen=MAX_MEMORY_FRAMES*2)
		self.game_score = 0
		self.gamma = 0.95
		self.epsilon = 1.0
		self.epsilon_min = 0.01
		self.epsilon_decay = float(sys.argv[4]) # takes a LONG time to train...
		self.learning_rate = 0.01
		self.input_size = INPUT_SIZE
		self.tau = .05
		self.actions = POSSIBLE_ACTIONS
		self.model = self.create_model()
		# "hack" implemented by DeepMind to improve convergence
		self.target_model = self.create_model()
		self.my_dmg = 0
		self.my_hp = 0
		self.my_de = 0
		self.my_kill = 0
		
	def get_Score(self, prev_state, new_state):
		reward = 0
		prev_state = prev_state[-1]
		if prev_state[ENEMY_HP_IDX] > new_state[ENEMY_HP_IDX] and new_state[ENEMY_HP_IDX] == 0:
			reward = reward + KILL_REWARD # :D Not enough to overcome suicide.
		if prev_state[MY_HP_IDX] > new_state[MY_HP_IDX] and new_state[MY_HP_IDX] == 0:
			reward = reward + SUICIDE_REWARD # Died, get a reward of -500
		else:
			reward = reward - (new_state[MY_HP_IDX] - prev_state[MY_HP_IDX])*MY_HP_PENALTY*new_state[MY_HP_IDX] # So reward penalty gets worse for getting hit
			reward = reward + (new_state[ENEMY_HP_IDX] - prev_state[ENEMY_HP_IDX])*THEIR_HP_REWARD*new_state[ENEMY_HP_IDX] # Reward if hitting!
		reward = reward - ((abs(new_state[MY_X] - new_state[ENEMY_X]) * .25) + (abs(new_state[MY_Y] - new_state[ENEMY_Y])*.1)) # Slight penalty for going away from the user.
		self.add_OverallScore(prev_state[MY_HP_IDX] > new_state[MY_HP_IDX], prev_state[ENEMY_HP_IDX] > new_state[ENEMY_HP_IDX], new_state[MY_HP_IDX]-prev_state[MY_HP_IDX], new_state[ENEMY_HP_IDX]-prev_state[ENEMY_HP_IDX])
		return reward

	def add_OverallScore(self, hasDied, otherDied, myHP, theirHP):
		self.game_score = self.game_score + (SUICIDE_REWARD if hasDied == 1 else 0) + (KILL_SCORE if otherDied == 1 else 0)
		self.my_de = self.my_de + (1 if hasDied == 1 else 0)
		self.my_kill = self.my_kill + (1 if otherDied == 1 else 0)
		#(-myHP * .2) + (theirHP * .15)
		if not hasDied:
			self.game_score = self.game_score + (-myHP * MY_HP_PENALTY)
			self.my_hp = self.my_hp + myHP
		if not otherDied:
			self.game_score = self.game_score + (theirHP * THEIR_HP_REWARD)
			self.my_dmg = self.my_dmg + theirHP
		
	def create_model(self):
		model = Sequential()
		model.add(fallbackLSTM(MODEL_PARAMETERS[1],input_shape=MODEL_PARAMETERS[MY_HP_IDX], activation='tanh', return_sequences=True))
		model.add(Dropout(MODEL_DROPOUT[0]))
		model.add(fallbackLSTM(MODEL_PARAMETERS[2], activation='tanh'))
		model.add(Dropout(MODEL_DROPOUT[1]))
		model.add(Dense(MODEL_PARAMETERS[3], activation='relu'))
		# Reason for a small second to last dense layer is that 
		# A LOT of the values are highly correlated (sadly), so we don't expect a lot of difference here. 
		model.add(Dropout(MODEL_DROPOUT[2]))
		model.add(Dense(MODEL_PARAMETERS[4], activation='linear'))
		
		self.opt = tf.keras.optimizers.Adam(lr=self.learning_rate, decay=1e-5)
		model.compile(loss="mean_squared_error",
			optimizer=self.opt)
		return model

	def remember(self, state, action, reward, new_state, done):
		self.memory.append([state, action, reward, new_state, done])

	def get_real_action(self, action):
		for y in range(len(self.actions)):
			x = self.actions[y]
			if sum([(1 if abs(x[i] - action[i]) >= 0.01 else 0) for i in range(len(action))]) == 0:
				return y

	def replay(self):
		if len(self.memory) < MAX_BATCH_SIZE:
			return
		samples = random.sample(self.memory, MAX_BATCH_SIZE)
		for sample in samples:
			state, action, reward, new_state, done = sample
			action = self.get_real_action(action)
			target = self.target_model.predict(state)
			if done:
				target[0][action] = reward
			else:
				Q_future = max(self.target_model.predict(new_state)[0])
				target[0][action] = reward + Q_future * self.gamma
			self.model.fit(state, target, epochs=1, verbose=0)
		self.memory.clear()

	def act(self, state):
		self.epsilon *= self.epsilon_decay
		self.epsilon = max(self.epsilon_min, self.epsilon)
		if np.random.random() < self.epsilon:
			return self.actions[int(np.random.random()*len(self.actions)//1)]
		return self.actions[np.argmax(self.model.predict(state)[0])]

	def target_train(self):
		weights = self.model.get_weights()
		target_weights = self.target_model.get_weights()
		for i in range(len(target_weights)):
			target_weights[i] = weights[i] * self.tau + target_weights[i] * (1 - self.tau)
		self.target_model.set_weights(target_weights)
	
	def save_model(self, fn):
		self.model.save(fn)
	
	def load_model(self, fn):
		self.model = tf.keras.models.load_model(fn)
		self.target_model = tf.keras.models.load_model(fn)
	
	def test(self, fn):
		plot_model(self.model, to_file='model_plot.png', show_shapes=True, show_layer_names=True)

# BUILD MODEL SECTION!
agent = DQN(int(GPU_Partition))
if modelMode % 2 == 0:
	debugPrint("Loading model from file...\n")
	agent.load_model(parentFile)
else:
	debugPrint("Building model...\n")
stderr.flush()
debugPrint("Finished!\nPlease input data:\n")

pa = deque(maxlen=MAX_FRAMES_RECORDING)
kill_me = deque(maxlen=MAX_FRAMES_RECORDING)
timeout_timer = 0
for i in range(MAX_FRAMES_RECORDING):
	pa.append([0 for i in range(INPUT_SIZE)])
	kill_me.append([0 for i in range(INPUT_SIZE)])
outcount = 0
outval = [0,0,0,0,0,0,0]
while True:
	try:
		input_k = getInput(256)
		if "exit please" in input_k:
			break
	except:
		break
	action = agent.act(np.reshape(np.array(pa), (1,MAX_FRAMES_RECORDING,INPUT_SIZE)))
	# It is 6 values, brute force
	PipePrint((action[0]+1)/2,(action[1]+1)/2,action[2],action[3],action[4],action[5],action[6])
	outval = [(outval[i] * outcount + action[i])/(float(outcount + 1)) for i in range(7)]
	outcount = outcount + 1
	stderr.flush()
	# Output action....
	
	vv = [float(x) for x in input_k.strip().split(" ")] # Cur state!
	
	if(len(vv) != INPUT_SIZE):
		stderr.write('\0' + "GO GO TENSORFLOW!" + '\0')
		stderr.flush()
		continue
	reward = agent.get_Score(pa, vv)
	if modelMode < 2:
		kill_me.append(vv)
		if INPUT_RATE-1 == timeout_timer:
			timeout_timer = -1
			
			agent.remember(np.reshape(np.array(pa), (1,MAX_FRAMES_RECORDING,INPUT_SIZE)), action, reward, np.reshape(np.array(kill_me), (1,MAX_FRAMES_RECORDING,INPUT_SIZE)), False)
			agent.replay()
			agent.target_train()
		timeout_timer = timeout_timer+1
	pa.append(vv)
	
def SaveModel(filename):
	stderr.flush()
	debugPrint("End of file reached. Saving model.\n")
	stderr.write('\0' + "SAVING MODEL" + '\0')
	stderr.flush()
	scoreFile = filename + ".txt"
	e = os.path.isfile(scoreFile)
	if e:
		f = open(scoreFile, 'r')
		s = f.read()
		f.close()
		if float(s) > agent.game_score:
			debugPrint("A worse score was recorded! Not going to save this model.\n")
			return
	f = open(scoreFile, 'w')
	f.write(str(agent.game_score) + "")
	f.close()
	agent.save_model(filename)
	# now for the fun part
	stderr.write('\0' + "GO GO GOOGLE GADGETS" + '\0')
	stderr.flush()
	scope = ['https://spreadsheets.google.com/feeds', 'https://www.googleapis.com/auth/drive']
	creds = ServiceAccountCredentials.from_json_keyfile_name('client_secret.json', scope)
	client = gspread.authorize(creds)
	sheet = client.open("SSBM.io Statistics")
	s1 = sheet.sheet1
	row = [agent.game_score, agent.my_dmg, agent.my_hp, agent.my_de, agent.my_kill,time.time() - STARTING_TIME, *[i for i in outval], float(sys.argv[4])]
	s1.append_row(row)
	stderr.write('\0' + "MISSION COMPLETE" + '\0')
	stderr.flush()

if modelMode < 2:
	SaveModel(childFile)
	debugPrint("Finished savinge,exiting...\n")
PipePrint(-1,-1)