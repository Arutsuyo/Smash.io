# SSBM.io

## Required Software ##

### Windows Subsystem for Linux (Ubuntu 18.04) ###
If you are using WSL, you will need an X server, we used x410 during development. To activate it, follow this guide:
https://token2shell.com/howto/x410/setting-up-wsl-for-linux-gui-apps/


### Dolphin-emu ###
installed this via `apt-get install dolphin-emu`.


### Anaconda (VIA CMD) ###
Download:
https://www.anaconda.com/distribution/#download-section

Update steps:
```
conda update conda
conda update anaconda
conda update python
conda update --all
```

required Anaconda Packages:
tensorflow-gpu keras-gpu h5py zc.lockfile gspread oauth2client

Enviroment setup:
```
conda create --name tf-gpu
conda activate tf-gpu
conda install tensorflow-gpu keras-gpu h5py zc.lockfile pip
pip install gspread oauth2client
```

You can test the Anaconda enviroment by using:
```
conda activate tf-gpu
>>> import tensorflow as tf
>>> hello = tf.constant('Hello, TensorFlow!')
>>> sess = tf.Session()
>>> print(sess.run(hello))
```
you should recieve output from tensorflow detailing your 
GPU capabilities.

This was a helpful guide:
https://www.pugetsystems.com/labs/hpc/The-Best-Way-to-Install-TensorFlow-with-GPU-Support-on-Windows-10-Without-Installing-CUDA-1187/


### CUDA tollkit ###
Download:
https://developer.nvidia.com/cuda-downloads

Install Instructions:
Follow the on screen default


### CUDNN ###
Download:
https://developer.nvidia.com/rdp/cudnn-download

Install guide (Make sure to set up the paths correctly):
https://docs.nvidia.com/deeplearning/sdk/cudnn-install/index.html#install-windows


## Build Instructions ##
Create a build folder in your prefered location. 
If you have folders in your home directory, it would be HIGHLY advised to use the `-u` argument to designate a custom 
user directory that can be used by the program. Otherwise there is a chance that your folders can be deleted.
```
# IN CMD, in the root dir
conda activate tf-gpu
bash
mkdir build
cmake ..
make
./SSBM -gcm <path/to/ssbm.gcm>
```

### Arguments ###
These are the main arguments used, more can be found by running with -h:
```
-vs human/self(h/s) The play type used.
-gcm <path/to/ssbm.gcm> the location override for the super smash brothers melee iso/gcm
-pr <0|1|2|3> Tensorflow load type: 0=Load Model 1=New Model 2=Prediction Only 3=New model+Prediction Only
-p <python_alias> Specifies command to use then launching tensorflow/keras
-t # The number of instances to run. This should only be used on CPU's with >=8 cores.
```

## Overview ##
We communicate with Dolphin in 2 ways:
- The Memory is read via a socket that is triggered when it detects the Locations.txt file containing the memory addresses we request.
- We construct named pipes (Only available in Linux/WSL) in the specified folder which dolphin detects and uses as input devices. This is how we send the predicted controls back to dolphin.

Our C++ program uses a Pipe/Fork/Exec chain to launch both Dolphin and Keras (via Python/Anaconda) and handle the communication between the processes. 
Each instance of DolphinHandler has the given threads/processes:
- fork() Main Dolphin Process (2 threads) CPU-GPU cores of the main emulator.
- fork() Python Keras Process (1 per AI, totaling 2 if training with self play) (With cudNN GPU acceleration). This is only active as long as it takes to make a prediction or reshape. The communication with this process is done through pipe()'s for stdin/out/and err.
- std::thread MemoryScanner. This is a short while loop that will run as long as the socket is open and DolphinHandler is running.
- std::thread DolphinHandler thread. This handles the communication between all the other threads and tracks Stage/Menu changes.
- The main thread for the project tracks the status of each DolphinHandler and recycles them when they close, if they close safely. This thread is in a non-busy wait on a conditional variable, for 5 seconds at a time, activating early if one of the DolphinHandler notifies it.
