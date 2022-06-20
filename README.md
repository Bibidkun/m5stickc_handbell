# m5stickc_handbell

# Handbell with m5stickc
Using two m5stickC, select a scale from the master and shake the slave to make a sound.
The two units are linked using Bluetooth communication.

<img src="https://user-images.githubusercontent.com/45535897/174513002-7e351c42-c715-4eba-8e24-08f1b619ad13.PNG" width="50%">
<img src="https://user-images.githubusercontent.com/45535897/174513006-e757da33-ce72-4c73-8a72-51b67ceac1e6.PNG" width="50%">

# Features
The scale can be selected from m5 on the master side for scale from 523.251 Hz to 1046.502 Hz.

# Requirement
device 
* M5StickC Speaker Hat（Equipped with PAM8303）
platform
* espressif32
board
* m5stick-c
framework
* arduino

Libraries

* m5stack/M5StickC 0.2.5
* kosme/arduinoFFT 1.5.6
* arduino-libraries/Madgwick 1.2.0

# Installation
Find the library from Platform IO.

# Usage
Import the master file and slave file into each of the two m5stickC.


