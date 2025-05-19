#include <RedGlobals.h>
//#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
//#include <SPIFFS.h>
#include <ArduinoOTA.h>

bool otaInProgress; // flags if OTA is in progress
int secondsWithoutWIFI = 0; // counter the seconds without wifi


// configuration parameters
// Hostname, AP name & MQTT clientID
// define your default values here, if there are different values in config.json, they are overwritten.
// length should be max size + 1
char myHostName[64] = "RedTide";
char deviceLocation[64] = "shelf";
char mqttServer[64] = "Carbon.local";
char mqttPort[16] = "1883";
char mqttUser[64] = "";
char mqttPwd[64] = "";
char topLED[16] = "48";
char bottomLED[16] = "64";
char NoaaStation[16] = NOAA_DEFAULT_STATION;

// The extra parameters to be configured (can be either global or just in the setup)
// After connecting, parameter.getValue() will get you the configured value
// id/name placeholder/prompt default length
WiFiManagerParameter custom_deviceLocation("location", "Device Location", deviceLocation, 64);
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqttPort, 5);
WiFiManagerParameter custom_top_led("topLED", "Top LEDs", topLED, 16);
WiFiManagerParameter custom_bottom_led("bottomLED", "Bottom LEDs", bottomLED, 16);
WiFiManagerParameter custom_noaa_station("NoaaStation", "Noaa Station", NoaaStation, 16);

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}
/*
 * ********************************************************************************

 * ********************************************************************************
*/

void configureWIFI()
{

    // clean FS, for testing
    // SPIFFS.format();

    
    // read configuration from FS json
    //readConfigFromDisk();
    //savePreferences();
    // get configuration from NVM
    readPreferences();



    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);


    // add all your parameters here
    wifiManager.addParameter(&custom_deviceLocation);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_top_led);
    wifiManager.addParameter(&custom_bottom_led);
    wifiManager.addParameter(&custom_noaa_station);

    // reset settings - for testing
    // wifiManager.resetSettings();

    // set minimu quality of signal so it ignores AP's under that quality
    // defaults to 8%
    wifiManager.setMinimumSignalQuality();

    // in seconds how long to wait for a wifi connection
    wifiManager.setConnectTimeout(60);

    // sets timeout until configuration portal gets turned off
    // useful in power failure cases
    wifiManager.setConfigPortalTimeout(180);

    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("AutoConnectAP", "password"))
    {
        console.println("failed to connect and hit timeout");
        delay(3000);
        // reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }
    
    // we place a timeout which mean if no one connect to the hotspot
    // we will restart the ESP32 so it has a chance to reconnect to
    // regular wifi. This deals with power outage situtions where
    // the ESP32 might come up before the WIFI router
    if (WiFi.status() != WL_CONNECTED) {
      console.println("no one connected to the wifi so I'm restarting...");
      delay(3000);
      ESP.restart();
    }

   

    // if you get here you have connected to the WiFi
    if (debugMode) console.println("connected...yeey! Enabling telnet:)");
    console.enableTelnet(23);

    // // read updated parameters
    // strcpy(mqttServer, custom_mqtt_server.getValue());
    // strcpy(mqttPort, custom_mqtt_port.getValue());
    // strcpy(api_token, custom_api_token.getValue());

    // in this program start the config portal every time.
    //wifiManager.startConfigPortal();

    // and OTA
    configureOTA(myHostName);

    // save the custom parameters to FS
    if (shouldSaveConfig) {
        //writeConfigToDisk();
        savePreferences();
    }

    if (debugMode) console.printf("local ip: "); console.println(WiFi.localIP());

}
/*
 * ********************************************************************************
 * This routine will check the Wifi status, and reset the ESP is unable to connect
 *
 * If all is well it initiates mDNS service & OTA process
 *
 * ********************************************************************************
 */

void checkConnection()
{
    // THIS NEEDS TO BE THE FIRST ITEM IN LOOP
    ArduinoOTA.handle();

    if (WiFi.status() != WL_CONNECTED) // reconnect wifi
    {
        console.println("Not connected to WIFI.. give it ~10 seconds.");
        delay(1000);
        if (secondsWithoutWIFI++ > 30)
        {
          ESP.restart();
          delay(5000);
        }
    }
    else
    {
      secondsWithoutWIFI = 0;
    }

    //MDNS.update(); // and refresh mDNS

}
/*
 * ********************************************************************************

 read device configuration from config.json in the file system

 * ********************************************************************************
*/
// void readConfigFromDisk()
// {
//     if (debugMode) console.println("Reading config...");

