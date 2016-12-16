Clod Christmas Tree Tutorial
============================


### Intro

This project allows you to easily animate and control a strand of NeoPixels (WS2811/WS2812) with an ESP8266 microcontroller and a Raspberry Pi. You can choose colors and a wide range of animations via a simple web interface or MQTT message. The ESP8266 is programmed with an arduino sketch that interacts with a Raspberry Pi running [Clod](), which provides additional functionality such as OTA uploading, persistence, scheduling, and web dashboard controls.

The animations come from the excellent [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) library.

Here's a video of the tree in action:

[![Video](https://img.youtube.com/vi/PlFHaUBc0MQ/0.jpg)](https://www.youtube.com/watch?v=PlFHaUBc0MQ)


### Prerequisites

* [Clod]() installed and running on a Raspberry Pi

* An Espressif Chip [flashed]() with the Clod Initial_Config sketch, or other Clod sketch

* A strand of neopixels, or WS2812 / WS2811 LED strip


### Hardware

* I used [these lights](https://www.amazon.com/Agile-shop-Ws2811-Pixels-Digital-Addressable/dp/B017HAWXF0/ref=sr_1_5?ie=UTF8&qid=1481844073&sr=8-5&keywords=ws2811+christmas+lights) for the video above. They pop up from different sellers all across Amazon, so you might have to look around [here's the search term I used](https://www.amazon.com/s/ref=nb_sb_ss_i_5_7?url=search-alias%3Daps&field-keywords=ws2811+christmas+lights&sprefix=ws2811+%2Caps%2C201&crid=1A22FG0JFHN8C).

* Read and follow the [Adafruit Best Practices](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices) for supplying electricity to your NeoPixel strip and ESP8266

* Best practices, at a minimum, means ensuring proper logic level conversion from the 3.3v ESP8266 to the 5v NeoPixels and a large capacitor across the power terminals.

* Once everything is wired properly, follow the instructions to set up [Clod]() and [flash]() the chip.

* Upload the christmas_tree sketch and add the device (this is demonstrated at the beginning of the video).


### Code

Note: This tutorial is meant as a gentle introduction to the documentation found elsewhere.

The main file I'll be explaining so that you can make changes to the code is [main.cpp](https://github.com/jakeloggins/Clod-sketch-library/blob/master/sketches/christmas_tree/src/main.cpp). The pseudo code for that file is something like:

```
// Include files and global variables

// Huge section of animation functions and derivative functions

	// ---
	// chooseAnimation() function
	// ---

// MQTT and Clod specific code

	// ---
	// lookup_val code within the MQTT message callback function
	// ---

// setup
	
	// connect to WiFi, MQTT, and Clod

// main loop

	// connect to everything if you aren't already

	// if an animation is running, let it continue

		// ---
		// if it isn't, and the timeout has occurred: 
		// ---

			// select a new random animation (timeoutPlay toggle state is true)

			// ... or replay the old animation (timeoutPlay toggle state is false)

```

For simple changes, the only sections that matter are the chooseAnimation() function, the lookup_val if/else statements within the MQTT callback, and the switch case within the main loop. If you don't like an animation, comment it out from the chooseAnimation() function and the switch case. The chooseAnimation() is called when an option is slected from the menu. The switch case decides which animations play randomly after the timeout.

For slightly more complicated changes, you need to understand Clod and endpoints. Every user control in the video can be found in the [default_endpoints.json]() file. Here's what the RGB slider in that file looks like:

```
  "solidTreeColor": {
    "min": 0,
    "max": 255,
    "card-type": "crouton-rgb-slider",
    "static_endpoint_id": "RGB",
    "title": "Solid Tree Color",
    "values": {
      "red": 0,
      "green": 0,
      "blue": 0
    }
  },
```

This file tells Clod to make an RGB slider with the title of "Solid Tree Color" available on the dashboard. When you move those values around, it will send a Clod-formatted MQTT message to that device name and endpoint. In this example, that would be `/downstairs/livingroom/control/mainTree/solidTreeColor`.

The user can change the name the device name and endpoint, but `static_endpoint_id` is what your sketch should use to know how to handle that information. In `main.cpp`, that section looks like this:

```

if (lookup_val == "RGB") {

	lastMQTT = millis();
	StopAllAnimations();
	solidOverride = true;

	RGBendpoint = endPoint;

	String findKey = getValue(payload, '"', 1);
	String findValue = getValue(payload, ':', 1);
	findValue.remove(findValue.length() - 1);

	if (findKey == "red") {
	  redValue = findValue.toInt();
	}
	else if (findKey == "green") {
	  greenValue = findValue.toInt();
	}
	else if (findKey == "blue") {
	  blueValue = findValue.toInt();
	}
}

...

else if (lookup_val == "animationMenu") {

	lastMQTT = millis();
	StopAllAnimations();

	solidOverride = false;

	lastSelected = payload.substring(10);

	chooseAnimation();
}

else if (lookup_val == "timeoutPlay") {

	String findValue = getValue(payload, ':', 1);
	findValue.remove(findValue.length() - 1);

	if (findValue == "true") {
	  timeoutPlay = true;
	}
	else {
	  timeoutPlay = false;
	}
}

else if (lookup_val == "timeoutLength") {

	// grab number, multiply by 60000, assign it to timer variable

	String findValue = getValue(payload, ':', 1);
	findValue.remove(0,1); // removes the first quote
	findValue.remove(findValue.length() - 2); // removes the last quote and bracket

	timeoutMinutes = long(findValue.toInt());
	timeoutSeconds = (timeoutMinutes * 60000);

}
```

For our purposes, lookup_val IS the static_endpoint_id. How that assignment happens is covered elsewhere in the documentation. The user moves the "Solid Tree Color" RGB slider, the sketch knows from the static_endpoint_id that it should grab the value and store it in something `redValue`, `greenValue`, or `blueValue`.

Later on in the sketch, in the main loop, we use those values to set the strip color (among other things):

```
    if (solidOverride) {
      for(int i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (redValue, greenValue, blueValue));
      }
    strip.Show();
}
``` 

