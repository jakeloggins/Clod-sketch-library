#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>


//for LED status
#include <Ticker.h>
Ticker ticker;


uint32_t t = 0;


// -- Servo setup

  #include <Wire.h>
  #include <Adafruit_PWMServoDriver.h>

  // called this way, it uses the default address 0x40
  Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
  // you can also call it with a different address you want
  //Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);  

  #define SERVOMIN  100 // this is the 'minimum' pulse length count (out of 4096)
  #define SERVOMAX  450 // this is the 'maximum' pulse length count (out of 4096)


  uint16_t pulselen;
  int selectedServo = 0;
  bool servoFlag = false;

  int selectedPos;
  int selectedOpen = 175;
  int selectedClose = 5;
  static uint32_t lastMove = 0;
  static uint32_t moveLimit = 250;

  int selectedFanSpeed = 90;



  void servoRun() {

    if (millis() - lastMove > moveLimit) {
      lastMove = millis();
     
      yield();
      // convert to PWM using map function
      pulselen = map(selectedPos, 0, 180, SERVOMIN, SERVOMAX);
      Serial.printf("selected servo .. %d \n", selectedServo);
      Serial.printf("selected pos .. %d \n", selectedPos);
      Serial.printf("pulse .. %d \n", pulselen);
      // set PWM
      pwm.setPWM(selectedServo, 0, pulselen);

    }
    
  }


// -- fan stuff
  bool fanToggle = false;



// -- temp setup
  int counter = 5;
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define ONE_WIRE_BUS 13

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  OneWire oneWire(ONE_WIRE_BUS);

  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature DS18B20(&oneWire);
  char temperatureCString[6];
  char temperatureFString[6];

  float tempC;
  float tempF;

  void getTemperature() {

    do {
      DS18B20.requestTemperatures(); 
      tempC = DS18B20.getTempCByIndex(0);
      dtostrf(tempC, 2, 2, temperatureCString);
      tempF = DS18B20.getTempFByIndex(0);
      dtostrf(tempF, 3, 2, temperatureFString);
      delay(100);
    } while (tempC == 85.0 || tempC == (-127.0));
  }



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
  
  boolean controlFlag = false;
  String chip_id = String(ESP.getChipId());
  String confirmPath = "";
  String confirmPayload = "";
  String local_ip_str = "";

  String error_path = "";

  static uint32_t MQTTtick = 0;
  static uint32_t MQTTlimit = 500;

// -- MQTT server setup
  #include <PubSubClient.h>
  IPAddress server(192, 168, 1, 160);

  #define BUFFER_SIZE 100

  WiFiClient espClient;
  PubSubClient client(espClient, server, 1883);
  boolean sendConfirm = false;

// wait time for displaying NTP
  static uint32_t tick = 0;
  static uint32_t tickLimit = 30000;


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



  // -- MQTTtick and MQTTlimit are here so that a massive flood of MQTT messages don't overwhelm the chip and cause it to restart.
  // -- But it doesn't really work that well. Increasing the limit time only helps slightly. Ideally, something external would introduce 
  // -- a 200ms - 300ms delay between consecutive messages.
  String topic;
  String payload;
  void callback(const MQTT::Publish& pub) {
    yield();
    client.loop();

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
        controlFlag = false;
        for (i=1; i<maxitems; i++) {
          String chunk = getValue(topic, '/', i);
          if (chunk == "control") {
            commandLoc = i;
            command = chunk;
            deviceName = getValue(topic, '/', i + 1);
            endPoint = getValue(topic, '/', i + 2);
            controlFlag = true;
            break;
          }
        }

        if (controlFlag) {

        //Serial.println("device and endpoint incoming...");
        //Serial.println(deviceName);
        //Serial.println(endPoint);

          // send endpoint_key to function stored in namepins.h at compile time
          // function returns static_endpoint_id associated with that endpoint

          String lookup_val = lookup(endPoint);
          //Serial.println("looking value incoming...");
          //Serial.println(lookup_val);

          // sketch acts on that value as it normally would, using the static_endpoint_id to know for sure what it should do (turn output pin on/off, adjust RGB light, etc)
          if (lookup_val == "blink") {
            // deserialize payload, get valueKey
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            if (findValue == "true") {
              Serial.end();
              pinMode(BUILTIN_LED, OUTPUT);
              ticker.attach(0.6, tick_fnc);
            }
            else if (findValue == "false") {
              ticker.detach();
              digitalWrite(BUILTIN_LED, HIGH);
              pinMode(BUILTIN_LED, INPUT);
              Serial.begin(115200);
            }

          }
          
          else if (lookup_val == "servoOpen") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            Serial.println("servoOpen");


            selectedOpen = findValue.toInt();

          }
          else if (lookup_val == "servoClose") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            Serial.println("servoClose");
            
            
            selectedClose = findValue.toInt();

          }
