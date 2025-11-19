#include <Arduino.h>
#include "globals.h"

#ifdef ESP32
  #include <EEPROM.h>
  #define EEPROM_SIZE 512  // Adjust size as needed
#elif defined(ESP8266)
  #include <EEPROM.h>
#endif

void serialSetup()
{
  #ifdef ESP32
    EEPROM.begin(EEPROM_SIZE);
  #elif defined(ESP8266)
    EEPROM.begin(512);
  #endif

  // NOTE: CTS_PIN (15) is shared with SD card CS pin
  // Don't initialize it here to avoid SD card conflicts
  // pinMode(CTS_PIN, OUTPUT);
  // digitalWrite(CTS_PIN, HIGH);
  setCarrierDCDPin(false);
  
  // Fetch baud rate from EEPROM
  serialspeed = EEPROM.read(BAUD_ADDRESS);
  
  // Check if it's out of bounds
  if (serialspeed < 0 || serialspeed > sizeof(bauds) / sizeof(bauds[0]))
  {
    serialspeed = 0;
  }
  Serial.begin(bauds[serialspeed]);
}

void setBaudRate(int inSpeed)
{
  if (inSpeed == 0)
  {
    sendResult(RES_ERROR);
    return;
  }
  int foundBaud = -1;
  for (size_t i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++)
  {
    if (inSpeed == bauds[i])
    {
      foundBaud = i;
      break;
    }
  }
  if (foundBaud == -1)
  {
    sendResult(RES_ERROR);
    return;
  }
  if (foundBaud == serialspeed)
  {
    sendResult(RES_OK);
    return;
  }
  Serial.print("Switching serial port to ");
  Serial.print(inSpeed);
  Serial.println(" in 5 seconds...");
  delay(5000);
  Serial.end();
  delay(200);
  Serial.begin(bauds[foundBaud]);
  serialspeed = foundBaud;
  delay(200);
  sendResult(RES_OK);
}
