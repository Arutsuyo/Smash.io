# Smash.io

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

required Packages:
tensorflow-gpu keras-gpu h5py zc.lockfile

Enviroment setup:
```
conda create --name tf-gpu
conda activate tf-gpu
conda install tensorflow-gpu keras-gpu h5py zc.lockfile
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
- Create a build folder in your prefered location
```
# IN CMD, in the root dir
conda activate tf-gpu
bash
mkdir build
cmake ..
make
./SSBM
```