/*          else if (lookup_val == "servoToggle") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            Serial.println("servoToggle");

            selectedServo = 0;

            if (findValue == "true") {
              selectedPos = selectedOpen;
            }
            else if (findValue == "false") {
              selectedPos = selectedClose;
            }

            servoFlag = true;
          }*/

         else if (lookup_val == "servoToggleZero") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            selectedServo = 0;
            servoFlag = true;

            if (findValue == "true") {
              selectedPos = selectedOpen;
            }
            else if (findValue == "false") {
              selectedPos = selectedClose;
            }
          }

         else if (lookup_val == "servoToggleOne") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            selectedServo = 1;
            servoFlag = true;

            if (findValue == "true") {
              selectedPos = selectedOpen;
            }
            else if (findValue == "false") {
              selectedPos = selectedClose;
            }
          }

         else if (lookup_val == "servoToggleTwo") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            selectedServo = 2;
            servoFlag = true;

            if (findValue == "true") {
              selectedPos = selectedOpen;
            }
            else if (findValue == "false") {
              selectedPos = selectedClose;
            }
          }

          
          else if (lookup_val == "fan") {
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            Serial.println("fan");


              selectedServo = 3;

            if (findValue == "true") {
              // write to selectedFanSpeed 
              fanToggle = true;
              yield();

              Serial.println("fan on");
              pulselen = map(selectedFanSpeed, 0, 180, 0, 4096);


              Serial.printf("pulse .. %d \n", pulselen);

              pwm.setPWM(selectedServo, 0, pulselen);
            }
            else if (findValue == "false") {
              // set speed to zero
              fanToggle = false;
              yield();

              Serial.println("fan off");
              pwm.setPWM(selectedServo, 0, 0);
            }

          }
          else if (lookup_val == "fanSpeed") {

            Serial.println("fanSpeed");

            selectedServo = 3;

            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);  
            selectedFanSpeed = findValue.toInt();

            if (fanToggle) {

              pulselen = map(selectedFanSpeed, 0, 180, 0, 4096);

              Serial.printf("pulse .. %d \n", pulselen);

              pwm.setPWM(selectedServo, 0, pulselen);

            }

          
          }
         


          // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]
          yield();

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

  DS18B20.begin();

  //Wire.begin(4, 5);

  pwm.begin();
  pwm.setPWMFreq(50);  
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
    else {
      client.loop();  
    }



  


  if (servoFlag) {
    // call function: selectedServo, selectedPos
    servoFlag = false;
    Serial.println("servoRun");
    servoRun();
  }


  // Do things every tickLimit seconds
  if ( millis() - tick > tickLimit) {
    tick = millis();

    counter += 1;

    // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]
    yield();
    client.loop();

    getTemperature();

    confirmPath = "";
    confirmPath = thisDevicePath;
    confirmPath += "/confirm/";
    confirmPath += thisDeviceName;
    confirmPath += "/";
    confirmPath += "temperature";

    confirmPayload = "{\"update\": {\"labels\":[";
    confirmPayload += String(counter);
    confirmPayload += "],\"series\":[[";
    confirmPayload += String(tempF);
    confirmPayload += "]]}}";


    t = millis(); 
    client.publish(MQTT::Publish(confirmPath, confirmPayload).set_qos(2));
    Serial.printf("Publish took %dms\n", millis() - t);

    t = millis();
    client.loop();
    Serial.printf("Loop took %dms\n", millis() - t);



    /*
    // i2c scan
    byte error, address;
    int nDevices;
   
    Serial.println("Scanning...");
   
    nDevices = 0;
    for(address = 1; address < 127; address++ ) 
    {
   
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
   
      if (error == 0)
      {
        Serial.print("I2C device found at address 0x");
        if (address<16) 
          Serial.print("0");
        Serial.print(address,HEX);
        Serial.println("  !");
   
        nDevices++;
      }
      else if (error==4) 
      {
        Serial.print("Unknow error at address 0x");
        if (address<16) 
          Serial.print("0");
        Serial.println(address,HEX);
      }    
    }
    if (nDevices == 0)
      Serial.println("No I2C devices found\n");
    else
      Serial.println("done\n");
    */















  }


  yield();

}


// add 2 more servos and save as separate sketch