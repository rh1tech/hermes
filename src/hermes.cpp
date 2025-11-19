/*
   Hermes - Serial to Wi-Fi Modem Emulator
   Copyright (C) 2024 Mikhail Matveev (@rh1tech)
   Based on https://github.com/ssshake/vintage-computer-wifi-modem
   Copyright (C) 2020 Richard Bettridge (@theoldnet)
   Licensed under GPL v3 GNU General Public License version 3
   https://www.gnu.org/licenses/gpl-3.0.html
*/

#include <Arduino.h>
#include "globals.h"

void restoreCommandModeIfDisconnected();

void setup()
{
  // Blink LED on startup (3 times)
  pinMode(16, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(16, HIGH);
    delay(200);
    digitalWrite(16, LOW);
    delay(200);
  }
  
  // CRITICAL: GPIO15 (SD card CS) is a boot mode pin on ESP8266!
  // GPIO15 must be LOW during boot for the ESP8266 to boot from flash
  // If SD card holds it HIGH, the ESP8266 won't boot properly
  
  // Immediately set pin 15 as OUTPUT and pull LOW to ensure proper boot state
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  delay(10);
  
  // Then set it HIGH (CS idle state for SD card)
  digitalWrite(15, HIGH);
  delay(50);
  
  #if NAPT_SUPPORTED
    ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  #endif
  EEPROM.begin(LAST_ADDRESS + 1);
  delay(10);
  readSettings();
  serialSetup();
  waitForFirstInput();
  welcome();
  delay(500); // Give system time to stabilize before SD init
  initSDCard();
  wifiSetup();
  webserverSetup();
}

void loop()
{
  if (firmwareUpdating == true)
  {
    handleOTAFirmware();
    return;
  }
  handleFlowControl();
  handleWebServer();  
  if (tcpServer.hasClient())
  {
    handleIncomingConnection();
  }
  if (cmdMode == true)
  {
    handleCommandMode();
  }
  else
  {
    handleConnectedMode();
  }
  restoreCommandModeIfDisconnected();
}
