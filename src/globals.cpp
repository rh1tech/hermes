#include <Arduino.h>
#include <globals.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <lwip/netif.h>
#include <netif/ppp/ppp.h>
#include <netif/ppp/pppos.h>
#include <IPAddress.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

String connectTimeString();
void eepromUpgradeToDeprecate();
void readSettings();
void serialSetup();
void wifiSetup();
void webserverSetup();
int checkButton();
void command();
void handleOTAFirmware();
void handleWebServer();
void handleConnectedMode();
void handleLEDState();
void sendResult(int resultCode);
void setCarrierDCDPin(byte carrier);
u32_t ppp_output_cb(ppp_pcb *pcb, unsigned char *data, u32_t len, void *ctx);
void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx);
void sendString(String msg);
void hangUp();
void answerCall();
void displayHelp();
void disconnectWiFi();
void connectWiFi();
void setBaudRate(int inSpeed);
void displayNetworkStatus();
void displayCurrentSettings();
void waitForSpace();
void displayStoredSettings();
void writeSettings();
void storeSpeedDial(byte num, String location);
void defaultEEPROM();
void handleHTTPRequest();
void handleGopherRequest();
void welcome();
void led_on();
void handleFlowControl();
String ipToString(IPAddress ip);
void check_for_firmware_update();
String getWifiStatus();
void handleGetStatus();
void redirectToRoot();
void handleRoot();
void handleWebHangUp();
void handleReboot();
void handleGetSettings();
void handleUpdateSettings();
void handleUpdateFirmware();
void handleUpdateSpeeddial();
void handleFactoryDefaults();
void handleFileUpload();
void handleGetSpeedDials();
void handleConnectedMode();
String getMacAddress();
void handleFlowControl();
void handleLEDState();
void handleFileUpload();
String getCallStatus();
String getCallLength();

WiFiClient tcpClient;
WiFiServer tcpServer(tcpServerPort);
MDNSResponder mdns;
ppp_pcb *ppp;
struct netif ppp_netif;
String speedDials[10];
String ssid, password, busyMsg;
byte serialspeed;
String build = "1.00";
String cmd = "";            // Gather a new AT command to this string from serial
bool cmdMode = true;        // Are we in AT command mode or connected mode
bool callConnected = false; // Are we currently in a call
bool telnet = false;        // Is telnet control code handling enabled
bool verboseResults = false;
bool firmwareUpdating = false;
int tcpServerPort = LISTEN_PORT;
unsigned long lastRingMs = 0; // Time of last "RING" message (millis())
unsigned long ledTime = 0;
const int speedDialAddresses[] = {DIAL0_ADDRESS, DIAL1_ADDRESS, DIAL2_ADDRESS, DIAL3_ADDRESS, DIAL4_ADDRESS, DIAL5_ADDRESS, DIAL6_ADDRESS, DIAL7_ADDRESS, DIAL8_ADDRESS, DIAL9_ADDRESS};
const int bauds[9] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
bool echo = true;
bool autoAnswer = false;
byte ringCount = 0;
String resultCodes[] = {"OK", "CONNECT", "RING", "NO CARRIER", "ERROR", "", "NO DIALTONE", "BUSY", "NO ANSWER"};
unsigned long connectTime = 0;
bool hex = false;
byte flowControl = F_NONE; // Use flow control
bool txPaused = false;     // Has flow control asked us to pause?
byte pinPolarity = P_NORMAL;
bool quietMode = false;

void setCarrierDCDPin(byte carrier)
{
    if (pinPolarity == P_NORMAL)
        carrier = !carrier;
    digitalWrite(DCD_PIN, carrier);
}

void sendResult(int resultCode)
{
    Serial.print("\r\n");
    if (quietMode == 1)
    {
        return;
    }
    if (verboseResults == 0)
    {
        Serial.println(resultCode);
        return;
    }
    if (resultCode == R_CONNECT)
    {
        Serial.print(String(resultCodes[R_CONNECT]) + " " + String(bauds[serialspeed]));
    }
    else if (resultCode == R_NOCARRIER)
    {
        Serial.print(String(resultCodes[R_NOCARRIER]) + " (" + connectTimeString() + ")");
    }
    else
    {
        Serial.print(String(resultCodes[resultCode]));
    }
    Serial.print("\r\n");
}

void sendString(String msg)
{
    Serial.print("\r\n");
    Serial.print(msg);
    Serial.print("\r\n");
}

void waitForSpace()
{
    Serial.print("PRESS SPACE");
    char c = 0;
    while (c != 0x20)
    {
        if (Serial.available() > 0)
        {
            c = Serial.read();
        }
    }
    Serial.print("\r");
}

void welcome()
{
    Serial.println();
    Serial.println("Hermes");
    Serial.println("Serial Wi-Fi Modem Emulator");
    Serial.println("Version " + build + "");
    Serial.println();
}