//     if (SPIFFS.begin())
//     {
//         if (SPIFFS.exists("/config.json"))
//         {
//             // file exists, reading and loading
//             File configFile = SPIFFS.open("/config.json", "r");
//             if (configFile)
//             {
//                 size_t size = configFile.size();
//                 // Allocate a buffer to store contents of the file.
//                 std::unique_ptr<char[]> buf(new char[size]);

//                 configFile.readBytes(buf.get(), size);
//                 DynamicJsonDocument json(1024);
//                 auto deserializeError = deserializeJson(json, buf.get());
//                 serializeJson(json, Serial);
//                 if (!deserializeError)
//                 { 
//                   if (json.containsKey("deviceLocation"))  strcpy(deviceLocation, json["deviceLocation"]);
//                   if (json.containsKey("mqtt_server"))  strcpy(mqttServer, json["mqtt_server"]);
//                   if (json.containsKey("mqtt_port"))    strcpy(mqttPort, json["mqtt_port"]);
//                   if (json.containsKey("topLED"))  strcpy(topLED, json["topLED"]);
//                   if (json.containsKey("bottomLED"))  strcpy(bottomLED, json["bottomLED"]);
//                   if (json.containsKey("NoaaStation")) strcpy(NoaaStation, json["NoaaStation"]);
//                 }
//                 else
//                 {
//                     console.println("failed to load json config");
//                 }
//             }
//         }
//     }
//     else
//     {
//         console.println("failed to mount FS");
//     }
//     // end read
// }
void readPreferences() {
  if (prefs.isKey("deviceLocation"))     strcpy(deviceLocation, prefs.getString("deviceLocation").c_str());
  if (prefs.isKey("mqtt_server"))    strcpy(mqttServer, prefs.getString("mqtt_server").c_str());
  if (prefs.isKey("mqtt_port"))    strcpy(mqttPort, prefs.getString("mqtt_port").c_str());
  if (prefs.isKey("topLED"))    strcpy(topLED, prefs.getString("topLED").c_str());
  if (prefs.isKey("bottomLED"))    strcpy(bottomLED, prefs.getString("bottomLED").c_str());
  if (prefs.isKey("NoaaStation"))    strcpy(NoaaStation, prefs.getString("NoaaStation").c_str());
}
/*
 * ********************************************************************************

 updates the configuration in config.json in the file system

 * ********************************************************************************
*/
// void writeConfigToDisk()
// {
//     if (debugMode) console.println("saving config");
//     DynamicJsonDocument json(1024);
//     json["deviceLocation"] = deviceLocation;
//     json["mqtt_server"] = mqttServer;
//     json["mqtt_port"] = mqttPort;
//     json["topLED"] = topLED;
//     json["bottomLED"] = bottomLED;
//     json["NoaaStation"] = NoaaStation;

//     File configFile = SPIFFS.open("/config.json", "w");
//     if (!configFile)
//     {
//         console.println("failed to open config file for writing");
//     }

//     serializeJson(json, Serial);
//     serializeJson(json, configFile);
//     configFile.close();
//     // end save
// }

void savePreferences()
{
  prefs.putString("deviceLocation",String(deviceLocation));
  prefs.putString("mqtt_server", String(mqttServer));
  prefs.putString("mqtt_port", String(mqttPort));
  prefs.putString("topLED", String(topLED));
  prefs.putString("bottomLED", String(bottomLED));
  prefs.putString("NoaaStation", String(NoaaStation));
  if (debugMode) 
    console.println("Preferences saved!");
}

/*
 * ********************************************************************************

  Reset all configuration paramters.

 * ********************************************************************************
*/
void resetConfiguration()
{
    WiFiManager wm;
    wm.resetSettings();
}


/*
 * ********************************************************************************
 * 
 * ********************************************************************************
 */
void configureOTA(char *hostName)
{
  // configure OTA
  otaInProgress = false; // used to stop other activities during OTA update

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(hostName);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    console.println("OTA Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    console.println("\nOTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char buffer[100];
    sprintf(buffer, "OTA Progress: %u%%\r", (progress / (total / 100)));
    console.println(buffer);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    char buffer[100];
    sprintf(buffer, "OTA Error[%u]: ", error);
    console.println(buffer);
    if (error == OTA_AUTH_ERROR)
    {
      console.println("OTA Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      console.println("OTA Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      console.println("OTA Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      console.println("OTA Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      console.println("OTA End Failed");
    }
  });

  ArduinoOTA.begin();
}
