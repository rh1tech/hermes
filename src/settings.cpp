#include <Arduino.h>
#include "globals.h" // Ensure this file contains declarations for all referenced variables and constants
#include <EEPROM.h>

// Function declarations
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
    // Serial.print(char(EEPROM.read(i)));
  }
  // Serial.println();
  return myString;
}

void setEEPROM(String inString, int startAddress, int maxLen)
{
  for (size_t i = static_cast<size_t>(startAddress);
       i < static_cast<size_t>(startAddress) + inString.length();
       i++)
  {
    EEPROM.write(i, inString[i - startAddress]);
    // Serial.print(i, DEC); Serial.print(": "); Serial.println(inString[i - startAddress]);
    // if (EEPROM.read(i) != inString[i - startAddress]) { Serial.print(" (!)"); }
    // Serial.println();
  }
  // null pad the remainder of the memory space
  for (int i = inString.length() + startAddress; i < maxLen + startAddress; i++)
  {
    EEPROM.write(i, 0x00);
    // Serial.print(i, DEC); Serial.println(": 0x00");
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
  echo = EEPROM.read(ECHO_ADDRESS);
  autoAnswer = EEPROM.read(AUTO_ANSWER_ADDRESS);
  // serialspeed = EEPROM.read(BAUD_ADDRESS);

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
  Serial.println("STORED PROFILE:");
  yield();
  Serial.print("BAUD: ");
  Serial.println(bauds[EEPROM.read(BAUD_ADDRESS)]);
  yield();
  Serial.print("SSID: ");
  Serial.println(getEEPROM(SSID_ADDRESS, SSID_LEN));
  yield();
  Serial.print("PASS: ");
  Serial.println(getEEPROM(PASS_ADDRESS, PASS_LEN));
  yield();
  // Serial.print("SERVER TCP PORT: "); Serial.println(word(EEPROM.read(SERVER_PORT_ADDRESS), EEPROM.read(SERVER_PORT_ADDRESS+1))); yield();
  Serial.print("BUSY MSG: ");
  Serial.println(getEEPROM(BUSY_MSG_ADDRESS, BUSY_MSG_LEN));
  yield();
  Serial.print("E");
  Serial.print(EEPROM.read(ECHO_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("Q");
  Serial.print(EEPROM.read(QUIET_MODE_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("V");
  Serial.print(EEPROM.read(VERBOSE_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("&K");
  Serial.print(EEPROM.read(FLOW_CONTROL_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("&P");
  Serial.print(EEPROM.read(PIN_POLARITY_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("NET");
  Serial.print(EEPROM.read(TELNET_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.print("S0:");
  Serial.print(EEPROM.read(AUTO_ANSWER_ADDRESS));
  Serial.print(" ");
  yield();
  Serial.println();
  yield();

  Serial.println("STORED SPEED DIAL:");
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
  // if (num < 0 || num > 9) { return; }
  speedDials[num] = location;
  // Serial.print("STORED "); Serial.print(num); Serial.print(": "); Serial.println(location);
}

void eepromUpgradeToDeprecate()
{
  /*
    If EEPROM version is 01 upgrade to version 02 which adds the quiet mode flag.
    If verbose mode was previously 2 (silent) set quiet mode to on.
    Otherwise set it to off.
  */
  if (EEPROM.read(VERSION_ADDRESS) == 0 && EEPROM.read(VERSION_ADDRESS + 1) == 1)
  {
    EEPROM.write(QUIET_MODE_ADDRESS, 0x00);
    if (EEPROM.read(VERBOSE_ADDRESS) == 2)
    {
      EEPROM.write(VERBOSE_ADDRESS, 0x00);
      EEPROM.write(QUIET_MODE_ADDRESS, 0x01);
    }
    else
    {
      EEPROM.write(QUIET_MODE_ADDRESS, 0x00);
    }
    EEPROM.write(VERSION_ADDRESS, VERSIONA);
    EEPROM.write(VERSION_ADDRESS + 1, VERSIONB);
  }

  if (EEPROM.read(VERSION_ADDRESS) != VERSIONA || EEPROM.read(VERSION_ADDRESS + 1) != VERSIONB)
  {
    defaultEEPROM();
  }
}

void displayHelp()
{
  welcome();
  Serial.println("AT COMMAND SUMMARY:");
  yield();
  Serial.println("DIAL HOST............: ATDTHOST:PORT");
  yield();
  Serial.println("SPEED DIAL...........: ATDSN (N=0-9)");
  yield();
  Serial.println("PPP SESSION..........: ATDTPPP");
  yield();
  Serial.println("SET SPEED DIAL.......: AT&ZN=HOST:PORT (N=0-9)");
  yield();
  Serial.println("HANDLE TELNET........: ATNETN (N=0,1)");
  yield();
  Serial.println("NETWORK INFO.........: ATI");
  yield();
  Serial.println("HTTP GET.............: ATGET<URL>");
  yield();
  Serial.println("GOPHER REQUEST.......: ATGPH<URL>");
  yield();
  // Serial.println("SERVER PORT........: AT$SP=N (N=1-65535)"); yield();
  Serial.println("AUTO ANSWER..........: ATS0=N (N=0,1)");
  yield();
  Serial.println("SET BUSY MSG.........: AT$BM=YOUR BUSY MESSAGE");
  yield();
  Serial.println("LOAD NVRAM...........: ATZ");
  yield();
  Serial.println("SAVE TO NVRAM........: AT&W");
  yield();
  Serial.println("SHOW SETTINGS........: AT&V");
  yield();
  Serial.println("FACT. DEFAULTS.......: AT&F");
  yield();
  Serial.println("PIN POLARITY.........: AT&PN (N=0/INV,1/NORM)");
  yield();
  Serial.println("ECHO OFF/ON..........: ATE0 / ATE1");
  yield();
  Serial.println("QUIET MODE OFF/ON....: ATQ0 / ATQ1");
  yield();
  Serial.println("VERBOSE OFF/ON.......: ATV0 / ATV1");
  yield();
  Serial.println("SET SSID......: AT$SSID=WIFISSID");
  yield();
  Serial.println("SET PASSWORD..: AT$PASS=WIFIPASSWORD");
  yield();
  waitForSpace();
  Serial.println("SET BAUD RATE.: AT$SB=N (3,12,24,48,96");
  yield();
  Serial.println("                192,384,576,1152)*100");
  yield();
  Serial.println("FLOW CONTROL..: AT&KN (N=0/N,1/HW,2/SW)");
  yield();
  Serial.println("WIFI OFF/ON...: ATC0 / ATC1");
  yield();
  Serial.println("HANGUP........: ATH");
  yield();
  Serial.println("ENTER CMD MODE: +++");
  yield();
  Serial.println("EXIT CMD MODE.: ATO");
  yield();
  Serial.println("UPDATE FIRMWARE.: AT$FW");
  yield();
  Serial.println("QUERY MOST COMMANDS FOLLOWED BY '?'");
  yield();
}

void displayCurrentSettings()
{
  Serial.println("ACTIVE PROFILE:");
  yield();
  Serial.print("BAUD: ");
  Serial.println(bauds[serialspeed]);
  yield();
  Serial.print("SSID: ");
  Serial.println(ssid);
  yield();
  Serial.print("PASS: ");
  Serial.println(password);
  yield();
  // Serial.print("SERVER TCP PORT: "); Serial.println(tcpServerPort); yield();
  Serial.print("BUSY MSG: ");
  Serial.println(busyMsg);
  yield();
  Serial.print("E");
  Serial.print(echo);
  Serial.print(" ");
  yield();
  Serial.print("Q");
  Serial.print(quietMode);
  Serial.print(" ");
  yield();
  Serial.print("V");
  Serial.print(verboseResults);
  Serial.print(" ");
  yield();
  Serial.print("&K");
  Serial.print(flowControl);
  Serial.print(" ");
  yield();
  Serial.print("&P");
  Serial.print(pinPolarity);
  Serial.print(" ");
  yield();
  Serial.print("NET");
  Serial.print(telnet);
  Serial.print(" ");
  yield();
  Serial.print("S0:");
  Serial.print(autoAnswer);
  Serial.print(" ");
  yield();
  Serial.println();
  yield();

  Serial.println("SPEED DIAL:");
  for (int i = 0; i < 10; i++)
  {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(speedDials[i]);
    yield();
  }
  Serial.println();
}
