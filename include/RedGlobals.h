

#ifndef RED_GLOBALS_H
#define RED_GLOBALS_H
#include <dConsole.h>
#include <Ticker.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <Preferences.h>

#define VERSION "V2.4" // N.B: document changes in README.md

// include Pins
#ifndef _PINS_H
#include <pins.h>
#endif

#define MQTT_TOPIC_PREFIX "sealevel" // prefix for all MQTT topics

// tide data
#define NOAA_BASE_URL "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?product=predictions&application=NOS.COOPS.TAC.WL&datum=MSL&time_zone=lst_ldt&units=english&interval=hilo&format=json"
#define NOAA_DEFAULT_STATION "8722718"
#define TIDE_UPDATE_INTERVAL 900000L    // 500s = 8.3 min, 900 = 15 min

// in main
void pauseTideUpdate();
void resumeTideUpdate();

// in WIFIConfig
extern char myHostName[];
extern char deviceLocation[];
extern char mqttServer[];
extern char mqttPort[];
extern char mqttUser[];
extern char mqttPwd[];
extern char topLED[]; // nunber of leds in the top string
extern char bottomLED[]; // nunber of leds in the bottom string
extern char NoaaStation[];
extern bool otaInProgress;  // stop doing stuff if we are uploading software
extern Preferences prefs;   // used to save preferences to NVM

void configureWIFI();
void checkConnection ();
//void readConfigFromDisk();
//void writeConfigToDisk();
void resetConfiguration();
void readPreferences();
void savePreferences();
void configureOTA(char *);


// in console.ino
extern dConsole console;
void setupConsole();
void handleConsole();

// in MQTTConfig
extern bool debugMode;
void configureMQTT();
bool checkMQTTConnection();
void mqttDisconnect();
void mqttCallback(char *topic, byte *payload, unsigned int length);



#endif