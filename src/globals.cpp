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
void readSettings();
void serialSetup();
void wifiSetup();
void webserverSetup();
void command();
void handleOTAFirmware();
void handleWebServer();
void handleConnectedMode();
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
String hermes_version = "1.00";
String hermes_version_url = "http://protea.rh1.tech/ota/hermes.txt";
String cmd = "";            // Gather a new AT command to this string from serial
bool cmdMode = true;        // Are we in AT command mode or connected mode
bool callConnected = false; // Are we currently in a call
bool telnet = false;        // Is telnet control code handling enabled
bool verboseResults = false;
bool firmwareUpdating = false;
int tcpServerPort = LISTEN_PORT;
unsigned long lastRingMs = 0; // Time of last "RING" message (millis())
const int speedDialAddresses[] = {DIAL0_ADDRESS, DIAL1_ADDRESS, DIAL2_ADDRESS, DIAL3_ADDRESS, DIAL4_ADDRESS, DIAL5_ADDRESS, DIAL6_ADDRESS, DIAL7_ADDRESS, DIAL8_ADDRESS, DIAL9_ADDRESS};
const int bauds[9] = {9600, 300, 1200, 2400, 4800, 19200, 38400, 57600, 115200};
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
    if (quietMode)
        return;

    if (!verboseResults)
    {
        Serial.println(resultCode);
        return;
    }

    String resultMessage;
    switch (resultCode)
    {
    case R_CONNECT:
        resultMessage = String(resultCodes[R_CONNECT]) + " " + String(bauds[serialspeed]);
        break;
    case R_NOCARRIER:
        resultMessage = String(resultCodes[R_NOCARRIER]) + " (" + connectTimeString() + ")";
        break;
    default:
        resultMessage = String(resultCodes[resultCode]);
        break;
    }

    const char *colorStart = "\x1b[0m";
    switch (resultCode)
    {
    case R_OK:
    case R_CONNECT:
        colorStart = "\x1b[30;42m"; // black on green
        break;
    case R_RING:
    case R_NOCARRIER:
    case R_NODIALTONE:
    case R_BUSY:
    case R_NOANSWER:
        colorStart = "\x1b[30;43m"; // black on yellow
        break;
    case R_ERROR:
        colorStart = "\x1b[37;41m"; // white on red
        break;
    default:
        colorStart = "\x1b[0m";
        break;
    }

    Serial.print(colorStart);
    Serial.print(" ");
    Serial.print(resultMessage);
    Serial.print(" ");
    Serial.print("\x1b[0m");
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
    Serial.print("Press [Space] key");
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
    Serial.println("\016lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk");
    Serial.println("\016x\017\033[44;30m\xB2\xB1\xB0\033[44;1;37m        PROTEA: SERIAL TERMINAL EMULATOR         \033[0;44;30m\xB0\xB1\xB2\033[0m\016x");
    Serial.println("\016tqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqu");
    Serial.println("\016x\017 ESP8266 Firmware (Hermes) version \x1b[32m" + hermes_version + "\x1b[0m                \016x");
    Serial.println("\016x\017 Documentation and updates: \x1b[36mhttps://rh1.tech\x1b[0m           \016x");
    Serial.println("\016x\017 (C) 2025 Mikhail Matveev, GPL Version 3 License       \016x");
    Serial.println("\016mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj\017");    
    Serial.println();
}

void waitForFirstInput()
{
    while (!Serial.available())
    {
        // Wait for any character
    }
    Serial.read();
}

String ipToString(IPAddress ip)
{
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
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
        ringCount = lastRingMs = 0;
        WiFiClient anotherClient = tcpServer.accept();
        anotherClient.print(busyMsg);
        anotherClient.print("\r\n");
        anotherClient.print("Current call length: ");
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

void handleFlowControl()
{
    if (flowControl == F_NONE)
        return;
    if (flowControl == F_SOFTWARE)
    {
    }
}

void handleCommandMode()
{
    if (Serial.available())
    {
        char chr = Serial.read();
        if ((chr >= 193) && (chr <= 218))
        {
            chr -= 96;
        }
        if ((chr == '\n') || (chr == '\r'))
        {
            command();
        }
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
    if ((!tcpClient.connected() && ppp == NULL) && (cmdMode == false) && callConnected == true)
    {
        cmdMode = true;
        sendResult(R_NOCARRIER);
        connectTime = 0;
        callConnected = false;
        setCarrierDCDPin(callConnected);
    }
}