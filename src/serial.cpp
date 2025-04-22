#include <Arduino.h>
#include "globals.h"

void serialSetup()
{

  //  pinMode(DCD_PIN, OUTPUT);
  // Research
  // 7 RTS is High from the computer at 9v on other modem
  // 8 CTS is high from modem as either output high or input high, not sure

  // Consider disabling/ignoring this. PC is going to say RTS high, but do we care since flow control isn't actually implemented in code yet
  //   pinMode(RTS_PIN, INPUT); //this was set to output but the diagrams I look at seem to say a modem's rts pin is input
  //   digitalWrite(RTS_PIN, LOW); // ready to receive data // disabled this because it's input, maybe this is wrong

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
  // requested baud rate not found, return error
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
  Serial.print("SWITCHING SERIAL PORT TO ");
  Serial.print(inSpeed);
  Serial.println(" IN 5 SECONDS");
  delay(5000);
  Serial.end();
  delay(200);
  Serial.begin(bauds[foundBaud]);
  serialspeed = foundBaud;
  delay(200);
  sendResult(R_OK);
}
