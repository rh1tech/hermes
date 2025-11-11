#include <Arduino.h>

#ifdef ESP32
// ESP32-specific includes for PPP
#include "netif/ppp/pppos.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/prot/ip4.h"
#endif

#ifdef ESP8266
// ESP8266-specific includes if needed
// Note: PPP functionality may not be available on ESP8266
#endif
#include <globals.h>

// ========================= Utility Functions =========================

String connectTimeString()
{
  unsigned long now = millis();
  int secs = (now - connectTime) / 1000;
  int mins = secs / 60;
  int hours = mins / 60;
  String out = "";
  if (hours < 10)
    out.concat("0");
  out.concat(String(hours));
  out.concat(":");
  if (mins % 60 < 10)
    out.concat("0");
  out.concat(String(mins % 60));
  out.concat(":");
  if (secs % 60 < 10)
    out.concat("0");
  out.concat(String(secs % 60));
  return out;
}

// ========================= Command Structure =========================

struct ATCommand
{
  const char *pattern;
  void (*handler)(const String &up, const String &raw);
  bool exact_match;
};

// Forward declarations
void handleAT(const String &, const String &);
void handleDial(const String &, const String &);
void handleSSHConnect(const String &, const String &);
void handleTelnetMode(const String &, const String &);
void handleAnswer(const String &, const String &);
void handleHelp(const String &, const String &);
void handleReset(const String &, const String &);
void handleWiFiConnection(const String &, const String &);
void handleEcho(const String &, const String &);
void handleVerbosity(const String &, const String &);
void handlePinPolarity(const String &, const String &);
void handleFlowControl(const String &, const String &);
void handleBaudRate(const String &, const String &);
void handleBusyMessage(const String &, const String &);
void handleNetworkInfo(const String &, const String &);
void handleProfileView(const String &, const String &);
void handleProfileWrite(const String &, const String &);
void handleFirmwareUpdate(const String &, const String &);
void handleSpeedDial(const String &, const String &);
void handleSSID(const String &, const String &);
void handlePassword(const String &, const String &);
void handleFactoryReset(const String &, const String &);
void handleAutoAnswer(const String &, const String &);
void handleHexTranslate(const String &, const String &);
void handleHangup(const String &, const String &);
void handleReboot(const String &, const String &);
void handleOnline(const String &, const String &);
void handleWiFiScan(const String &, const String &);
void handleServerPort(const String &, const String &);
void handleIPAddress(const String &, const String &);
void handleHTTPGet(const String &, const String &);
void handleGopher(const String &, const String &);
void handleQuiet(const String &, const String &);
void handleSDInit(const String &, const String &);
void handleHardReset(const String &, const String &);
void handleSDSpeed(const String &, const String &);

// ========================= Helper Functions =========================

// Overload for bool variables (e.g., quietMode)
static void handleBinaryParameter(const String &up, const String &prefix, bool &variable)
{
  if (up.length() < prefix.length() + 1)
  {
    sendResult(RES_ERROR);
    return;
  }
  String p = up.substring(prefix.length(), prefix.length() + 1);
  if (p == "?")
  {
    sendString(String(variable ? 1 : 0));
    sendResult(RES_OK);
  }
  else if (p == "0")
  {
    variable = false;
    sendResult(RES_OK);
  }
  else if (p == "1")
  {
    variable = true;
    sendResult(RES_OK);
  }
  else
  {
    sendResult(RES_ERROR);
  }
}

// Legacy function for backwards compatibility
void handleQuietMode(String upCmd)
{
  handleBinaryParameter(upCmd, "ATQ", quietMode);
}

// ========================== Connect to SSH ===========================

