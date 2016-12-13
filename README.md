Clod Sketch Library
===================

The Clod Sketch Library allows users with little programming knowledge to easily customize and upload sketches to esp devices. From an interface such as the Crouton dashboard, a user can simply select the name of a sketch, enter some customizing information, and press upload. Since all sketches work with over-the-air updates, a user can re-select the esp chip and upload a new sketch at anytime. This section covers how to prepare a sketch so that it is compatible with the Clod Sketch Library, and how to add it to the library. **If you just want to use Clod, you do not need to read this section.**


Custom Sketch Protocol
----------------------

#### Why is this Necessary?

Suppose a user wants to upload a sketch that monitors and logs the temperature, we'll call it `Basic_Temp`. Per the [walkthrough](https://github.com/jakeloggins/Clod-scripts/blob/master/README.md), the sketch needs to know how to respond to `/deviceInfo/` requests to its endpoint topics. So we'll program the sketch to give the device a name of `Basic Temperature`, a single endpoint of `temperature`, and a path of `/house/`. Easy enough. 

The uploader script will happily upload this sketch to a device. It will lookup information about the esp device, navigate to the sketch's Platform IO project folder, modify the platformio.ini file, and run the Platform IO command. Everything is great.

But what happens when a user wants to monitor the temperature in 5 different locations on the same network? All five devices will be transmitting to `/house/control/basicTemperature/temperature`. The Crouton dashboard will display the temperature readings of 5 different devices as if it's a single device. 

Therefore, the protocol allows the user to specify unique device and endpoint names but keep the same basic sketch.


#### Namepins.h

The uploader script handles user customization by writing to the file `namepins.h` that is included in the main sketch file of `main.cpp`. In this way, customized information received by the uploader script may be incorporated to the sketch without repeatedly hardcoding the main file. Each time a user uploads a sketch, the `namepins.h` file is overwritten.


#### Identifcation Variables

Within `namepins.h`, the following variables are created from the device object:

* `String thisDeviceName` - User customized device name.

* `String thisDevicePath` - User customized device path.

* `String subscribe_path` - The path followed by a `#` symbol to subscribe to all required endpoints.


#### Pin Numbers

Different espressif models have different I/O pin configurations. To allow a sketch to work with all models, the uploader script reads the `board_type` from within the `espInfo` object and defines letters to pin numbers. For example, a `board_type` of `esp01_1m` would write this into the `namepins.h` file:

`#define PIN_A 2`

... while a `board_type` of `esp07` would write the following:

```
#define PIN_A 4
#define PIN_B 5
#define PIN_C 12
#define PIN_D 13
```

Therefore, a sketch can refer to PIN_A and the uploader script will decide the appropriate pin number.


#### Static Endpoint Id

To begin the upload process, a device object is sent to the uploader script. Within the device object is the endpoints object, which is the main way Clod updates values between users and devices (a detailed look at the device object is in the [walkthrough](https://github.com/jakeloggins/Clod-scripts). Here's an example of a device's endpoints object:

```json
"endPoints": {
      "lastTimeTriggered": {
        "title": "Last Time Triggered",
        "card-type": "crouton-simple-input",
        "static_endpoint_id": "time_input",
        "values": {
          "value": "n/a"
        }
      },
      "alarmLight": {
        "title": "Alarm Light",
		"card-type": "crouton-rgb-slider",
        "static_endpoint_id": "RGB",
        "values": {
		    "red": 0,
    		"green": 0,
    		"blue": 0
  		},
		"min": 0,
		"max": 255,
      }
    }
```

Each object is an endpoint, its key is simply a camelized version of the title. `static_endpoint_id` tells the sketch *what the endpoint actually does* so that the user can ensure all device and endpoint names throughout Clod are unique without hardcoding the main sketch file.

The uploader script writes a `lookup` function into `namepins.h` that allows the sketch to associate the `endpoint_key` with the `static_endpoint_id`. Here's an example:

```arduino
String lookup(String endpoint_key) {
	if (endpoint_key == "alarmLight") {
		return "RGB";
	}
}
```

When the sketch receives a message, it should send the endpoint portion of the MQTT topic to the `lookup` function. The function will return a String, which the sketch can use to know what the endpoint is supposed to do. In the example above, the user has named an endpoint `Alarm Light`. The lookup function associates that name with `RGB`. The sketch is programmed to adjust the values of a neopixel strip whenever the lookup function sends it the string `RGB`. 


#### Platform IO File Structure

For now, all custom sketches can be found in their own folder within Crouton's sketches directory. These follow Platform IO's project folder format, which you can read about [here](http://docs.platformio.org/en/latest/ide/atom.html#setting-up-the-project). The Crouton dashboard's uploader interface looks through the sketches directory and generates a list of the project folders. Your sketch should be named main.cpp and placed in the project's `src` folder. Libraries for only your project go in the project's `lib` folder. The `.pioenvs` folder is Platform IO's cache system. Do not edit the `.pioenvs` folder, it will be periodically deleted by Platform IO and ignored within this git.  


#### Default Endpoints

A sketch should have a `default_endpoints.json` file within its project folder. This is used by the uploader script to create the endpoints object if one was not sent to it. Default endpoint keys and titles should be intuitive and short. The default endpoint file *must* have a static_endpoint_id, because uploader creates a fresh namepins.h file before each upload. Here's an example default_endpoints.json file:

```json
{
  "alarmLight": {
    "min": 0,
    "max": 255,
    "card-type": "crouton-rgb-slider",
    "static_endpoint_id": "RGB",
    "title": "Alarm Lights",
    "values": {
      "red": 0,
      "green": 0,
      "blue": 0
    }
  }
}
```

#### Custom Endpoints

Custom endpoints can be changed on the user's end by simply changing the `default_endpoints.json` file. Future releases of Clod will bring this process to the Crouton dashboard, and introduce the use of a separate `custom_endpoints.json` file.


#### Hookup Guide

The hookup guide provides an additional set of instructions to the user after they have completed the upload process. This step is particularly important for non-technical users. A `guide.md` file should be included in the sketch's project folder. If there are any specific precautions, notes, or wiring instructions that go along with the sketch, they should be placed here. When the upload process is executed within the Crouton dashboard, the user will be prompted to view this file. 


### But I don't care about sharing with other people, can I just quickly hack something together for myself?

Yes. Look at the example clients, MQTT Standard, and the walkthrough. Write a sketch that does the minimum to follow those guidelines and upload it to your device manually. Every aspect of Clod, excluding the uploader, will be fully available to your device.


### What about non-espressif devices?

Currently, the uploader is only compatible with espressif based devices. But future releases of the uploader script can, in theory, easily handle any device or platform that works with Platform IO.


Add Your Sketch to the Library
------------------------------

For now, just add your sketch folder to this git and submit a pull request. 


Automatic Sketch Library Updates
--------------------------------

Coming soon: updating the library after installation.
Coming soon: browsing and downloading individual sketches from within an easy interface.

