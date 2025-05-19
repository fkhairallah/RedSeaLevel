#include <RedGlobals.h>
Preferences prefs; // preferences library

void resumeTideUpdate()
{
  console.println("Resuming tide update");
  //tideUpdateTicker.attach(TIDE_UPDATE_INTERVAL, getNOAAtideData);
}

void pauseTideUpdate()
{
  console.println("Pausing tide update");
  //tideUpdateTicker.detach();
}

void setup()
{

  // initialize preferences library
  prefs.begin(myHostName, false); // false:: read/write mode
  debugMode = prefs.getBool("debugMode");
  // prefs.clear();    // clear all parameters

  // setup Console
  setupConsole();


  configureWIFI(); // configure wifi
  configureMQTT();


}

void loop()
{
  // This should be the first line in loop();
  checkConnection(); // check WIFI connection & Handle OTA
  handleConsole();   // handle any commands from console

  // Work to be done, but only if we aren't updating the software
  if (!otaInProgress)
  {

    checkMQTTConnection(); // check MQTT

    delay(100);
  }
}