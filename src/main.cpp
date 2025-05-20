#include <RedGlobals.h>
Preferences prefs; // preferences library

// Ultrasonic sensor data
float current_level = 0.0; // running average of distance measurement in cm
int sample_count = 0;    // number of samples in current_level
Ticker tideUpdateTicker;
Ticker mqttPublishTicker;

// Function to measure distance using HC-SR04 and update running average
void measureDistanceAndUpdateAverage() {
  long duration;
  float distance_cm;

  // Clears the TRIGP_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Sets the TRIGP_PIN on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  // Timeout is 1 second by default. Max range ~400cm => ~23300 us.
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm
  // Speed of sound wave = 343 m/s = 0.0343 cm/us
  // Distance = (duration * speed_of_sound) / 2 (for round trip)
  distance_cm = duration * 0.0343 / 2.0;

  // Basic filtering for plausible values (HC-SR04 typical range 2cm to 400cm)
  if (distance_cm > 1.0 && distance_cm < 450.0) { // Adjusted lower bound slightly
    if (sample_count == 0) {
      current_level = distance_cm;
    } else {
      // Update running average
      current_level = (current_level * sample_count + distance_cm) / (sample_count + 1.0);
    }
    sample_count++;
    if (debugMode) {
      console.printf("Measured distance: %.2f cm, Current avg: %.2f cm, Samples: %d\n", distance_cm, current_level, sample_count);
    }
  } else {
    if (debugMode) {
      console.printf("Distance out of range or error: %.2f cm (duration: %ld us)\n", distance_cm, duration);
    }
  }
}

// Function to publish the average level to MQTT and reset average
void publishAverageLevel() {
  if (otaInProgress || !mqtt_client.connected()) {
    if (debugMode) {
        console.println("OTA in progress or MQTT not connected, skipping MQTT publish of level.");
    }
    // Do not reset average if publish is skipped, try again next interval
    return;
  }

  if (sample_count > 0) {
    // our seawall, where the measurement is taking place, is, basically, at 0 NAVD88
    // convert to feet and change offset to MLLW
    float current_level_mllw = SEAWALL_MLLW_OFFSET - (current_level * 0.0328084);   // NAVD88 to MLLW conversion
    publishLevel(current_level_mllw);                                             // Publish the average level
    if (debugMode) {
      console.printf("Publishing average %f ft to MQTT\n", current_level_mllw);
    }

    // Reset for the next interval ("process restarts")
    current_level = 0.0;
    sample_count = 0;
  } else {
    if (debugMode) {
      console.println("No valid samples collected in this interval, not publishing to MQTT.");
    }
    // Reset even if no samples, to ensure a clean start for the next interval
    current_level = 0.0;
    sample_count = 0;
  }
}

void resumeTideUpdate()
{
  console.println("Resuming tide measurement and MQTT publishing.");
  // Reset average and count when resuming to start fresh for the new period of activity
  current_level = 0.0;
  sample_count = 0;
  tideUpdateTicker.attach_ms(TIDE_UPDATE_INTERVAL, measureDistanceAndUpdateAverage);
  mqttPublishTicker.attach_ms(MQTT_UPDATE_INTERVAL, publishAverageLevel);
}

void pauseTideUpdate()
{
  console.println("Pausing tide measurement and MQTT publishing.");
  tideUpdateTicker.detach();
  mqttPublishTicker.detach();
}

void setup()
{
  // Setup sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW); // Ensure trigger pin is low initially

  // initialize preferences library
  prefs.begin(myHostName, false); // false:: read/write mode
  debugMode = prefs.getBool("debugMode");
  // prefs.clear();    // clear all parameters

  // setup Console
  setupConsole();


  configureWIFI(); // configure wifi
  configureMQTT(); // configure MQTT (this also calls configureTopics() which sets up mqtt_level)

  // Start the measurement and publishing tasks.
  resumeTideUpdate();
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
    // Tickers handle their own timing for sensor reads and MQTT publishes.
    delay(100);
  }
}