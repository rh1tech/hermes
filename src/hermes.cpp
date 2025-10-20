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
  #if NAPT_SUPPORTED
    ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  #endif
  EEPROM.begin(LAST_ADDRESS + 1);
  delay(10);
  readSettings();
  serialSetup();
  waitForFirstInput();
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
