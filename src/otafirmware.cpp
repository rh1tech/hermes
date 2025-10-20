#include <Arduino.h>
#include "globals.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

ESP8266WiFiMulti WiFiMulti;
#define UPDATE_CLASS ESPhttpUpdate
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

WiFiMulti WiFiMulti;
#define UPDATE_CLASS httpUpdate
#endif

String getLatestVersion()
{
  WiFiClient client;
  HTTPClient http;
  http.begin(client, hermes_version_url);
  int httpResponseCode = http.GET();
  String latestVersion = "unset";
  if (httpResponseCode > 0)
  {
    latestVersion = http.getString();
    hermes_version.trim();
    latestVersion.trim();
  }
  else
  {
    Serial.println("Error checking for firmware update");
    Serial.print("Code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return latestVersion;
}

void check_for_firmware_update()
{
  String latestVersion = getLatestVersion();
  if (hermes_version != latestVersion)
  {
    Serial.println("");
    Serial.print("New Hermes firmware version is available: ");
    Serial.println(latestVersion);
    Serial.println("Command to update: AT$FW");
  }
}

void update_started()
{
  Serial.println("Firmware update process started");
}

void update_finished()
{
  firmwareUpdating = false;
  Serial.println("Firmware update process finished");
  Serial.println("Rebooting...");
  Serial.println("");
}

void update_progress(int cur, int total)
{
  Serial.printf("Downloading firmware: %d of %d byte(s)...\n", cur, total);
}

void update_error(int err)
{
  Serial.printf("Firmware update fatal error, code %d\n", err);
}

void handleOTAFirmware()
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;
    UPDATE_CLASS.onStart(update_started);
    UPDATE_CLASS.onEnd(update_finished);
    UPDATE_CLASS.onProgress(update_progress);
    UPDATE_CLASS.onError(update_error);
    String latestVersion = getLatestVersion();
    String modifiedVersion = latestVersion;
    modifiedVersion.replace('.', '_');
    t_httpUpdate_return ret = UPDATE_CLASS.update(client, "http://protea.rh1.tech/ota/hermes_" + modifiedVersion + ".bin");
    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED, error (%d): %s\n", UPDATE_CLASS.getLastError(), UPDATE_CLASS.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
    }
  }
}