void connectSSH(String upCmd)
{
  String host, port;
  int portIndex = upCmd.indexOf(":");
  if (portIndex != -1)
  {
    host = upCmd.substring(5, portIndex);
    port = upCmd.substring(portIndex + 1, upCmd.length());
  }
  else
  {
    host = upCmd.substring(5, upCmd.length());
    port = "22";
  }
  host.trim();
  port.trim();
  Serial.print("Dialing ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
}

// ========================= Dial Out Function =========================

void dialOut(String upCmd)
{
  if (callConnected)
  {
    sendResult(RES_ERROR);
    return;
  }

  String host, port;
  int portIndex;

  if (upCmd.indexOf("ATDS") == 0)
  {
    byte speedNum = upCmd.substring(4, 5).toInt();
    portIndex = speedDials[speedNum].indexOf(':');
    if (portIndex != -1)
    {
      host = speedDials[speedNum].substring(0, portIndex);
      port = speedDials[speedNum].substring(portIndex + 1);
    }
    else
    {
      port = "23";
    }
  }
  else
  {
    int portIndex = upCmd.indexOf(":");
    if (portIndex != -1)
    {
      host = upCmd.substring(4, portIndex);
      port = upCmd.substring(portIndex + 1, upCmd.length());
    }
    else
    {
      host = upCmd.substring(4, upCmd.length());
      port = "23";
    }
  }

  host.trim();
  port.trim();

#ifdef ESP8266
  if (host.equals("PPP") || host.equals("777"))
  {
    if (ppp)
    {
      Serial.println("PPP already active");
      sendResult(RES_ERROR);
      return;
    }
    ppp = pppos_create(&ppp_netif, ppp_output_cb, ppp_status_cb, NULL);
    ppp_set_usepeerdns(ppp, 1);
    ppp_set_ipcp_dnsaddr(ppp, 0, ip_2_ip4((const ip_addr_t *)WiFi.dnsIP(0)));
    ppp_set_ipcp_dnsaddr(ppp, 1, ip_2_ip4((const ip_addr_t *)WiFi.dnsIP(1)));

#if PPP_AUTH_SUPPORT
    ppp_set_auth(ppp, PPPAUTHTYPE_NONE, "", "");
    ppp_set_auth_required(ppp, 0);
#endif
    ppp_set_ipcp_ouraddr(ppp, ip_2_ip4((const ip_addr_t *)WiFi.localIP()));
    ppp_set_ipcp_hisaddr(ppp, ip_2_ip4((const ip_addr_t *)IPAddress(192, 168, 240, 2)));
    err_t ppp_err;
    ppp_err = ppp_listen(ppp);
    if (ppp_err == PPPERR_NONE)
    {
      sendResult(RES_CONNECT);
      connectTime = millis();
      cmdMode = false;
      callConnected = true;
      setCarrierDCDPin(callConnected);
    }
    else
    {
      Serial.println("ppp_listen failed\n");
      ppp_status_cb(ppp, ppp_err, NULL);
      ppp_close(ppp, 1);
      sendResult(RES_ERROR);
    }
    return;
  }
#endif

  Serial.print("Dialing ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
  char *hostChr = new char[host.length() + 1];
  host.toCharArray(hostChr, host.length() + 1);
  int portInt = port.toInt();
  tcpClient.setNoDelay(true); // Try to disable naggle
  if (tcpClient.connect(hostChr, portInt))
  {
    tcpClient.setNoDelay(true); // Try to disable naggle
    sendResult(RES_CONNECT);
    connectTime = millis();
    cmdMode = false;
    Serial.flush();
    callConnected = true;
    setCarrierDCDPin(callConnected);
  }
  else
  {
    sendResult(RES_NOANSWER);
    callConnected = false;
    setCarrierDCDPin(callConnected);
  }
  delete[] hostChr;
}

// ========================= Command Table =========================

static const ATCommand atCommands[] = {
    // Exact matches first
    {"AT", handleAT, true},
    {"ATNET0", handleTelnetMode, true},
    {"ATNET1", handleTelnetMode, true},
    {"ATNET?", handleTelnetMode, true},
    {"ATA", handleAnswer, true},
    {"AT?", handleHelp, true},
    {"ATHELP", handleHelp, true},
    {"ATZ", handleReset, true},
    {"ATC0", handleWiFiConnection, true},
    {"ATC1", handleWiFiConnection, true},
    {"ATI", handleNetworkInfo, true},
    {"AT&V", handleProfileView, true},
    {"AT&W", handleProfileWrite, true},
    {"AT$FW", handleFirmwareUpdate, true},
    {"AT$SSID?", handleSSID, true},
    {"AT$PASS?", handlePassword, true},
    {"AT&F", handleFactoryReset, true},
    {"ATS0=0", handleAutoAnswer, true},
    {"ATS0=1", handleAutoAnswer, true},
    {"ATS0?", handleAutoAnswer, true},
    {"ATHEX=1", handleHexTranslate, true},
    {"ATHEX=0", handleHexTranslate, true},
    {"ATO", handleOnline, true},
    {"ATSCAN", handleWiFiScan, true},
    {"AT$SP?", handleServerPort, true},
    {"ATIP?", handleIPAddress, true},
    {"AT$SB?", handleBaudRate, true},
    {"AT$BM?", handleBusyMessage, true},
    {"AT$SDINIT", handleSDInit, true},
    {"AT$HRESET", handleHardReset, true},
    {"AT$SDSPEED", handleSDSpeed, true},

    // Prefix matches
    {"ATDT", handleDial, false},
    {"ATSSH", handleSSHConnect, false},
    {"ATDP", handleDial, false},
    {"ATDI", handleDial, false},
    {"ATDS", handleDial, false},
    {"ATE", handleEcho, false},
    {"ATV", handleVerbosity, false},
    {"AT&P", handlePinPolarity, false},
    {"AT&K", handleFlowControl, false},
    {"AT$SB=", handleBaudRate, false},
    {"AT$BM=", handleBusyMessage, false},
    {"AT&Z", handleSpeedDial, false},
    {"AT$SSID=", handleSSID, false},
    {"AT$PASS=", handlePassword, false},
    {"ATH", handleHangup, false},
    {"AT$RB", handleReboot, false},
    {"AT$SP=", handleServerPort, false},
    {"ATGET", handleHTTPGet, false},
    {"ATGPH", handleGopher, false},
    {"ATQ", handleQuiet, false},
};

static const int numCommands = sizeof(atCommands) / sizeof(atCommands[0]);

// ========================= Main Command Function =========================

void command()
{
  cmd.trim();
  if (cmd == "")
    return;

  Serial.println();

  // Keep both uppercase for matching and original for case-sensitive values
  const String rawCmd = cmd;
  String upCmd = rawCmd;
  upCmd.toUpperCase();

  // Dispatch to handlers
  for (int i = 0; i < numCommands; i++)
  {
    const ATCommand &c = atCommands[i];
    if (c.exact_match)
    {
      if (upCmd == c.pattern)
      {
        c.handler(upCmd, rawCmd);
        cmd = "";
        return;
      }
    }
    else
    {
      if (upCmd.startsWith(c.pattern))
      {
        c.handler(upCmd, rawCmd);
        cmd = "";
        return;
      }
    }
  }

  // Unknown command
  Serial.print("Unknown command. Type AT? for help.");
  sendResult(RES_ERROR);
  cmd = "";
}

// ========================= Command Handlers =========================

void handleAT(const String &, const String &)
{
  sendResult(RES_OK);
}

void handleDial(const String &up, const String &)
{
  dialOut(up);
}

void handleSSHConnect(const String &up, const String &)
{
  connectSSH(up);
}

void handleTelnetMode(const String &up, const String &)
{
  if (up == "ATNET0")
  {
    telnet = false;
  }
  else if (up == "ATNET1")
  {
    telnet = true;
  }
  else
  { // ATNET?
    Serial.println(String(telnet));
  }
  sendResult(RES_OK);
}

void handleAnswer(const String &, const String &)
{
  if (tcpServer.hasClient())
  {
    answerCall();
  }
  else
  {
    sendResult(RES_ERROR);
  }
}

void handleHelp(const String &, const String &)
{
  displayHelp();
  sendResult(RES_OK);
}

void handleReset(const String &, const String &)
{
  readSettings();
  sendResult(RES_OK);
}

void handleWiFiConnection(const String &up, const String &)
{
  if (up == "ATC0")
  {
    disconnectWiFi();
  }
  else
  {
    connectWiFi();
  }
  sendResult(RES_OK);
}

void handleEcho(const String &up, const String &)
{
  handleBinaryParameter(up, "ATE", echo);
}

void handleVerbosity(const String &up, const String &)
{
  handleBinaryParameter(up, "ATV", verboseResults);
}

void handlePinPolarity(const String &up, const String &)
{
  if (up.length() < 5)
  {
    sendResult(RES_ERROR);
    return;
  }
  String p = up.substring(4, 5);
  if (p == "?")
  {
    sendString(String(pinPolarity));
    sendResult(RES_OK);
  }
  else if (p == "0")
  {
    pinPolarity = P_INVERTED;
    sendResult(RES_OK);
    setCarrierDCDPin(callConnected);
  }
  else if (p == "1")
  {
    pinPolarity = P_NORMAL;
    sendResult(RES_OK);
    setCarrierDCDPin(callConnected);
  }
  else
  {
    sendResult(RES_ERROR);
  }
}

void handleFlowControl(const String &up, const String &)
{
  if (up.length() < 5)
  {
    sendResult(RES_ERROR);
    return;
  }
  String p = up.substring(4, 5);
  if (p == "?")
  {
    sendString(String(flowControl));
    sendResult(RES_OK);
  }
  else
  {
    int v = p.toInt();
    if (v >= 0 && v <= 2)
    {
      flowControl = v;
      sendResult(RES_OK);
    }
    else
    {
      sendResult(RES_ERROR);
    }
  }
}

void handleBaudRate(const String &up, const String &)
{
  if (up.startsWith("AT$SB="))
  {
    setBaudRate(up.substring(6).toInt());
  }
  else
  {
    sendString(String(bauds[serialspeed]));
    sendResult(RES_OK);
  }
}

void handleBusyMessage(const String &up, const String &raw)
{
  if (up.startsWith("AT$BM="))
  {
    busyMsg = raw.substring(6); // preserve case
    sendResult(RES_OK);
  }
  else
  {
    sendString(busyMsg);
    sendResult(RES_OK);
  }
}

void handleNetworkInfo(const String &, const String &)
{
  displayNetworkStatus();
  sendResult(RES_OK);
}

void handleProfileView(const String &, const String &)
{
  displayCurrentSettings();
  waitForSpace();
  displayStoredSettings();
  sendResult(RES_OK);
}

void handleProfileWrite(const String &, const String &)
{
  writeSettings();
  sendResult(RES_OK);
}

void handleFirmwareUpdate(const String &, const String &)
{
  firmwareUpdating = true;
}

void handleSpeedDial(const String &up, const String &raw)
{
  if (up.length() < 5)
  {
    sendResult(RES_ERROR);
    return;
  }
  byte speedNum = up.substring(4, 5).toInt();
  if (speedNum > 9)
  {
    sendResult(RES_ERROR);
    return;
  }

  if (up.length() >= 6)
  {
    String op = up.substring(5, 6);
    if (op == "=")
    {
      storeSpeedDial(speedNum, raw.substring(6)); // preserve case
      sendResult(RES_OK);
    }
    else if (op == "?")
    {
      sendString(speedDials[speedNum]);
      sendResult(RES_OK);
    }
    else
    {
      sendResult(RES_ERROR);
    }
  }
  else
  {
    sendResult(RES_ERROR);
  }
}

void handleSSID(const String &up, const String &raw)
{
  if (up.startsWith("AT$SSID="))
  {
    ssid = raw.substring(8); // preserve case
    sendResult(RES_OK);
  }
  else
  {
    sendString(ssid);
    sendResult(RES_OK);
  }
}

void handlePassword(const String &up, const String &raw)
{
  if (up.startsWith("AT$PASS="))
  {
    password = raw.substring(8); // preserve case
    sendResult(RES_OK);
  }
  else
  {
    sendString(password);
    sendResult(RES_OK);
  }
}

void handleFactoryReset(const String &, const String &)
{
  defaultEEPROM();
  readSettings();
  sendResult(RES_OK);
}

void handleAutoAnswer(const String &up, const String &)
{
  if (up == "ATS0=0")
  {
    autoAnswer = false;
    sendResult(RES_OK);
  }
  else if (up == "ATS0=1")
  {
    autoAnswer = true;
    sendResult(RES_OK);
  }
  else
  { // ATS0?
    sendString(String(autoAnswer));
    sendResult(RES_OK);
  }
}

void handleHexTranslate(const String &up, const String &)
{
  if (up == "ATHEX=1")
  {
    hex = true;
  }
  else
  {
    hex = false;
  }
  sendResult(RES_OK);
}

void handleHangup(const String &, const String &)
{
  hangUp();
}

void handleReboot(const String &, const String &)
{
  sendResult(RES_OK);
  Serial.flush();
  delay(500);
  ESP.restart();
}

void handleOnline(const String &, const String &)
{
  if (callConnected == 1)
  {
    sendResult(RES_CONNECT);
    cmdMode = false;
  }
  else
  {
    sendResult(RES_ERROR);
  }
}

void handleWiFiScan(const String &, const String &)
{
  Serial.println("Scanning for WiFi networks...\n");
  int n = WiFi.scanNetworks();
  if (n <= 0)
  {
    sendString("No networks found");
    sendResult(RES_OK);
  }
  else
  {
    const int PAGE_SIZE = 10;
    int printed = 0;
    for (int i = 0; i < n; i++)
    {
      String line = String(i) + ": " + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm)";
      uint8_t enc = WiFi.encryptionType(i);
#ifdef ESP32
      if (enc != WIFI_AUTH_OPEN)
#else
      if (enc != ENC_TYPE_NONE)
#endif
        line += " *";
      Serial.println(line);
      printed++;
      if (n > PAGE_SIZE && (printed % PAGE_SIZE == 0) && (i != n - 1))
        waitForSpace();
    }
    sendResult(RES_OK);
  }
  WiFi.scanDelete();
}

void handleServerPort(const String &up, const String &)
{
  if (up.startsWith("AT$SP="))
  {
    tcpServerPort = up.substring(6).toInt();
    sendString("Changes require to run AT&W and restart to take effect");
    sendResult(RES_OK);
  }
  else
  {
    sendString(String(tcpServerPort));
    sendResult(RES_OK);
  }
}

void handleIPAddress(const String &, const String &)
{
  Serial.println(WiFi.localIP());
  sendResult(RES_OK);
}

void handleHTTPGet(const String &, const String &)
{
  handleHTTPRequest();
}

void handleGopher(const String &, const String &)
{
  handleGopherRequest();
}

void handleQuiet(const String &up, const String &)
{
  handleBinaryParameter(up, "ATQ", quietMode);
}

void handleSDInit(const String &, const String &)
{
  manualInitSDCard();
  sendResult(RES_OK);
}

void handleHardReset(const String &, const String &)
{
  Serial.println();
  Serial.println("\x1b[37;41m WARNING: HARD RESET \x1b[0m");
  Serial.println("This will erase ALL settings and reboot the device.");
  Serial.println("Factory defaults will be restored.");
  Serial.println();
  Serial.print("Erasing EEPROM...");
  Serial.flush();
  
  // Clear entire EEPROM
  for (int i = 0; i < LAST_ADDRESS + 1; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
  
  Serial.println(" Done");
  Serial.println("Writing factory defaults...");
  Serial.flush();
  
  // Write factory defaults
  defaultEEPROM();
  
  Serial.println("Settings restored to factory defaults");
  sendResult(RES_OK);
  Serial.flush();
  
  Serial.println();
  Serial.println("Rebooting in 3 seconds...");
  Serial.flush();
  delay(3000);
  
  ESP.restart();
}

void handleSDSpeed(const String &, const String &)
{
  testSDCardSpeed();
  sendResult(RES_OK);
}