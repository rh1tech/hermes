#include <Arduino.h>
#include <globals.h>

#ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
    #if NAPT_SUPPORTED
        #include <lwip/napt.h>
        #include <netif/ppp/ppp.h>
        #include <netif/ppp/pppos.h>
    #endif
    #include <lwip/dns.h>
    #include <lwip/netif.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <ESPmDNS.h>
    // ESP32 doesn't support PPP in Arduino framework by default
#endif

#include <IPAddress.h>
#include <EEPROM.h>

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

#ifdef ESP8266
    #if NAPT_SUPPORTED
        u32_t ppp_output_cb(ppp_pcb *pcb, unsigned char *data, u32_t len, void *ctx);
        void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx);
    #endif
#endif

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

#ifdef ESP8266
    MDNSResponder mdns;
    #if NAPT_SUPPORTED
        ppp_pcb *ppp;
        struct netif ppp_netif;
    #else
        void *ppp = NULL; // Placeholder
    #endif
#elif defined(ESP32)
    void *ppp = NULL; // ESP32 doesn't support PPP in this context
#endif

String speedDials[10];
String ssid, password, busyMsg;
byte serialspeed;
String hermes_version = "1.00";
String hermes_version_url = "http://protea.rh1.tech/ota/hermes.txt";
String cmd = "";            // Gather a new AT command to this string from serial
bool cmdMode = true;        // Are we in AT command mode or connected mode
bool callConnected = false; // Are we currently in a call
bool telnet = false;        // Is telnet control code handling enabled
bool sshConnected = false; // Are we currently in a SSH session
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
        case RES_CONNECT:
                resultMessage = String(resultCodes[RES_CONNECT]) + " " + String(bauds[serialspeed]);
                break;
        case RES_NOCARRIER:
                resultMessage = String(resultCodes[RES_NOCARRIER]) + " (" + connectTimeString() + ")";
                break;
        default:
                resultMessage = String(resultCodes[resultCode]);
                break;
        }

        const char *colorStart = "\x1b[0m";
        switch (resultCode)
        {
        case RES_OK:
        case RES_CONNECT:
                colorStart = "\x1b[30;42m"; // black on green
                break;
        case RES_RING:
        case RES_NOCARRIER:
        case RES_NODIALTONE:
        case RES_BUSY:
        case RES_NOANSWER:
                colorStart = "\x1b[30;43m"; // black on yellow
                break;
        case RES_ERROR:
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
        
        #ifdef ESP8266
            Serial.println("\016x\017 ESP8266 Firmware (Hermes) version \x1b[32m" + hermes_version + "\x1b[0m                \016x");
        #elif defined(ESP32)
            Serial.println("\016x\017 ESP32 Firmware (Hermes) version \x1b[32m" + hermes_version + "\x1b[0m                  \016x");
        #endif
        
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
#ifdef ESP8266
    #if NAPT_SUPPORTED
        if (ppp)
        {
                ppp_close(ppp, 0);
        }
        else
    #endif
#endif
        {
                tcpClient.stop();
        }
        callConnected = false;
        setCarrierDCDPin(callConnected);
        sendResult(RES_NOCARRIER);
        connectTime = 0;
}

void answerCall()
{
        tcpClient = tcpServer.accept();
        tcpClient.setNoDelay(true); // try to disable naggle
        sendResult(RES_CONNECT);
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
                        sendResult(RES_RING);
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
                sendResult(RES_CONNECT);
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
		
		// Echo the original character BEFORE any transformation
		// This ensures Win1251 and other 8-bit characters are echoed correctly
		if (echo == true)
		{
			Serial.write(chr);
		}
		
		// Transform uppercase letters for command parsing only
		// This does NOT affect what was echoed above
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
			// Backspace character was already echoed above
		}
		else
		{
			if (cmd.length() < MAX_CMD_LENGTH)
				cmd.concat(chr);
			// Character was already echoed above
			if (hex)
			{
				Serial.print(chr, HEX);
			}
		}
	}
}void restoreCommandModeIfDisconnected()
{
        bool pppConnected = false;
#ifdef ESP8266
    #if NAPT_SUPPORTED
        pppConnected = (ppp != NULL);
    #endif
#endif
        
        if ((!tcpClient.connected() && !pppConnected) && (cmdMode == false) && callConnected == true)
        {
                cmdMode = true;
                sendResult(RES_NOCARRIER);
                connectTime = 0;
                callConnected = false;
                setCarrierDCDPin(callConnected);
        }
}