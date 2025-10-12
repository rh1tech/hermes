#include <Arduino.h>
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
  sendResult(R_OK);

  mdns.begin("ProteaWiFi", WiFi.localIP());
}

void connectWiFi()
{
  if (ssid == "" || password == "")
  {
    Serial.println("Please set Wi-Fi network SSID and password first. Enter AT? for help.");
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
    if (ppp)
    {
      Serial.print("Connected to PPP");
    }
    else
    {
      Serial.print("Connected to ");
      Serial.println(ipToString(tcpClient.remoteIP()));
    }
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
