#include <Arduino.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <lwip/netif.h>
#include <netif/ppp/ppp.h>
#include <netif/ppp/pppos.h>
#include <IPAddress.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#define VERSIONA 0
#define VERSIONB 2
#define VERSION_ADDRESS 0 // EEPROM address
#define VERSION_LEN 2     // Length in bytesF
#define SSID_ADDRESS 2
#define SSID_LEN 32
#define PASS_ADDRESS 34
#define PASS_LEN 63
#define IP_TYPE_ADDRESS 97   // for future use
#define STATIC_IP_ADDRESS 98 // length 4, for future use
#define STATIC_GW 102        // length 4, for future use
#define STATIC_DNS 106       // length 4, for future use
#define STATIC_MASK 110      // length 4, for future use
#define BAUD_ADDRESS 111
#define ECHO_ADDRESS 112
#define SERVER_PORT_ADDRESS 113 // 2 bytes
#define AUTO_ANSWER_ADDRESS 115 // 1 byte
#define TELNET_ADDRESS 116      // 1 byte
#define VERBOSE_ADDRESS 117
#define FLOW_CONTROL_ADDRESS 119
#define PIN_POLARITY_ADDRESS 120
#define QUIET_MODE_ADDRESS 121
#define DIAL0_ADDRESS 200
#define DIAL1_ADDRESS 250
#define DIAL2_ADDRESS 300
#define DIAL3_ADDRESS 350
#define DIAL4_ADDRESS 400
#define DIAL5_ADDRESS 450
#define DIAL6_ADDRESS 500
#define DIAL7_ADDRESS 550
#define DIAL8_ADDRESS 600
#define DIAL9_ADDRESS 650
#define BUSY_MSG_ADDRESS 700
#define BUSY_MSG_LEN 80
#define LAST_ADDRESS 780
#define LISTEN_PORT 23 // Listen to this if not connected. Set to zero to disable.

#define FLASH_BUTTON 0 // GPIO0 (programming mode pin)
#define LED_PIN 16     // Status LED
#define LED_ESP_PIN 2
#define DCD_PIN 2 // DCD Carrier Status

#define RTS_PIN 13 // RTS Request to Send, connect to host's CTS pin RTS is DB9 PIN 7
#define CTS_PIN 15 // CTS Clear to Send, connect to host's RTS pin CTS is DB9 PIN 8

#define RING_INTERVAL 3000 // How often to print RING when having a new incoming connection (ms)

#define MAX_CMD_LENGTH 256 // Maximum length for AT command

#define LED_TIME 15 // How many ms to keep LED on at activity

// Telnet codes
#define DO 0xfd
#define WONT 0xfc
#define WILL 0xfb
#define DONT 0xfe

// #define DEBUG 1          // Print additional debug information to serial channel
#undef DEBUG

#pragma once
enum resultCodes_t
{
    R_OK,
    R_CONNECT,
    R_RING,
    R_NOCARRIER,
    R_ERROR,
    R_NONE,
    R_NODIALTONE,
    R_BUSY,
    R_NOANSWER
};

#pragma once
enum flowControl_t
{
    F_NONE,
    F_HARDWARE,
    F_SOFTWARE
};

#pragma once
enum pinPolarity_t
{
    P_INVERTED,
    P_NORMAL
}; // Is LOW (0) or HIGH (1) active?

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
void sendString(String msg);
void waitForSpace();
void welcome();
void waitForFirstInput();
String ipToString(IPAddress ip);
void hangUp();
void answerCall();
void handleIncomingConnection();
void handleFlowControl();
void handleCommandMode();
void restoreCommandModeIfDisconnected();

extern WiFiServer tcpServer;
extern WiFiClient tcpClient;
extern MDNSResponder mdns;
extern ppp_pcb *ppp;
extern struct netif ppp_netif;
extern String speedDials[10];
extern String ssid, password, busyMsg;
extern byte serialspeed;
extern String build;
extern String cmd;
extern bool cmdMode;
extern bool callConnected;
extern bool telnet;
extern bool verboseResults;
extern bool firmwareUpdating;
extern int tcpServerPort;
extern unsigned long lastRingMs;
extern unsigned long ledTime;
extern const int speedDialAddresses[];
extern const int bauds[9];
extern bool echo;
extern bool autoAnswer;
extern byte ringCount;
extern String resultCodes[];
extern unsigned long connectTime;
extern bool hex;
extern byte flowControl;
extern bool txPaused;
extern byte pinPolarity;
extern bool quietMode;