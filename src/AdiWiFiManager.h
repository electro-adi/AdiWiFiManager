#ifndef ADIWIFIMANAGER_h
#define ADIWIFIMANAGER_h

//ADI'S SUPER AWESOME WIFI NETWORK PROVISIONING AND FILE MANAGEMENT LIBRARY

#if __has_include("AdiWiFiManagerConfig.h")
  #include "AdiWiFiManagerConfig.h"
#endif

#if !defined(SD_ENABLED) && !defined(LittleFS_ENABLED)
#error "AdiWiFiManager: no filesystem selected. Create AdiWiFiManagerConfig.h in your sketch folder (copy AdiWiFiManagerConfig_example.h) and #define SD_ENABLED and/or LittleFS_ENABLED there."
#endif

#if !defined(ASSETS_LOCATION)
#warning "AdiWiFiManager: ASSETS_LOCATION not defined. Set it to SD or LittleFS in your AdiWiFiManagerConfig.h."
#endif

#ifndef ARDUINO_ARCH_ESP32
  #error "AdiWiFiManager only supports ESP32-based boards."
#endif

#if __has_include(<ESPAsyncWebServer.h>)
  #include <ESPAsyncWebServer.h>
#else
  #error "AdiWiFiManager requires the ESPAsyncWebServer library — install it via Library Manager or https://github.com/ESP32Async/ESPAsyncWebServer"
#endif

#if __has_include(<AsyncTCP.h>)
  #include <AsyncTCP.h>
#else
  #error "AdiWiFiManager requires the AsyncTCP library — install it via Library Manager or https://github.com/ESP32Async/AsyncTCP"
#endif

#ifndef HOSTNAME
  #define HOSTNAME "AdiWebServer"
#endif

#ifndef AP_SSID
  #define AP_SSID "ESP32-AP"
#endif

#ifndef AP_PASS
  #define AP_PASS "12345678"
#endif

// Maximum asset files the library scans for and keeps track of
#define MAX_ASSET_FILES 10

//Libraries

#include "Arduino.h"
#include <Preferences.h>
#include <WiFi.h>

#include <ESPmDNS.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "esp_system.h"
#include "esp_wifi_types.h"
#include "esp_chip_info.h"
#include "esp_wifi.h"

#include "stdlib_noniso.h"
#include <functional>
#include "StreamString.h"

#ifdef SD_ENABLED
  #include "SD.h"
#endif

#ifdef LittleFS_ENABLED
  #include <LittleFS.h>
#endif

#ifdef SPIFFS_ENABLED
  #include <SPIFFS.h>
#endif

#ifdef OTA_ENABLED
  #include <Update.h>
#endif

typedef void (*DebugLogCallback)(const char *message);
extern DebugLogCallback debugLogCallback;

inline void _DebugLog(const String &message) {
  if(debugLogCallback) debugLogCallback(message.c_str());
}

const char BUILD_[] = __DATE__ " " __TIME__;

class WiFiResult
{
  public:
    bool duplicate;
    String SSID;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t *BSSID;
    int32_t channel;
    bool isHidden;

  WiFiResult()
  {
  }
};
typedef int16_t wifi_ssid_count_t;

#define MAX_FILES 100

typedef struct
{
String filename;
String ftype;
String fsize;
} fileinfo;

extern DNSServer DNS;
extern AsyncWebServer server;
extern uint8_t wifi_status;
extern unsigned long last_scan;
extern String _ssid, _pass;
extern bool scan_now, scan_complete, connect_to_new_network;
extern String _update_error_str;
extern unsigned long _current_progress_size;
extern WiFiResult *wifiSSIDs;
extern wifi_ssid_count_t wifiSSIDCount;
extern fileinfo Filenames[MAX_FILES];
extern int numfiles;
extern String chosen_background, chosen_icon;

class AdiWiFiManager {
	private:

		Preferences _preferences;

		bool AP_MODE = false;
		bool wb_stays_active = false;
		bool webserver_running = false;

		String HTML_Header();
		void Handle_Home(AsyncWebServerRequest *request);
		void Handle_Wifi(AsyncWebServerRequest * request);
		void Handle_Wifi_List(AsyncWebServerRequest *request);
		void Handle_Wifi_Save(AsyncWebServerRequest * request);
		void _connectToWiFi(String ssid, String pass);
		void _connectToNewWiFi();
		bool WIFI_Scan();
		void WIFI_CopySSIDs(wifi_ssid_count_t n);
		
		String getRandomAssetFile(const String &folder);
		int getFileTypePriority(String filename, String ftype);

	#ifdef SD_ENABLED

		int  SD_countFilesInDirectory(String path);
		void SD_Directory(String path);
		void SD_createDirectoryRecursive(const String& path);
		void SD_deleteRecursive(String path);
		void Handle_SD_Dir(AsyncWebServerRequest * request);
		void Handle_SD_File_Upload(AsyncWebServerRequest *request);
		void on_SD_File_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
		void Handle_SD_File_Download(AsyncWebServerRequest *request);
		void Handle_SD_File_Delete(AsyncWebServerRequest *request);
		void Handle_SD_File_Rename(AsyncWebServerRequest *request);
		void Handle_SD_File_Move(AsyncWebServerRequest *request);
		void Handle_SD_Create_Folder(AsyncWebServerRequest *request);

	#endif

	#ifdef LittleFS_ENABLED

		int  LittleFS_countFilesInDirectory(String path);
		void LittleFS_Directory(String path);
		void LittleFS_createDirectoryRecursive(const String& path);
		void LittleFS_deleteRecursive(String path);
		void Handle_LittleFS_Dir(AsyncWebServerRequest * request);
		void Handle_LittleFS_File_Upload(AsyncWebServerRequest *request);
		void on_LittleFS_File_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
		void Handle_LittleFS_File_Download(AsyncWebServerRequest *request);
		void Handle_LittleFS_File_Delete(AsyncWebServerRequest *request);
		void Handle_LittleFS_File_Rename(AsyncWebServerRequest *request);
		void Handle_LittleFS_File_Move(AsyncWebServerRequest *request);
		void Handle_LittleFS_Create_Folder(AsyncWebServerRequest *request);
	
	#endif

	#ifdef SPIFFS_ENABLED
		void SPIFFS_Directory(String path);
		void Handle_SPIFFS_Dir(AsyncWebServerRequest * request);
		void Handle_SPIFFS_File_Upload(AsyncWebServerRequest *request);
		void on_SPIFFS_File_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
		void Handle_SPIFFS_File_Download(AsyncWebServerRequest *request);
		void Handle_SPIFFS_File_Delete(AsyncWebServerRequest *request);
		void Handle_SPIFFS_File_Rename(AsyncWebServerRequest *request);
	#endif

	#ifdef OTA_ENABLED
		void Handle_OTA(AsyncWebServerRequest *request);
	#endif
	

		String ConvBinUnits(uint64_t bytes, int resolution);
		String EncryptionType(wifi_auth_mode_t encryptionType);

		void Display_System_Info(AsyncWebServerRequest *request);
		void Handle_Page_Not_Found(AsyncWebServerRequest *request);

	public:

		void StartWebserver();
		void StopWebserver();

		void setDebugCallback(DebugLogCallback callback);
		void WB_StaysActive(bool WBstaysActive);
		void connectToWiFi(bool ap_on_fail, String sta_ssid, String sta_pass);
		wifi_ssid_count_t getScanResults(WiFiResult* &results);
		uint8_t getWiFiStatus();
		void loop();
		void disconnect();
		void eraseSavedWiFi();

};

#endif 