#include <Arduino.h>
#include "globals.h"

/**
   Arduino main init function
*/
void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // off
  pinMode(FLASH_BUTTON, INPUT);
  digitalWrite(FLASH_BUTTON, HIGH);
  // the esp fork of LWIP doesn't automatically init when enabling nat, so just do it in setup
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  // why? was this part of the eeprom upgrade code or not, it preceeded the comments for it
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

/**
   Arduino main loop function
*/
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
  // No idea what this is
  // New unanswered incoming connection on server listen socket
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
