/**********************************************************************************
 * 
 *    Implements functionality for telnet and serial console
 * 
 *********************************************************************************/
#include <RedGlobals.h>


dConsole console;

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    console.println("Failed to obtain time");
    return;
  }
  console.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  console.printf(" DST=%s\r\n", timeinfo.tm_isdst?"TRUE":"FALSE");
}

/*
 * ********************************************************************************

  ********************  CUSTOMIZABLE SECTION  ***************************

 * ********************************************************************************
*/
#define CUSTOM_COMMANDS "Custom Commands: status, on, off, test, noaa"

void executeCustomCommands(char* commandString,char* parameterString)
{
  if (strcmp(commandString, "on") == 0)     resumeTideUpdate();
  if (strcmp(commandString, "off") == 0)    pauseTideUpdate();
  if (strcmp(commandString, "status") == 0)
  {

    printLocalTime();
    console.printf("Prefs %s MQTT=%s #%s, NOAA %s\r\n", prefs.getString("deviceLocation"), prefs.getString("mqtt_server"), prefs.getString("mqtt_port"), prefs.getString("NoaaStation"));
    console.printf("MQTT %s %s\r\n", mqttServer, mqttPort);
  }


  if (strcmp(commandString, "test") == 0)
  {
  }


  if (strcmp(commandString, "noaa") == 0) {
    strcpy(NoaaStation, parameterString);
    savePreferences();
    console.printf("NOAA station changed to %s\r\n", NoaaStation);
  }
}

/*
 * ********************************************************************************

    ********************  END OF CUSTOMIZABLE SECTION  ***************************

 * ********************************************************************************
*/

void setupConsole()
{
  console.enableSerial(&Serial, true);
  //console.enableTelnet(23);
}


void handleConsole()
{
  // console
  if (console.check())
  {
    executeCustomCommands(console.commandString, console.parameterString);

    if (strcmp(console.commandString, "?") == 0)
    {
      console.print("[RED]");
      console.print(MQTT_TOPIC_PREFIX);
      console.print(" ");
      console.println(VERSION);
      console.printf("Host: %s @", myHostName);
      console.println(WiFi.localIP().toString());
      console.printf("MQTT Server %s, port: %s, %s\r\n", mqttServer, mqttPort, deviceLocation);
      console.println("Commands: ?, debug, reset (Factory), reboot, quit");
      console.println(CUSTOM_COMMANDS);
    }
    if (strcmp(console.commandString, "debug") == 0)
    {
      debugMode = !debugMode;
      console.print("Debug mode is now ");
      console.println(debugMode);
      prefs.putBool("debugMode", debugMode);
      savePreferences();
    }
    if (strcmp(console.commandString, "location") == 0)
    {
      strcpy(deviceLocation, console.parameterString);
      savePreferences();

      console.printf("location changed to %s\r\n", deviceLocation);
      console.println("Change will take effect after next reboot");
    }
    if (strcmp(console.commandString, "mqtt") == 0)
    {
      strcpy(mqttServer, console.parameterString);
      savePreferences();
      console.print("MQTT server changed to ");
      console.println(mqttServer);
      mqttDisconnect();
    }
    if (strcmp(console.commandString, "reset") == 0)
    {
      console.print("Reseting configuration...");
      resetConfiguration();
      console.println(" Done.");
    }
    if (strcmp(console.commandString, "reboot") == 0)
    {
      console.print("Rebooting...");
      delay(200);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
    if ((strcmp(console.commandString, "exit") == 0) || (strcmp(console.commandString, "quit") == 0))
    {
      console.print("quiting...");
      console.closeTelnetConnection();
      delay(500);
      console.println("");
    }

    console.print("[RED]> ");
  }
}
