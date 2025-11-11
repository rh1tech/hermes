#include <Arduino.h>

#ifdef ESP32
  #include <EEPROM.h>
  #define EEPROM_SIZE 512
#elif defined(ESP8266)
  #include <EEPROM.h>
#else
  #error "Unsupported platform. Please define ESP32 or ESP8266"
#endif
#include "globals.h"
#include <EEPROM.h>

String getEEPROM(int startAddress, int len);
void setEEPROM(String inString, int startAddress, int maxLen);

String getEEPROM(int startAddress, int len)
{
  String myString;
  for (int i = startAddress; i < startAddress + len; i++)
  {
    if (EEPROM.read(i) == 0x00)
    {
      break;
    }
    myString += char(EEPROM.read(i));
  }
  return myString;
}

void setEEPROM(String inString, int startAddress, int maxLen)
{
  for (size_t i = static_cast<size_t>(startAddress);
       i < static_cast<size_t>(startAddress) + inString.length();
       i++)
  {
    EEPROM.write(i, inString[i - startAddress]);
  }
  for (int i = inString.length() + startAddress; i < maxLen + startAddress; i++)
  {
    EEPROM.write(i, 0x00);
  }
}

void defaultEEPROM()
{
  EEPROM.write(VERSION_ADDRESS, VERSIONA);
  EEPROM.write(VERSION_ADDRESS + 1, VERSIONB);
  setEEPROM("", SSID_ADDRESS, SSID_LEN);
  setEEPROM("", PASS_ADDRESS, PASS_LEN);
  setEEPROM("d", IP_TYPE_ADDRESS, 1);
  EEPROM.write(SERVER_PORT_ADDRESS, highByte(LISTEN_PORT));
  EEPROM.write(SERVER_PORT_ADDRESS + 1, lowByte(LISTEN_PORT));
  EEPROM.write(BAUD_ADDRESS, 0x00);
  EEPROM.write(ECHO_ADDRESS, 0x01);
  EEPROM.write(AUTO_ANSWER_ADDRESS, 0x01);
  EEPROM.write(TELNET_ADDRESS, 0x00);
  EEPROM.write(VERBOSE_ADDRESS, 0x01);
  EEPROM.write(FLOW_CONTROL_ADDRESS, 0x02);
  EEPROM.write(PIN_POLARITY_ADDRESS, 0x01);
  EEPROM.write(QUIET_MODE_ADDRESS, 0x00);
  setEEPROM("theoldnet.com:23", speedDialAddresses[0], 50);
  setEEPROM("bbs.retrocampus.com:23", speedDialAddresses[1], 50);
  setEEPROM("bbs.eotd.com:23", speedDialAddresses[2], 50);
  setEEPROM("blackflag.acid.org:31337", speedDialAddresses[3], 50);
  setEEPROM("bbs.starbase21.net:23", speedDialAddresses[4], 50);
  setEEPROM("reflections.servebbs.com:23", speedDialAddresses[5], 50);
  setEEPROM("heatwavebbs.com:9640", speedDialAddresses[6], 50);
  for (int i = 5; i < 10; i++)
  {
    setEEPROM("", speedDialAddresses[i], 50);
  }
  setEEPROM("SORRY, SYSTEM IS CURRENTLY BUSY. PLEASE TRY AGAIN LATER.", BUSY_MSG_ADDRESS, BUSY_MSG_LEN);
  EEPROM.commit();
}

void readSettings()
{
  // Check if EEPROM has been initialized with valid version
  byte verA = EEPROM.read(VERSION_ADDRESS);
  byte verB = EEPROM.read(VERSION_ADDRESS + 1);
  
  // If version doesn't match, initialize EEPROM with defaults
  if (verA != VERSIONA || verB != VERSIONB) {
    defaultEEPROM();
  }
  
  echo = EEPROM.read(ECHO_ADDRESS);
  autoAnswer = EEPROM.read(AUTO_ANSWER_ADDRESS);
  ssid = getEEPROM(SSID_ADDRESS, SSID_LEN);
  password = getEEPROM(PASS_ADDRESS, PASS_LEN);
  busyMsg = getEEPROM(BUSY_MSG_ADDRESS, BUSY_MSG_LEN);
  tcpServerPort = word(EEPROM.read(SERVER_PORT_ADDRESS), EEPROM.read(SERVER_PORT_ADDRESS + 1));
  telnet = EEPROM.read(TELNET_ADDRESS);
  verboseResults = EEPROM.read(VERBOSE_ADDRESS);
  flowControl = EEPROM.read(FLOW_CONTROL_ADDRESS);
  pinPolarity = EEPROM.read(PIN_POLARITY_ADDRESS);
  quietMode = EEPROM.read(QUIET_MODE_ADDRESS);
  for (int i = 0; i < 10; i++)
  {
    speedDials[i] = getEEPROM(speedDialAddresses[i], 50);
  }
}

