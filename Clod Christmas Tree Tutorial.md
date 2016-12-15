Clod Christmas Tree Tutorial
============================


### Intro

This project allows you to easily animate and control a strand of NeoPixels (WS2811/WS2812) with an ESP8266 microcontroller and a Raspberry Pi. You can choose colors and a wide range of animations via a simple web interface or MQTT message. The ESP8266 is programmed with an arduino sketch that interacts with a Raspberry Pi running [Clod](), which provides additional functionality such as OTA uploading, persistence, scheduling, and web dashboard controls.

The list of available animations come from the excelent [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) library.

Here's a video of the tree in action:

[![Video](https://img.youtube.com/vi/PlFHaUBc0MQ/0.jpg)](https://www.youtube.com/watch?v=PlFHaUBc0MQ)


### Prerequisties

* [Clod]() installed and running on a Raspberry Pi

* An Espressif Chip [flashed]() with the Clod Initial_Config sketch, or other Clod sketch

* A strand of neopixels, or WS2812 / WS2811 LED strip


### Hardware

* I used [these lights](https://www.amazon.com/Agile-shop-Ws2811-Pixels-Digital-Addressable/dp/B017HAWXF0/ref=sr_1_5?ie=UTF8&qid=1481844073&sr=8-5&keywords=ws2811+christmas+lights) for the video above. They pop up from different sellers all across Amazon, so you might have to look around [here's the search term I used](https://www.amazon.com/s/ref=nb_sb_ss_i_5_7?url=search-alias%3Daps&field-keywords=ws2811+christmas+lights&sprefix=ws2811+%2Caps%2C201&crid=1A22FG0JFHN8C).

* Read and follow the [Adafruit Best Practices](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices) for supplying electricity to your NeoPixel strip and ESP8266

* Best practices, at a minimum, means ensuring proper logic level conversion from the 3.3v ESP8266 to the 5v NeoPixels and a large capacitor across the power terminals.

* Once everything is wired properly, follow the instructions to set up [Clod]() and [flash]() the chip with the [christmas_tree]() sketch.


### Code





