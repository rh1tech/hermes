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

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // off
  pinMode(FLASH_BUTTON, INPUT);
  digitalWrite(FLASH_BUTTON, HIGH);
  // The ESP fork of LWIP doesn't automatically initialize when enabling NAT, so we initialize it in setup
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  // Part of the EEPROM upgrade code or not, it preceded the comments for it
  EEPROM.begin(LAST_ADDRESS + 1);
  delay(10);
  eepromUpgradeToDeprecate();
  readSettings();
  serialSetup();
  // waitForFirstInput();
  welcome();
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
  checkButton();
  // Handle new unanswered incoming connection on the server listen socket
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
  handleLEDState();
}
