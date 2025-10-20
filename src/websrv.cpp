#include <Arduino.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "websrv.h"

#ifdef ESP32
#include <WebServer.h>
#include <Update.h>
#else // ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#endif

#ifdef ESP32
WebServer webServer(80);
// Note: ESP32 core libraries do not include a direct equivalent of ESP8266HTTPUpdateServer.
// Firmware updates on ESP32 are typically handled with a custom handler using the Update library.
// The setup for httpUpdater is omitted for ESP32 in this refactoring.
#else // ESP8266
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer webServer(80);
#endif

#include "websrv.h"

void handleLoadEEPROM();
void handleSaveEEPROM();

void handleWebServer()
{
  webServer.handleClient();
}

void webserverSetup()
{
  webServer.on("/", handleRoot);
  webServer.on("/api/commands/ath", handleWebHangUp);
  webServer.on("/api/commands/reboot", handleReboot);
  webServer.on("/api/get/status", handleGetStatus);
  webServer.on("/api/get/settings", handleGetSettings);
  webServer.on("/api/save/settings", handleUpdateSettings);
  webServer.on("/api/commands/update", handleUpdateFirmware);
  webServer.on("/api/commands/factory", handleFactoryDefaults);
  webServer.on("/api/commands/eeprom/load", handleLoadEEPROM);
  webServer.on("/api/commands/eeprom/save", handleSaveEEPROM);
  webServer.begin();
}

void handleUpdateSettings()
{
  if (webServer.hasArg("plain") == false)
  {
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(400, "application/json", "{\"error\":\"Body not received\"}");
    return;
  }
  String body = webServer.arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error)
  {
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  const char *s_ssid = doc["ssid"];
  const char *s_password = doc["password"];
  int s_serialSpeed = doc["serialSpeed"];
  int s_tcpServerPort = doc["tcpServerPort"];
  const char *s_busyMsg = doc["busyMsg"];
  const size_t baudCount = sizeof(bauds) / sizeof(bauds[0]);
  if (s_serialSpeed < 0 || static_cast<size_t>(s_serialSpeed) >= baudCount)
  {
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(400, "application/json", "{\"error\":\"Invalid serial speed\"}");
    return;
  }
  if (!s_ssid || !s_password || s_serialSpeed < 0 || static_cast<size_t>(s_serialSpeed) >= baudCount || s_tcpServerPort <= 0 || s_tcpServerPort > 65535 || !s_busyMsg)
  {
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(400, "application/json", "{\"error\":\"Invalid data\"}");
    return;
  }
  ssid = String(s_ssid);
  password = String(s_password);
  serialspeed = static_cast<byte>(s_serialSpeed);
  tcpServerPort = s_tcpServerPort;
  busyMsg = String(s_busyMsg);
  Serial.println("Received new settings from web interface.");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.println("Password: ********");
  Serial.print("Serial Speed: ");
  Serial.println(bauds[serialspeed]);
  Serial.print("TCP Server Port: ");
  Serial.println(tcpServerPort);
  Serial.print("BUSY Message: ");
  Serial.println(busyMsg);
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleUpdateFirmware()
{
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
  delay(1000);
  firmwareUpdating = true;
}

void handleFactoryDefaults()
{
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
  delay(500);
  defaultEEPROM();
  readSettings();
  sendResult(RES_OK);
}

void handleGetSettings()
{
  String json;
  json.reserve(180);
  json += "{";
  json += "\"echo\":\"" + String(echo) + "\",";
  json += "\"autoAnswer\":\"" + String(autoAnswer) + "\",";
  json += "\"serialSpeed\":\"" + String(serialspeed) + "\",";
  json += "\"ssid\":\"" + String(ssid) + "\",";
  json += "\"password\":\"" + String(password) + "\",";
  json += "\"busyMsg\":\"" + String(busyMsg) + "\",";
  json += "\"tcpServerPort\":\"" + String(tcpServerPort) + "\",";
  json += "\"telnet\":\"" + String(telnet) + "\",";
  json += "\"verboseResults\":\"" + String(verboseResults) + "\",";
  json += "\"flowControl\":\"" + String(flowControl) + "\",";
  json += "\"pinPolarity\":\"" + String(pinPolarity) + "\",";
  json += "\"quietMode\":\"" + String(quietMode) + "\"";
  json += "}";
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", json);
}

void handleGetSpeedDials()
{
  // Placeholder
}

void handleGetStatus()
{
  String json;
  json.reserve(220);
  json += "{";
  json += "\"wifiStatus\":\"" + getWifiStatus() + "\",";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"mac\":\"" + getMacAddress() + "\",";
  json += "\"ip\":\"" + ipToString(WiFi.localIP()) + "\",";
  json += "\"gateway\":\"" + ipToString(WiFi.gatewayIP()) + "\",";
  json += "\"subnet\":\"" + ipToString(WiFi.subnetMask()) + "\",";
  json += "\"tcpServerPort\":\"" + String(tcpServerPort) + "\",";
  json += "\"callStatus\":\"" + getCallStatus() + "\",";
  json += "\"callLength\":\"" + getCallLength() + "\",";
  json += "\"baud\":\"" + String(bauds[serialspeed]) + "\"";
  json += "}";
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", json);
}

String getWifiStatus()
{
  switch (WiFi.status())
  {
  case WL_CONNECTED:
    return "CONNECTED";
  case WL_IDLE_STATUS:
    return "OFFLINE";
  case WL_CONNECT_FAILED:
    return "CONNECT FAILED";
  case WL_NO_SSID_AVAIL:
    return "SSID UNAVAILABLE";
  case WL_CONNECTION_LOST:
    return "CONNECTION LOST";
  case WL_DISCONNECTED:
    return "DISCONNECTED";
  case WL_SCAN_COMPLETED:
    return "SCAN COMPLETED";
  default:
    return "ERROR";
  }
}

String getMacAddress()
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

String getCallStatus()
{
  if (callConnected)
  {
    #ifdef PPP_ENABLED
    return "CONNECTED TO " + String(ppp ? "PPP" : ipToString(tcpClient.remoteIP()));
    #endif
    #ifndef PPP_ENABLED
    return "CONNECTED TO " + String(ipToString(tcpClient.remoteIP()));
    #endif
  }
  return "NOT CONNECTED";
}

String getCallLength()
{
  if (callConnected)
  {
    return connectTimeString();
  }
  return "00:00:00";
}

void handleWebHangUp()
{
  hangUp();
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleLoadEEPROM() {
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
  delay(500);
  readSettings();
  Serial.println(F("Settings reloaded from EEPROM (requested from web)"));
}

void handleSaveEEPROM() {
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
  delay(500);
  writeSettings();
  Serial.println(F("Settings saved to EEPROM (requested from web)"));
}

void handleReboot()
{
  Serial.println(F("Rebooting... (requested from web)"));
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", "{\"status\":\"success\"}");
  delay(500);
  ESP.restart();
}

void handleRoot()
{
  constexpr size_t bufSize = 128;
  char buffer[bufSize + 1];
  const size_t len = sizeof(MAIN_page) - 1;
  size_t sent = 0;

  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "text/html", "");

  while (sent < len)
  {
    size_t chunkSize = (len - sent) < bufSize ? (len - sent) : bufSize;
    for (size_t i = 0; i < chunkSize; i++)
    {
      buffer[i] = static_cast<char>(pgm_read_byte(&(MAIN_page[sent + i])));
    }
    buffer[chunkSize] = '\0';
    webServer.sendContent(buffer);
    sent += chunkSize;
  }
  webServer.sendContent(""); // finalize chunked response
  delay(50);
}

void redirectToRoot()
{
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}