void waitForFirstInput()
{
    char c;
    // unsigned long startMillis = millis();
    while (c != 8 && c != 127 && c != 20)
    { // Check for the backspace key to begin
        while (c != 32)
        { // Check for space to begin
            // disabled the wait before welcome
            while (c != 0x0a && c != 0x0d)
            {
                if (Serial.available() > 0)
                {
                    c = Serial.read();
                }
                if (checkButton() == 1)
                {
                    break; // button pressed, we're setting to 300 baud and moving on
                }
                // if (millis() - startMillis > 2000) {
                // digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                // startMillis = millis();
                // }
                yield();
            }
        }
    }
}

String ipToString(IPAddress ip)
{
    char s[16];
    sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return s;
}

void hangUp()
{
    if (ppp)
    {
        ppp_close(ppp, 0);
    }
    else
    {
        tcpClient.stop();
    }
    callConnected = false;
    setCarrierDCDPin(callConnected);
    sendResult(R_NOCARRIER);
    connectTime = 0;
}

void answerCall()
{
    tcpClient = tcpServer.accept();
    tcpClient.setNoDelay(true); // try to disable naggle
    // tcpServer.stop();
    sendResult(R_CONNECT);
    connectTime = millis();
    cmdMode = false;
    callConnected = true;
    setCarrierDCDPin(callConnected);
    Serial.flush();
}

void handleIncomingConnection()
{
    if (callConnected == 1 || (autoAnswer == false && ringCount > 3))
    {
        // We're in a call already or didn't answer the call after three rings
        // We didn't answer the call. Notify our party we're busy and disconnect
        ringCount = lastRingMs = 0;
        WiFiClient anotherClient = tcpServer.accept();
        anotherClient.print(busyMsg);
        anotherClient.print("\r\n");
        anotherClient.print("CURRENT CALL LENGTH: ");
        anotherClient.print(connectTimeString());
        anotherClient.print("\r\n");
        anotherClient.print("\r\n");
        anotherClient.flush();
        anotherClient.stop();
        return;
    }

    if (autoAnswer == false)
    {
        if (millis() - lastRingMs > 6000 || lastRingMs == 0)
        {
            lastRingMs = millis();
            sendResult(R_RING);
            ringCount++;
        }
        return;
    }

    if (autoAnswer == true)
    {
        tcpClient = tcpServer.accept();
        if (verboseResults == 1)
        {
            sendString(String("RING ") + ipToString(tcpClient.remoteIP()));
        }
        delay(1000);
        sendResult(R_CONNECT);
        connectTime = millis();
        cmdMode = false;
        tcpClient.flush();
        callConnected = true;
        setCarrierDCDPin(callConnected);
    }
}

// RTS/CTS protocol is a method of handshaking which uses one wire in each direction to allow each
// device to indicate to the other whether or not it is ready to receive data at any given moment.
// One device sends on RTS and listens on CTS; the other does the reverse. A device should drive
// its handshake-output wire low when it is ready to receive data, and high when it is not. A device
// that wishes to send data should not start sending any bytes while the handshake-input wire is low;
// if it sees the handshake wire go high, it should finish transmitting the current byte and then wait
// for the handshake wire to go low before transmitting any more.
// http://electronics.stackexchange.com/questions/38022/what-is-rts-and-cts-flow-control
void handleFlowControl()
{
    if (flowControl == F_NONE)
        return;
    // disabled in case it's accidentally pausing
    //   if (flowControl == F_HARDWARE) {
    //     if (digitalRead(CTS_PIN) == pinPolarity) txPaused = true;
    //     else txPaused = false;
    //   }
    if (flowControl == F_SOFTWARE)
    {
    }
}

void handleCommandMode()
{
    // In command mode, do not exchange data with TCP but gather characters into a string.
    if (Serial.available())
    {
        char chr = Serial.read();

        // Convert uppercase PETSCII characters to lowercase ASCII (C64) in command mode only.
        if ((chr >= 193) && (chr <= 218))
        {
            chr -= 96;
        }

        // Return, Enter, newline, or carriage return - any of these will end the command.
        if ((chr == '\n') || (chr == '\r'))
        {
            command();
        }
        // Backspace or Delete removes the previous character.
        else if ((chr == 8) || (chr == 127) || (chr == 20))
        {
            cmd.remove(cmd.length() - 1);
            if (echo == true)
            {
                Serial.write(chr);
            }
        }
        else
        {
            if (cmd.length() < MAX_CMD_LENGTH)
                cmd.concat(chr);
            if (echo == true)
            {
                Serial.write(chr);
            }
            if (hex)
            {
                Serial.print(chr, HEX);
            }
        }
    }
}

void restoreCommandModeIfDisconnected()
{
    // Switch to command mode if both TCP and PPP are disconnected and the system is not already in command mode.
    if ((!tcpClient.connected() && ppp == NULL) && (cmdMode == false) && callConnected == true)
    {
        cmdMode = true;
        sendResult(R_NOCARRIER);
        connectTime = 0;
        callConnected = false;
        setCarrierDCDPin(callConnected);
        // if (tcpServerPort > 0) tcpServer.begin();
    }
}