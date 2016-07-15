Clod Sketch Library
===================

The Clod Sketch Library allows users with little programming knowledge to easily customize and upload sketches to esp devices. From an interface such as the Crouton dashboard, a user can simply select the name of a sketch, enter some customizing information, and press upload. Since all sketches work with over-the-air updates, a user can re-select the esp chip and upload a new sketch at anytime. This section covers how to prepare a sketch so that it is compatible with the Clod Sketch Library, and how to add it to the library. **If you just want to use Clod, you do not need to read this section.**


Create Your Own Sketch
----------------------

### Custom Sketch Protocol

##### Why is this Necessary?

Suppose a user wants to upload a sketch that monitors and logs the temperature, we'll call it `Basic_Temp`. Per the [walkthrough](), the sketch needs to know how to respond to `/deviceInfo/` requests and respond appropriately to its endpoint topics. So we'll program the sketch to give the device a name of `Basic Temperature`, a single endpoint of `temperature`, and a path of `/house/`. Easy enough. 

The uploader script will happily upload this sketch to a device. It will lookup information about the esp device, navigate to the sketch's Platform IO project folder, modify the platformio.ini file, and run the Platform IO command. Everything is great.

But what happens when a user wants to monitor the temperature in 5 different locations on the same network? All five devices will be transmitting to `/house/control/basicTemp/temperature`. The Crouton dashboard will display the temperature readings of 5 different devices as if it's a single device. 

Therefore, the protocol allows the user to specify unique device and endpoint names but keep the same basic sketch.



##### Platform IO File Structure

##### User Customization

##### Hookup Guide


The main problem with the 



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

Yes. Look at the example clients, MQTT Standard, and the walkthrough. Create a sketch that does the minimum to follow it and upload it to your device manually. Every aspect of Clod, excluding the uploader, will be fully available to your sketch.


### What about non-espressif devices?

Currently, the uploader is only compatible with espressif based devices. But future releases of the uploader script can, in theory, easily handle any device or platform that works with Platform IO.


Add Your Sketch to the Library
------------------------------

For now, just add your sketch folder to this git and submit a pull request. 


Automatic Sketch Library Updates
--------------------------------

Coming soon: the ability to update the library after installation from within an interface.

