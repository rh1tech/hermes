#include <Arduino.h>
#include "globals.h"

/**
   Turn on the LED and store the time, so the LED will be turned off shortly after.
*/
void led_on()
{
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  ledTime = millis();
}

void handleLEDState()
{
  // Turn off tx/rx led if it has been lit long enough to be visible
  if (millis() - ledTime > LED_TIME)
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // toggle LED state
}
