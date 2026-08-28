#ifndef ADIWIFIMANAGER_h
#define ADIWIFIMANAGER_h

//ADI'S SUPER AWESOME WIFI NETWORK PROVISIONING AND FILE MANAGEMENT LIBRARY

//#define LittleFS_ENABLED
#define SD_ENABLED

#define MAX_SAVED_WIFI_STATIONS 5 //number of slots in eeprom to save wifi networks

#define DEFAULT_AP_SSID "AdiWebServer"
#define DEFAULT_AP_PASS "12345678"
#define DEFAULT_HOSTNAME "AdiWebServer"

//This is the location where the assets for the webserver are stored. it can be either SD or LittleFS
#define ASSETS_LOCATION SD
#define MAX_ASSET_FILES 10

/*
Needed Folders:

	/Assets
		/Backgrounds (background images, must be jpg, jpeg, png or bmp)
		/MainIcons (homepage icons, must be jpg, gif, png or bmp)
		/OtherIcons (wifi icons, file explorer icons etc)

*/

#define HOMEPAGE_H1 "Doorlock Webserver"
#define HOMEPAGE_H2 "AdiWebServer"
#define HOMEPAGE_H3 "HEHEHEHEHEHE"

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
#include "Update.h"
#include "StreamString.h"

#ifdef SD_ENABLED
  #include "SD.h"
#endif

#ifdef LittleFS_ENABLED
  #include <LittleFS.h>
#endif

typedef void (*DebugLogCallback)(const char *message);
extern DebugLogCallback debugLogCallback;

inline void _DebugLog(const String &message) {
  if (debugLogCallback) debugLogCallback(message.c_str());
}

//#define DEBUG_PRINT(x) _DebugLog(x);
//#define DEBUG_PRINTLN(x) _DebugLog(String(x) + "\n");

#if !defined(SD_ENABLED) && !defined(LittleFS_ENABLED)
#error "AdiWiFiManager needs SD_ENABLED or LittleFS_ENABLED defined to store webserver assets"
#endif

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
		String wifi_ap_ssid = DEFAULT_AP_SSID;
		String wifi_ap_pass = DEFAULT_AP_PASS;
		String _hostname = DEFAULT_HOSTNAME;

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

		void Handle_OTA(AsyncWebServerRequest *request);

		String ConvBinUnits(uint64_t bytes, int resolution);
		String EncryptionType(wifi_auth_mode_t encryptionType);

		void Display_System_Info(AsyncWebServerRequest *request);
		void Handle_Page_Not_Found(AsyncWebServerRequest *request);

	public:

		void StartWebserver();
		void StopWebserver();

		void setDebugCallback(DebugLogCallback callback);
		void WB_StaysActive(bool WBstaysActive);
		void setAP_ssid_pass(String ssid, String pass);
		void setHostname(String hostname);
		void connectToWiFi(bool ap_on_fail, String sta_ssid, String sta_pass);
		wifi_ssid_count_t getScanResults(WiFiResult* &results);
		uint8_t getWiFiStatus();
		void loop();
		void disconnect();
		void eraseSavedWiFi();

};

#endif 