#ifndef ADIWIFIMANAGER_CONFIG_H
#define ADIWIFIMANAGER_CONFIG_H

// Copy this file into your sketch folder as "AdiWiFiManagerConfig.h" and edit it there.

// Uncomment the file systems that you want to access through the web file manager
#define SD_ENABLED
//#define SD_MMC_ENABLED
//#define LittleFS_ENABLED
//#define SPIFFS_ENABLED

/*
Assets folder structure:

	/Assets
		/Backgrounds (background images, must be jpg, jpeg, png or bmp)
		/MainIcons (homepage icons, must be jpg, gif, png or bmp)
		/OtherIcons (wifi icons, file explorer icons etc)

*/

// This is the location where the assets for the webserver are stored. it can be either SD/SD_MMC or LittleFS (can't use spiffs for this)
#define ASSETS_LOCATION SD

// Number of slots in eeprom to save wifi networks
#define MAX_SAVED_WIFI_STATIONS 5

// Set the hostname for the device. For Mdns http://<hostname>.local
#define HOSTNAME "ESP32"

// Access Point SSID and Password
#define AP_SSID "ESP32-AP"
#define AP_PASS "12345678"

// Homepage heading texts
#define HOMEPAGE_H1 "AdiWebServer"
#define HOMEPAGE_H2 "Coolest Webserver Ever"
#define HOMEPAGE_H3 "BEHEHEHEHEHEHHEHE"

// Uncomment this is if you want to enable OTA functionality
#define OTA_ENABLED


#endif