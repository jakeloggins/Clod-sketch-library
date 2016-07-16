Clod Sketch Library
===================

The Clod Sketch Library allows users with little programming knowledge to easily customize and upload sketches to esp devices. From an interface such as the Crouton dashboard, a user can simply select the name of a sketch, enter some customizing information, and press upload. Since all sketches work with over-the-air updates, a user can re-select the esp chip and upload a new sketch at anytime. This section covers how to prepare a sketch so that it is compatible with the Clod Sketch Library, and how to add it to the library. **If you just want to use Clod, you do not need to read this section.**


Create Your Own Sketch
----------------------

### Custom Sketch Protocol

##### Why is this Necessary?

Suppose a user wants to upload a sketch that monitors and logs the temperature, we'll call it `Basic_Temp`. Per the [walkthrough](), the sketch needs to know how to respond to `/deviceInfo/` requests to its endpoint topics. So we'll program the sketch to give the device a name of `Basic Temperature`, a single endpoint of `temperature`, and a path of `/house/`. Easy enough. 

The uploader script will happily upload this sketch to a device. It will lookup information about the esp device, navigate to the sketch's Platform IO project folder, modify the platformio.ini file, and run the Platform IO command. Everything is great.

But what happens when a user wants to monitor the temperature in 5 different locations on the same network? All five devices will be transmitting to `/house/control/basicTemperature/temperature`. The Crouton dashboard will display the temperature readings of 5 different devices as if it's a single device. 

Therefore, the protocol allows the user to specify unique device and endpoint names but keep the same basic sketch.


##### Namepins.h

The uploader script handles user customization by writing to the file `namepins.h` that is included in the main sketch file of `main.cpp`. In this way, customized information received by the uploader script may be incorporated to the sketch without repeatedly hardcoding the main file. Each time a user uploads a sketch, the `namepins.h` file is overwritten.


##### Identifcation Variables

Within `namepins.h`, the following variables are created from device object:

* `String thisDeviceName` - User customized device name.

* `String thisDevicePath` - User customized device path.

* `String subscribe_path` - The path followed by a `#` symbol to subscribe to all required endpoints.


##### Pin Numbers

Different espressif models have different I/O pin configurations. To allow a sketch to work with all models, the uploader script reads the `board_type` from within the `espInfo` object and defines letters to pin numbers. For example, a `board_type` of `esp01_1m` would write this into the namepins.h file:

`#define PIN_A 2`

... while a `board_type` of `esp07` would write the following:

```
#define PIN_A 4
#define PIN_B 5
#define PIN_C 12
#define PIN_D 13
```

Therefore, a sketch can refer to PIN_A and the uploader script will decide the appropriate pin number.


##### Static Endpoint Id



##### Default Endpoints



##### Custom Endpoints






##### Hookup Guide

##### Platform IO File Structure

For now, all custom sketches can be found in Crouton's sketches directory. These follow Platform IO's project folder format, which you can read about [here](http://docs.platformio.org/en/latest/ide/atom.html#setting-up-the-project). The Crouton dashboard's uploader interface looks through this directory and generates a list of the project folders. Your sketch should be named main.cpp and placed in the project's `src` folder. Libraries for only your project go in the project's `lib` folder. The `.pioenvs` folder is Platform IO's cache system. Do not edit the `.pioenvs` folder, it will be periodically deleted by Platform IO and ignored within this git.  




// behavior of a custom sketch on an esp chip
	
	// ask persistence/control/device_name/chipID "request states" --  do you have any states with my device_name or chipID
	
	    // if not, receive reply from deviceInfo/control/device_name "no states" 

	      // send deviceInfo/confirm/device_name {default object} -- send the device info object with default endpoints included
	        // NOTE: This step probably isn't necessary. persistence should always have 

	    // if yes, receive each endpoint from [device path]/control/[device_name]/[endpoint_key] (camelized card title) 

	      // receive each endpoint one by one and it's corresponding value

	        // send endpoint_key to function stored in namepins.h at compile time
	        
	        // function returns static_endpoint_id associated with that endpoint
	    
	        // sketch acts on that value as it normally would, using the static_endpoint_id to know for sure what it should do (turn output pin on/off, adjust RGB light, etc)
	        
	        // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]


### But I don't care about sharing with other people, can I just quickly hack something together for myself?

Yes. Look at the example clients, MQTT Standard, and the walkthrough. Write a sketch that does the minimum to follow those guidelines and upload it to your device manually. Every aspect of Clod, excluding the uploader, will be fully available to your sketch.


### What about non-espressif devices?

Currently, the uploader is only compatible with espressif based devices. But future releases of the uploader script can, in theory, easily handle any device or platform that works with Platform IO.


Add Your Sketch to the Library
------------------------------

For now, just add your sketch folder to this git and submit a pull request. 


Automatic Sketch Library Updates
--------------------------------

Coming soon: updating the library after installation.
Coming soon: browsing and downloading individual sketches from within an easy interface.

