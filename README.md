Clod Sketch Library
===================

The Clod Sketch Library allows users with little programming knowledge to easily customize and upload sketches to esp devices. From an interface such as the Crouton dashboard, a user can simply select the name of a sketch, enter some customizing information, and press upload. Since all sketches work with over-the-air updates, a user can re-select the esp chip and upload a new sketch at anytime. This section covers how to prepare a sketch so that it is compatible with the Clod Sketch Library, and how to add it to the library. **If you just want to use Clod, you do not need to read this section.**


Create Your Own Sketch
----------------------

### Custom Sketch Protocol





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


### But I don't care about this, can I just hardcode something?

Yes. Look at the example clients, MQTT Standard, and the walkthrough. Create a sketch that does the minimum to follow it and upload it to your device manually. Every aspect of Clod, excluding the uploader, will be fully available to your sketch.


### What about non-espressif devices?

Currently, the uploader is only compatible with espressif based devices. But future releases of the uploader script can, in theory, easily handle any device or platform that works with Platform IO.


Add Your Sketch to the Library
------------------------------

For now, just add your sketch folder to this repo and submit a pull request. 




Automatic Sketch Library Updates
--------------------------------

Coming soon: the ability to update the library and/or remove 