void writeSettings()
{
  setEEPROM(ssid, SSID_ADDRESS, SSID_LEN);
  setEEPROM(password, PASS_ADDRESS, PASS_LEN);
  setEEPROM(busyMsg, BUSY_MSG_ADDRESS, BUSY_MSG_LEN);
  EEPROM.write(BAUD_ADDRESS, serialspeed);
  EEPROM.write(ECHO_ADDRESS, byte(echo));
  EEPROM.write(AUTO_ANSWER_ADDRESS, byte(autoAnswer));
  EEPROM.write(SERVER_PORT_ADDRESS, highByte(tcpServerPort));
  EEPROM.write(SERVER_PORT_ADDRESS + 1, lowByte(tcpServerPort));
  EEPROM.write(TELNET_ADDRESS, byte(telnet));
  EEPROM.write(VERBOSE_ADDRESS, byte(verboseResults));
  EEPROM.write(FLOW_CONTROL_ADDRESS, byte(flowControl));
  EEPROM.write(PIN_POLARITY_ADDRESS, byte(pinPolarity));
  EEPROM.write(QUIET_MODE_ADDRESS, byte(quietMode));
  for (int i = 0; i < 10; i++)
  {
    setEEPROM(speedDials[i], speedDialAddresses[i], 50);
  }
  EEPROM.commit();
}

void displayStoredSettings()
{
  Serial.println("Stored Profile:");
  // yield();
  const uint8_t baudIndex = EEPROM.read(BAUD_ADDRESS);
  const uint8_t echoStored = EEPROM.read(ECHO_ADDRESS);
  const uint8_t quietStored = EEPROM.read(QUIET_MODE_ADDRESS);
  const uint8_t verboseStored = EEPROM.read(VERBOSE_ADDRESS);
  const uint8_t flowStored = EEPROM.read(FLOW_CONTROL_ADDRESS);
  const uint8_t pinPolarityStored = EEPROM.read(PIN_POLARITY_ADDRESS);
  const uint8_t telnetStored = EEPROM.read(TELNET_ADDRESS);
  const uint8_t autoAnswerStored = EEPROM.read(AUTO_ANSWER_ADDRESS);
  Serial.print("Baud: ");
  if (baudIndex < (sizeof(bauds) / sizeof(bauds[0])))
    Serial.println(bauds[baudIndex]);
  else
    Serial.println("?");
  yield();
  Serial.print("SSID: ");
  Serial.println(getEEPROM(SSID_ADDRESS, SSID_LEN));
  yield();
  Serial.print("Password: ");
  Serial.println(getEEPROM(PASS_ADDRESS, PASS_LEN));
  yield();
  Serial.print("Busy message: ");
  Serial.println(getEEPROM(BUSY_MSG_ADDRESS, BUSY_MSG_LEN));
  yield();
  // Status flags
  Serial.print("E");
  Serial.print(echoStored);
  Serial.print(" ");
  yield();
  Serial.print("Q");
  Serial.print(quietStored);
  Serial.print(" ");
  yield();
  Serial.print("V");
  Serial.print(verboseStored);
  Serial.print(" ");
  yield();
  Serial.print("&K");
  Serial.print(flowStored);
  Serial.print(" ");
  yield();
  Serial.print("&P");
  Serial.print(pinPolarityStored);
  Serial.print(" ");
  yield();
  Serial.print("NET");
  Serial.print(telnetStored);
  Serial.print(" ");
  yield();
  Serial.print("S0:");
  Serial.print(autoAnswerStored);
  Serial.print(" ");
  yield();
  Serial.println();
  yield();
  Serial.println("Stored Speed Dial:");
  for (int i = 0; i < 10; i++)
  {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(getEEPROM(speedDialAddresses[i], 50));
    yield();
  }
  Serial.println();
}

