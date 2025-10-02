#include <Arduino.h>
#include "globals.h"

void serialSetup()
{
  pinMode(CTS_PIN, OUTPUT); // This was set to input, but the diagrams I reviewed indicate a modem's CTS pin is output
  digitalWrite(CTS_PIN, HIGH); // Pull up the CTS pin
  setCarrierDCDPin(false);
  // Fetch baud rate from EEPROM
  serialspeed = EEPROM.read(BAUD_ADDRESS);
  // Check if it's out of bounds -- we must ensure communication is possible
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
    sendResult(R_ERROR);
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
    sendResult(R_ERROR);
    return;
  }
  if (foundBaud == serialspeed)
  {
    sendResult(R_OK);
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
  sendResult(R_OK);
}
