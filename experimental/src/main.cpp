#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>

//for LED status
#include <Ticker.h>
Ticker ticker;


// -- global info --
  #include <namepins.h>
    // namepins stores
      // device name global variable - thisDeviceName
      // device path global variable - thisDevicePath
      // subscribe path global variable - subscribe_path (thisDevicePath += "/#")
      // startup pins according to board type
      // the lookup function for associating user defined endpoint names with endpoint_static_id during compile time
  #include <wifilogin.h>
    // located in platformio library folder (usually [home]/.platformio/lib)
    // stores const char ssid and const char password


String chip_id = String(ESP.getChipId());
String confirmPath = "";
String confirmPayload = "";
String local_ip_str = "";

String error_path = "";


int redValue = 0;
int greenValue = 0;
int blueValue = 0;

boolean neoPixelChange = false;

static uint32_t MQTTtick = 0;
static uint32_t MQTTlimit = 300;


// -- MQTT server setup
  #include <PubSubClient.h>
  IPAddress server(192, 168, 1, 140);
  
  #define BUFFER_SIZE 100
  
  WiFiClient espClient;
  PubSubClient client(espClient, server, 1883);
  boolean sendConfirm = false;



// -- NeoPixelBus
  #include <NeoPixelBus.h>

  const uint16_t PixelCount = 30; // this example assumes 4 pixels, making it smaller will cause a failure
  const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

  #define colorSaturation 256

  // three element pixels, in different order and speeds
  //NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, Neo400KbpsMethod> strip(PixelCount, PixelPin);

  // You can also use one of these for Esp8266,
  // each having their own restrictions
  //
  // These two are the same as above as the DMA method is the default
  // NOTE: These will ignore the PIN and use GPI03 pin
  //NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod> strip(PixelCount, PixelPin);

  // Uart method is good for the Esp-01 or other pin restricted modules
  // NOTE: These will ignore the PIN and use GPI02 pin
  NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, NeoEsp8266Uart400KbpsMethod> strip(PixelCount, PixelPin);

  // The bitbang method is really only good if you are not using WiFi features of the ESP
  // It works with all but pin 16
  //NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang400KbpsMethod> strip(PixelCount, PixelPin);

  // four element pixels, RGBW
  //NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

  RgbColor red(colorSaturation, 0, 0);
  RgbColor green(0, colorSaturation, 0);
  RgbColor blue(0, 0, colorSaturation);
  RgbColor white(colorSaturation);
  RgbColor black(0);

  HslColor hslRed(red);
  HslColor hslGreen(green);
  HslColor hslBlue(blue);
  HslColor hslWhite(white);
  HslColor hslBlack(black);


// -- NTP
  unsigned int localPort = 2390;      // local port to listen for UDP packets
  /* Don't hardwire the IP address or we won't get the benefits of the pool.
   *  Lookup the IP address for the host name instead */
  //IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
  IPAddress timeServerIP; // time.nist.gov NTP server address
  const char* ntpServerName = "time.nist.gov";
  const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
  // A UDP instance to let us send and receive packets over UDP
  WiFiUDP udp;
  // wait time for displaying NTP
  static uint32_t tick = 0;
  static uint32_t NTPlimit = 30000;

// -- TICK FUNTION
  void tick_fnc()
  {
    //toggle state
    int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
  }

