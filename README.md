A Sample rate decimator and bit depth reducer for Daisy Pod hardware (using DaisyDuino)

Written by Lars Netzel with aid from of GPT-5 (ChatGPT)

* Language: Arduino C++
* Dependency: Daisyduino 1.7.0 (https://github.com/electro-smith/DaisyDuino)
* IDE: Arduino 1.8.19 (Windows Store 1.8.57.0)

How to use:

* Plug stereo audio into Line, listen to processed audio through line out or phones.
* Button 1 will toggle pitch tracking on/off
* Button2 will toggle fx on/off
* Turning POT 1 clockwise will decimate more (48000 -> 100)
* Turning POT 2 counter clockwise will reduce more bits (16 -> 2)

Good to know:

With pitch tracking on more of the pitch will come through, works best on single instruments.