void storeSpeedDial(byte num, String location)
{
  speedDials[num] = location;
}

void displayHelp()
{
  welcome();
  auto printLine = [](const __FlashStringHelper *msg)
  {
    Serial.println(msg);
    yield();
  };
  printLine(F("AT Command Summary:"));
  printLine(F("Dial Host:           ATDTHOST:PORT"));
  printLine(F("Speed Dial:          ATDSN (N=0-9)"));
  printLine(F("PPP Session.:        ATDTPPP"));
  printLine(F("Set Speed Dial:      AT&ZN=HOST:PORT (where N is 0-9)"));
  printLine(F("Handle Telnet:       ATNETN (N=0,1)"));
  printLine(F("Network Information: ATI"));
  printLine(F("HTTP GET:            ATGET<URL>"));
  printLine(F("GOPHER Request:      ATGPH<URL>"));
  printLine(F("Auto Answer:         ATS0=N (N=0,1)"));
  printLine(F("Set BUSY Message:    AT$BM=YOUR BUSY MESSAGE"));
  printLine(F("Load from NVRAM:     ATZ"));
  printLine(F("Save to NVRAM:       AT&W"));
  printLine(F("Show Settings:       AT&V"));
  printLine(F("Reset To Defaults:   AT&F"));
  printLine(F("Pin Polarity:        AT&PN (N=0/INV,1/NORM)"));
  printLine(F("Echo On/Off:         ATE0 / ATE1"));
  printLine(F("Quiet Mode On/Off:   ATQ0 / ATQ1"));
  printLine(F("Verbose On/Off:      ATV1 / ATV0"));
  printLine(F("Set SSID:            AT$SSID=WIFISSID"));
  printLine(F("Set Password:        AT$PASS=PASSWORD"));
  waitForSpace();
  printLine(F("Set Baud Rate:       AT$SB=N (300,1200,2400,4800,9600,19200,38400,57600,115200)"));
  printLine(F("Flow Control:        AT&KN (N=0/N,1/HW,2/SW)"));
  printLine(F("Wi-Fi On/Off:        ATC1 / ATC0"));
  printLine(F("Hang Up:             ATH"));
  printLine(F("Enter CMD mode:      +++"));
  printLine(F("Exit CMD mode:       ATO"));
  printLine(F("Update Firmware:     AT$FW"));
}

void displayCurrentSettings()
{
  Serial.println(F("Active Profile:"));
  yield();
  Serial.print(F("Baud: "));
  if (serialspeed < (sizeof(bauds) / sizeof(bauds[0])))
    Serial.println(bauds[serialspeed]);
  else
    Serial.println(F("?"));
  yield();
  auto printLabelValue = [&](const __FlashStringHelper *label, const String &value)
  {
    Serial.print(label);
    Serial.println(value);
    yield();
  };
  printLabelValue(F("SSID: "), ssid);
  printLabelValue(F("Password: "), password);
  printLabelValue(F("BUSY Message: "), busyMsg);
  // Status flags
  Serial.print(F("E"));
  Serial.print(echo);
  Serial.print(F(" "));
  Serial.print(F("Q"));
  Serial.print(quietMode);
  Serial.print(F(" "));
  Serial.print(F("V"));
  Serial.print(verboseResults);
  Serial.print(F(" "));
  Serial.print(F("&K"));
  Serial.print(flowControl);
  Serial.print(F(" "));
  Serial.print(F("&P"));
  Serial.print(pinPolarity);
  Serial.print(F(" "));
  Serial.print(F("NET"));
  Serial.print(telnet);
  Serial.print(F(" "));
  Serial.print(F("S0:"));
  Serial.print(autoAnswer);
  Serial.print(F(" "));
  Serial.println();
  yield();
  Serial.println(F("Speed Dial:"));
  for (int i = 0; i < 10; i++)
  {
    Serial.print(i);
    Serial.print(F(": "));
    Serial.println(speedDials[i]);
    yield();
  }
  Serial.println();
}