// -- MQTT functions

  String getValue(String data, char separator, int index)
  {
   yield();
   int found = 0;
    int strIndex[] = {0, -1  };
    int maxIndex = data.length()-1;
    for(int i=0; i<=maxIndex && found<=index; i++){
      if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
   }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
  }

  // --------------------------------------------------------
  // HIGH LEVEL OVERVIEW OF ESP AND PERSISTENCE COMMUNICATION
  // --------------------------------------------------------
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



  String topic;
  String payload;
  void callback(const MQTT::Publish& pub) {
    yield();
    if (millis() - MQTTtick > MQTTlimit) {
      MQTTtick = millis();
    

      int commandLoc;
      String command = "";
      String deviceName = "";
      String endPoint = "";

      topic = pub.topic();
      payload = pub.payload_string();

      // -- topic parser
          // syntax: 
          // global: / global / path / command / function
          // device setup: / deviceInfo / command / name
          // normal: path / command / name / endpoint

      // check item 1 in getValue
      String firstItem = getValue(topic, '/', 1);
      if (firstItem == "global") {
        // -- do nothing until I change to $ prefix before command types
      }
      else if (firstItem == "deviceInfo") {
        // get name and command
        deviceName = getValue(topic, '/', 3);
        command = getValue(topic, '/', 2);
        if ((deviceName == thisDeviceName) && (command == "control")) {
          if (payload == "no states") {
            // -- do something to send the default states, but now that this is managed by persistence this shouldn't be necessary
            // -- maybe it just resets all the states to 0 or whatever was originally programmed into the sketch
            //sendJSON = true;
          }
          else if (payload == "blink on") {
            Serial.end();
            pinMode(BUILTIN_LED, OUTPUT);
            ticker.attach(0.6, tick_fnc);
          }
          else if (payload == "blink off") {
            ticker.detach();
            digitalWrite(BUILTIN_LED, HIGH);
            pinMode(BUILTIN_LED, INPUT);
            Serial.begin(115200);
          }
          else {
            // -- persistence will no longer send the default object, if it's an arduino based esp chip, it will just send the control messages to /[device_path]/control/[device_name]/[endpoint_key]
          }






        }
          
      }
      else {

        int i;
        int maxitems;
        
        // count number of items
        for (i=1; i<topic.length(); i++) {
          String chunk = getValue(topic, '/', i);
          if (chunk == NULL) {
            break;
          }
        }

        // get topic variables
        maxitems = i;
        
        for (i=1; i<maxitems; i++) {
          String chunk = getValue(topic, '/', i);
          if (chunk == "control") {
            commandLoc = i;
            command = chunk;
            deviceName = getValue(topic, '/', i + 1);
            endPoint = getValue(topic, '/', i + 2);
            break;
          }
        }

        //Serial.println("device and endpoint incoming...");
        //Serial.println(deviceName);
        //Serial.println(endPoint);

          // send endpoint_key to function stored in namepins.h at compile time
          // function returns static_endpoint_id associated with that endpoint
        
          String lookup_val = lookup(endPoint);
          //Serial.println("looking value incoming...");
          //Serial.println(lookup_val);

          // sketch acts on that value as it normally would, using the static_endpoint_id to know for sure what it should do (turn output pin on/off, adjust RGB light, etc)
          if (lookup_val == "RGB") {
            // deserialize payload, get valueKey
            // or just look for value or red,green,blue
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
            //neoPixelChange = true;
          }
          /*
          else if (lookup_val == "SECOND STATIC ENDPOINT ID") {
            
          }
          else if (lookup_val == "THIRD STATIC ENDPOINT ID") {

          }
          */


          // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]

          confirmPath = "";
          confirmPath = thisDevicePath;
          confirmPath += "/confirm/";
          confirmPath += thisDeviceName;
          confirmPath += "/";
          confirmPath += endPoint;
          confirmPayload = payload;

          //sendConfirm = true;
          client.publish(MQTT::Publish(confirmPath, confirmPayload).set_qos(2));

      }
    }




  }


// -- NeoPixel Sunrise Functions
  void sunrise(uint16_t wait) {
    uint16_t i, j;

    for(j=0; j<256; j++) {
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (j, 0, 0));
      }
      strip.Show();
      delay(wait);
    }
  }

  void offblack() {
      uint16_t i;
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (0, 0, 0));
      }
      strip.Show();
  }


  void flashblue(uint8_t wait, uint8_t flashes) {
    uint16_t i, z;
    for (z=0; z<flashes; z++) {
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (0, 0, 255));
      }
      strip.Show();
      delay(wait);
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (0, 0, 0));
      }
      strip.Show();
      delay(wait);
    }
  }

  void flashgreen(uint8_t wait, uint8_t flashes) {
    uint16_t i, z;
    for (z=0; z<flashes; z++) {
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (0, 255, 0));
      }
      strip.Show();
      delay(wait);
      for(i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (0, 0, 0));
      }
      strip.Show();
      delay(wait);
    }
  }




void setup() {

  Serial.begin(115200);
  Serial.println("Booting");
  Serial.println(ESP.getResetInfo());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());


  // -- OTA
    // OTA options
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);
    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");
    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  local_ip_str = WiFi.localIP().toString();
  Serial.println(local_ip_str);

  // neopixel bus
  strip.Begin();
  strip.Show();

}

// send an NTP request to the time server at the given address
  unsigned long sendNTPpacket(IPAddress& address)
  {
    //Serial.println("sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    yield();
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
  }




