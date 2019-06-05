import os

stdout = open(1, "w")
stdoin = open(0, "r")

stdout.write( ("Python Started"))
stdout.write( ("Python Started") + '\0')
stdout.write( ("Python Started"))
stdout.flush()
stdout.write( ("Python Started") + '\0')
stdout.write('\0' + ("pred: Python Started") + '\0') # this is the key line
stdout.write( ("Python Started"))
stdout.write( ("Python Started") + '\0')
stdout.flush()
keepGoing = True
while keepGoing:
	line = os.read(0, 256)
	stdout.write(line.decode("utf-8") + '\0')
	stdout.write(line.decode("utf-8"))
	stdout.write(line.decode("utf-8") + '\0')
	stdout.write('\0' + ("pred: " + line.decode("utf-8")) + '\0') # this is the key line
	stdout.write(line.decode("utf-8"))
	stdout.write(line.decode("utf-8") + '\0')
	stdout.flush()
	if "-1 -1" in line.decode("utf-8"):
		keepGoing = False
