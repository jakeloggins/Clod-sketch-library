#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// for board and platform type
#include <ESPBoardDefs.h>
  // includes String board_type and String platform


//for LED status
#include <Ticker.h>
Ticker ticker;

// MQTT lib
#include <PubSubClient.h>

// MQTT server setup
#define BUFFER_SIZE 100

// Arduino OTA
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

WiFiClient espClient;
IPAddress default_server(192, 168, 1, 160);
PubSubClient client(espClient, default_server);
boolean sendConfirm = false;

String chip_id = String(ESP.getChipId());


String getIP(String data, char separator, int index)
{
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


void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}


//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "192.168.1.160";
char mqtt_port[6] = "1883";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  //Serial.println("Should save config");
  shouldSaveConfig = true;
}


String getValue(String data, char separator, int index)
{
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

String topic;
String payload;

void callback(const MQTT::Publish& pub) {
  yield();

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
    command = getValue(topic, '/', 2);
    deviceName = getValue(topic, '/', 3);

    if ((deviceName == chip_id) && (command == "control")) {
      if (payload == "blink on") {
        Serial.end();
        pinMode(BUILTIN_LED, OUTPUT);
        ticker.attach(0.6, tick);
      }
      else if (payload == "blink off") {
        ticker.detach();
        digitalWrite(BUILTIN_LED, HIGH);
        pinMode(BUILTIN_LED, INPUT);
        Serial.begin(115200);
      }
    }

  }

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("setup");

  // start ticker with 0.5 because we start in AP mode and try to connect
  //ticker.attach(0.6, tick);

  // --------------------
  //clean FS, for testing
    SPIFFS.format();
  // -------------------

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          //strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read


  Serial.println("after FS...");


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  //WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  int randIP = random(1,255);

  //set static ip
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,randIP), IPAddress(192,168,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  //wifiManager.addParameter(&custom_blynk_token);

  // ----------------------------
  //reset settings - for testing
    //wifiManager.resetSettings();
  // -----------------------------


  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();


  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Clod", "!ClodMQTT!")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //ticker.detach();
  //keep LED on
  //digitalWrite(BUILTIN_LED, LOW);

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  //strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    //json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  String local_ip_str = WiFi.localIP().toString();


  // break down the mqtt_server to integers
    String mqtt_server_str = String(mqtt_server);
    String firstIP = getIP(mqtt_server_str, '.', 0);
    String secondIP = getIP(mqtt_server_str, '.', 1);
    String thirdIP = getIP(mqtt_server_str, '.', 2);
    String fourthIP = getIP(mqtt_server_str, '.', 3);

    int one_IP = firstIP.toInt();
    int two_IP = secondIP.toInt();
    int three_IP = thirdIP.toInt();
    int four_IP = fourthIP.toInt();

  // create the custom server IP address
    IPAddress custom_mqtt_ip = IPAddress(one_IP, two_IP, three_IP, four_IP);

  int mqtt_port_int = atoi(mqtt_port);

  // debug
    /*Serial.println(default_server);
    Serial.println(custom_mqtt_ip);
    Serial.println(mqtt_port);
    Serial.println(mqtt_port_int);
    Serial.println(ESP.getChipId());
    Serial.println(ESP.getFlashChipId());
    Serial.println(ESP.getFlashChipSize());
    Serial.println(ESP.getFlashChipRealSize());
    Serial.println(ESP.getFlashChipSpeed());

    Serial.println(ESP.getVcc());


    Serial.println(ESP.getBootVersion());
    Serial.println(ESP.getCpuFreqMHz());
    */




    Serial.println("...connect attempt, check /uploader/confirm on mqtt server...");
    //Serial.println(client.connect("espClient"));


  // set MQTT client to user entered server and send confirm message

  // send confirm message example
    /*{
      "16013813": {
      "current_ip": "192.168.1.141",
      "type": "esp",
        "espInfo": {
          "chipID": "16013813",
          "board_type": "esp01_1m",
          "platform": "espressif",
          "flash_size": 1048576,
          "real_size": 1048576,
          "boot_version": 4
        }
      }
    }*/


  String confirm_msg = "{ \"";
  confirm_msg += chip_id;
  confirm_msg += "\": { \"current_ip\": \"";
  confirm_msg += local_ip_str;
  confirm_msg += "\", \"type\": \"esp\",";
  confirm_msg += "\"espInfo\": { ";

    confirm_msg += "\"chipID\": \"";
    confirm_msg += chip_id;
    confirm_msg += "\", \"board_type\": \"";
    confirm_msg += board_type; // hardcoded from ESPBoardDefs.h

    confirm_msg += "\", \"platform\": \"";
    confirm_msg += platform; // hardcoded from ESPBoardDefs.h
    confirm_msg += "\", \"flash_size\": ";
    confirm_msg += String(ESP.getFlashChipSize());
    confirm_msg += ", \"real_size\": ";
    confirm_msg += String(ESP.getFlashChipRealSize());
    confirm_msg += ", \"boot_version\": \"";
    confirm_msg += String(ESP.getBootVersion());
    confirm_msg += "\" } } }";



  String init_control_path = "/init/control/";
  init_control_path += chip_id;

  String init_error_path = "/init/errors/";
  init_error_path += chip_id;


    // persistence.js helper script listens to /init/# to add and delete from active_init_list.json
    client.set_server(custom_mqtt_ip, mqtt_port_int);

    // configure lwt message so that successful upload or device disconnect causes helper script to remove device from active_init_list
    //client.connect("clientId", init_error_path, "disconnected");
    client.connect(MQTT::Connect(chip_id).set_will(init_error_path, "disconnected"));


    // old previously working connect statement
    //client.connect("espClient");
    client.set_callback(callback);
    client.subscribe("/deviceInfo/control/#");


    /*
    client.publish("/uploader/confirm", "esp connected");
    client.publish("/uploader/confirm/ip", local_ip_str);
    client.publish("/uploader/confirm/chipID", chip_id);
    client.publish("/uploader/confirm/flashChipID", String(ESP.getFlashChipId()));
    client.publish("/uploader/confirm/flashChipSize", String(ESP.getFlashChipSize()));
    client.publish("/uploader/confirm/flashChipRealSize", String(ESP.getFlashChipRealSize()));
    client.publish("/uploader/confirm/flashChipSpeed", String(ESP.getFlashChipSpeed()));
    client.publish("/uploader/confirm/bootVersion", String(ESP.getBootVersion()));
    client.publish("/uploader/confirm/JSON", confirm_msg);
    */

    client.publish(init_control_path, confirm_msg);

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
    Serial.println(WiFi.localIP());


}


void loop() {
  // put your main code here, to run repeatedly:

  //Serial.println("..");

  // OTA
  ArduinoOTA.handle();

  // -- MQTT connect
      if (!client.connected()) {
        Serial.println("not connected");
        if (client.connect("espClient")) {
          Serial.println("MQTT connected");
          client.publish("/uploader/confirm", "esp connected - main loop");
          client.set_callback(callback);
          client.subscribe("/deviceInfo/control/#");
        }
      }

      if (client.connected()) {
        client.loop();
      }


}