void loop() {
  ArduinoOTA.handle();

  // -- MQTT connect
    if (!client.connected()) {
      error_path += thisDevicePath;
      error_path += "/";
      error_path += "errors/";
      error_path += thisDeviceName;
      if (client.connect(MQTT::Connect(thisDeviceName).set_will(error_path, "disconnected"))) {
        Serial.println("MQTT connected");
        client.set_callback(callback);
        client.subscribe(MQTT::Subscribe()
          .add_topic("/deviceInfo/#")
          .add_topic("/global/#")
          .add_topic(subscribe_path)
                 );
        // notify persistence of device IP
          String persistence_ip_path = "/persistence/control/";
          persistence_ip_path += thisDeviceName;
          persistence_ip_path += "/ip";
          client.publish(MQTT::Publish(persistence_ip_path, local_ip_str).set_qos(2));

        // ask persistence/control/device_name/chipID "request states" --  do you have any states with my device_name or chipID
          String persistence_path = "/persistence/control/";
          persistence_path += thisDeviceName;
          persistence_path += "/";
          persistence_path += chip_id;
          client.publish(MQTT::Publish(persistence_path, "request states").set_qos(2));
          Serial.println("request states sent");
      }
    }

    if (client.connected()) {
      client.loop();
    }

  /*
  // -- Device Request Response
    if (sendJSON) {
      buildDeviceJson(); // flag set to false inside function
    }
  */

  /*
  // -- Confirm Messages
    if (sendConfirm) {
      client.publish(MQTT::Publish(confirmPath, confirmPayload).set_qos(2));
      sendConfirm = false;
    }
  */

  /*
  // -- NeoPixel updates
    if (neoPixelChange) {
      for(int i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (redValue, greenValue, blueValue));
      }
      strip.Show();
      neoPixelChange = false;
    }
  */

  // -- NeoPixel continuous update
    for(int i=0; i<PixelCount; i++) {
      strip.SetPixelColor(i, RgbColor (redValue, greenValue, blueValue));
    }
    strip.Show();

  /*
  // -- NeoPixel Sunrise Alarm Code
    if (millis() - lastcheck > alarmcheck)  {
      DateTime now = RTC.now();
      lastcheck = millis();
      if (alarmminute == now.minute() && alarmhour == now.hour()) {
         sunrise(4000);
         sunrise(400);
         sunrise(40);
         sunrise(40);
         sunrise(40);
         sunrise(40);
         sunrise(40);
         sunrise(20);
         sunrise(20);
         black();
      }
    }
  */

  
  // Do things every NTPLimit seconds
  if ( millis() - tick > NTPlimit) {
    tick = millis();

    /*
    // -- get server and send NTP packet
      //get a random server from the pool
      WiFi.hostByName(ntpServerName, timeServerIP);

      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
      // wait to see if a reply is available
      delay(1000);

      int cb = udp.parsePacket();
      if (!cb) {
        Serial.println("no packet yet");
      }
      else {
        //Serial.print("packet received, length=");
        //Serial.println(cb);
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;

        int UTChour = (epoch  % 86400L) / 3600;   // (86400 equals secs per day)
        int UTCmin = (epoch  % 3600) / 60;        // (3600 equals secs per minute)
        //Serial.println(UTChour);

        Serial.print(year(epoch));
        Serial.print('-');
        Serial.print(month(epoch));
        Serial.print('-');
        Serial.println(day(epoch));

        Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
        Serial.print(UTChour);
        Serial.print(':');
        if ( ((epoch % 3600) / 60) < 10 ) {
          Serial.print('0');
        }
        Serial.print(UTCmin);
        Serial.print(':');
        if ( (epoch % 60) < 10 ) {
          Serial.print('0');
        }
        Serial.println(epoch % 60); // print the second
      }
    */

    /*
    // -- begin neropixel test
    
      delay(1000);

      Serial.println("Colors R, G, B, W...");

      // set the colors,
      // if they don't match in order, you need to use NeoGrbFeature feature
      strip.SetPixelColor(0, RgbColor (128, 0, 0));
      strip.SetPixelColor(1, green);
      strip.SetPixelColor(2, blue);
      strip.SetPixelColor(3, white);
      // the following line demonstrates rgbw color support
      // if the NeoPixels are rgbw types the following line will compile
      // if the NeoPixels are anything else, the following line will give an error
      //strip.SetPixelColor(3, RgbwColor(colorSaturation));
      strip.Show();


      delay(1000);

      Serial.println("Off ...");

      // turn off the pixels
      strip.SetPixelColor(0, black);
      strip.SetPixelColor(1, black);
      strip.SetPixelColor(2, black);
      strip.SetPixelColor(3, black);
      strip.Show();

      delay(1000);

      Serial.println("HSL Colors R, G, B, W...");

      // set the colors,
      // if they don't match in order, you may need to use NeoGrbFeature feature
      strip.SetPixelColor(0, hslRed);
      strip.SetPixelColor(1, hslGreen);
      strip.SetPixelColor(2, hslBlue);
      strip.SetPixelColor(3, hslWhite);
      strip.Show();


      delay(1000);

      Serial.println("Off again...");

      // turn off the pixels
      strip.SetPixelColor(0, hslBlack);
      strip.SetPixelColor(1, hslBlack);
      strip.SetPixelColor(2, hslBlack);
      strip.SetPixelColor(3, hslBlack);
      strip.Show();
    */

    // -- neopixel sunrise test
      //sunrise(40);
      //offblack();
    
    /*  
    // -- print IP address
      Serial.println(WiFi.localIP());
      //Serial.println("..");
    */
  } 
  

  yield();
}
