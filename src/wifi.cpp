#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#error "Unsupported platform"
#endif
#include "globals.h"

namespace
{
  const uint8_t MAX_CONNECT_ATTEMPTS = 25;
  const uint16_t CONNECT_RETRY_DELAY_MS = 500;

  const char *wifiStatusToString(wl_status_t st)
  {
    switch (st)
    {
    case WL_IDLE_STATUS:
      return "OFFLINE";
    case WL_NO_SSID_AVAIL:
      return "SSID UNAVAILABLE";
    case WL_SCAN_COMPLETED:
      return "SCAN COMPLETED";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
    }
  }

  void printMac()
  {
    byte mac[6];
    WiFi.macAddress(mac);
    for (uint8_t i = 0; i < 6; ++i)
    {
      if (mac[i] < 0x10)
        Serial.print('0');
      Serial.print(mac[i], HEX);
      if (i < 5)
        Serial.print(':');
    }
  }
}

void wifiSetup()
{
  if (tcpServerPort > 0)
    tcpServer.begin();

  WiFi.mode(WIFI_STA);

  const char *hostname = "PROTEA";
  WiFi.setHostname(hostname);

  connectWiFi();
  sendResult(RES_OK);

#ifdef ESP8266
  mdns.begin("ProteaWiFi", WiFi.localIP());
#elif defined(ESP32)
  MDNS.begin("ProteaWiFi");
#endif
}

void connectWiFi()
{
  if (ssid.isEmpty() || ssid[0] == 0xFF || password.isEmpty())
  {
    Serial.println("Welcome to to Hermes, the Protea Wi-Fi to Serial Modem!");
    Serial.println("Default baud rate is 9600 bps. You can change it using the command: \x1b[36mAT$SB=N\x1b[0m,");
    Serial.println("where \x1b[36mN\x1b[0m is one of: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200.");  
    Serial.println("You need to connect to a Wi-Fi network before making calls.");
    Serial.println("To do this, set the SSID and password using the following commands:");
    Serial.println("\x1b[36mAT$SSID=YOUR_WIFI_NETWORK\x1b[0m");
    Serial.println("\x1b[36mAT$PASS=YOUR_WIFI_PASSWORD\x1b[0m");
    Serial.println("When done, connect using the command: \x1b[36mATC1\x1b[0m");
    Serial.println("Enter \x1b[36mAT?\x1b[0m for help. When done, save your settings by entering: \x1b[36mAT&W\x1b[0m");
    return;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid)
  {
    Serial.println("Already connected.");
    return;
  }

  WiFi.disconnect();
  delay(100);
  Serial.print("Checking Wi-Fi signal strength (SSID: ");
  Serial.print(ssid);
  Serial.println(")...");

  int16_t rssi = 0;
  bool found = false;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i)
  {
    if (WiFi.SSID(i) == ssid)
    {
      rssi = WiFi.RSSI(i);
      found = true;
      break;
    }
  }
  WiFi.scanDelete();
  if (found)
  {
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
  else
  {
    Serial.println("Signal strength: unknown (SSID not found)");
  }

  Serial.print("\nConnecting to SSID: ");
  Serial.print(ssid);

  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());

  uint8_t attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt++ < MAX_CONNECT_ATTEMPTS)
  {
    Serial.print('.');
    delay(CONNECT_RETRY_DELAY_MS);
    yield();
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Could not connect to ");
    Serial.print(ssid);
    Serial.println(". Check SSID, password and signal strength.");
    WiFi.disconnect();
    return;
  }

  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  check_for_firmware_update();
}

void disconnectWiFi()
{
  WiFi.disconnect();
}

void displayNetworkStatus()
{
  wl_status_t st = WiFi.status();
  Serial.print("Wi-Fi Status: ");
  Serial.println(wifiStatusToString(st));
  yield();

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("MAC Address: ");
  printMac();
  Serial.println();
  yield();

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  yield();

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  yield();

  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  yield();

  Serial.print("Server Port: ");
  Serial.println(tcpServerPort);
  yield();

  Serial.print("URL: http://");
  Serial.println(WiFi.localIP());
  yield();

  Serial.print("Call Status: ");
  if (callConnected)
  {
#ifdef PPP_ENABLED
    if (ppp)
    {
      Serial.print("Connected to PPP");
    }
    else
    {
      Serial.print("Connected to ");
      Serial.println(ipToString(tcpClient.remoteIP()));
    }
#endif
#ifndef PPP_ENABLED
    Serial.print("Connected to ");
    Serial.println(ipToString(tcpClient.remoteIP()));
#endif
    yield();
    Serial.print("Call length: ");
    Serial.println(connectTimeString());
  }
  else
  {
    Serial.println("Not connected");
  }
  yield();
}
