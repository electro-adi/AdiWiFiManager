# AdiWiFiManager

An Arduino library for ESP32 that handles WiFi provisioning, a saved network manager, a web based file manager for SD and/or LittleFS, OTA firmware updates, and a system info page.

## Features

- **WiFi provisioning** — falls back to a SoftAP with a captive portal when it can't connect to a saved network, so you can join the AP and pick a network from your phone/laptop without hardcoding credentials.
- **Saved networks** — stores up to `MAX_SAVED_WIFI_STATIONS` SSID/password pairs in NVS (via `Preferences`), and tries them automatically on boot.
- **Web file manager** — browse, upload (including whole folders), download, rename, move, and batch delete files on SD and/or LittleFS, with per folder navigation and multi select for delete and move.
- **OTA updates** — flash new firmware from the web UI.
- **System info page** — chip info, memory, filesystem usage, current WiFi details (SSID, IP, RSSI, encryption type), and reboot the device all from a single page.
- **Themeable homepage** — background image and icon are picked at random from a folder you control, rerolled on every page load.

## Requirements

- **Board:** ESP32
- **Arduino libraries**
  - [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)  (Tested on version 3.9.3)
  - [AsyncTCP](https://github.com/ESP32Async/AsyncTCP) (Tested on version 3.4.9)
- **Storage:** an SD card and/or the ESP32's internal flash formatted as LittleFS. At least one is required.

## Installation

1. Copy the `AdiWiFiManager` folder into your `libraries/` folder (Arduino IDE) or `lib/` folder (PlatformIO).
2. Install the two dependencies listed above.
3. Set up your config file (below) before your first build.

## Configuration

The library needs to know which filesystem(s) to use before it will compile — this lives in a config file **in your own sketch**, not inside the library, so different projects using this library can each have their own settings.

Copy `AdiWiFiManagerConfig_example.h` into your sketch folder, rename it to `AdiWiFiManagerConfig.h`, and edit it there:

For Platform.io users, the `AdiWiFiManagerConfig.h` file should be inside of the include folder and `-I include` must be added to the build flags section within platform.ini.

```cpp
// AdiWiFiManagerConfig.h — lives in your sketch folder

#define SD_ENABLED
//#define LittleFS_ENABLED        // enable one or both

#define ASSETS_LOCATION SD        // must match one of the filesystems above

#define MAX_SAVED_WIFI_STATIONS 5
#define DEFAULT_AP_SSID   "AdiWebServer"
#define DEFAULT_AP_PASS   "12345678"
#define DEFAULT_HOSTNAME  "AdiWebServer"
#define MAX_ASSET_FILES   10

#define HOMEPAGE_H1 "AdiWebServer"
#define HOMEPAGE_H2 "AdiWebServer"
#define HOMEPAGE_H3 ""
```

If you skip this file entirely, or leave out `SD_ENABLED`/`LittleFS_ENABLED`/`ASSETS_LOCATION`, the build fails with a message telling you exactly what to add.

### Asset folder layout

Wherever `ASSETS_LOCATION` points, the library expects this structure to exist (create it once, upload it via `mkspiffs`/`mklittlefs`/an SD card writer, or through the file manager itself once the device is up):

```
/Assets
  /Backgrounds     — homepage background images (jpg, jpeg, png, bmp)
  /MainIcons       — homepage icon, randomized on each load (jpg, jpeg, png, gif, bmp)
  /OtherIcons      — UI icons used by the WiFi list and file manager (signal strength, file type icons, etc.)
```

> **Note on LittleFS vs SPIFFS:** this library expects LittleFS, not the older SPIFFS. LittleFS supports real nested folders and much longer paths; SPIFFS caps full paths at ~31 characters and has no real directory support, which will silently break asset lookups and the file manager's folder view if used instead.

## Quick start

```cpp
#include <Arduino.h>
#include "AdiWiFiManager.h"

AdiWiFiManager WiFiManager;

void wifiDebugLog(const char *message) {
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
  }

  WiFiManager.setDebugCallback(wifiDebugLog);
  WiFiManager.setAP_ssid_pass("ESP32", "12345678");
  WiFiManager.setHostname("ESP32");

  // Keep the webserver running even after a successful WiFi connection
  WiFiManager.WB_StaysActive(true);

  // Try to connect using saved networks; fall back to AP mode if none work
  WiFiManager.connectToWiFi(true, "", "");

  if (WiFiManager.getWiFiStatus() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  }
}

void loop() {
  WiFiManager.loop();
}
```

On first boot (no saved networks), the device starts a SoftAP named after whatever you passed to `setAP_ssid_pass()`. Connect to it, and either the captive portal prompt or a browser navigating to the device's IP will show the provisioning page, where you can pick a network and enter its password.

## Web pages

Once the webserver is running (either in AP mode for provisioning, or in STA mode if `WB_StaysActive(true)` was set), these pages are available at the device's IP:

| Path | Purpose |
|---|---|
| `/` | Homepage |
| `/wifi` | Scan for networks, connect, and view current connection/AP status |
| `/sd_dir`, `/littlefs_dir` | File manager for SD / LittleFS |
| `/update` | OTA firmware upload |
| `/system` | Chip, memory, filesystem, and WiFi diagnostics |

## API reference

```cpp
void StartWebserver();
void StopWebserver();
```
Start/stop the web server, DNS captive portal responder, and mDNS responder. `connectToWiFi()` calls `StartWebserver()` for you

```cpp
void setDebugCallback(DebugLogCallback callback);
```
Registers a function to receive the library's internal log messages, instead of them going nowhere. Signature: `void yourFunction(const char *message);`. Route it to `Serial`, a display, storage — whatever you want. No callback means no output.

```cpp
void WB_StaysActive(bool WBstaysActive);
```
If `true`, the webserver keeps running after a successful STA connection (so you can still reach the file manager, OTA, etc. once online). If `false`, the webserver shuts down once connected and the device just sits on WiFi.

```cpp
void connectToWiFi(bool ap_on_fail, String sta_ssid, String sta_pass);
```
Attempts to connect. Pass a specific `sta_ssid`/`sta_pass` to connect to a particular network, or empty strings to try saved networks instead. If every attempt fails and `ap_on_fail` is `true`, falls back to SoftAP + captive portal for provisioning.

```cpp
uint8_t getWiFiStatus();
```
Returns the current `wl_status_t` (e.g. compare against `WL_CONNECTED`).

```cpp
wifi_ssid_count_t getScanResults(WiFiResult* &results);
```
Gives you a pointer to the most recent scan results and returns the count. Useful if you want to show nearby networks on your own display. Don't hold onto the pointer across a `loop()` call — a new scan can reallocate it.

```cpp
void loop();
```
Call this every iteration of your sketch's `loop()`. Drives the captive portal DNS responder, scan polling, and applying newly submitted WiFi credentials from the web UI.

```cpp
void disconnect();
void eraseSavedWiFi();
```
Disconnect from WiFi without touching the webserver, or wipe all saved network credentials from NVS.

### `WiFiResult`

```cpp
class WiFiResult {
  public:
    bool duplicate;
    String SSID;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t *BSSID;
    int32_t channel;
    bool isHidden;
};
```
One entry per scanned network, returned via `getScanResults()`. `duplicate` marks repeated SSIDs from multiple access points of the same network (mesh setups, extenders) so you can skip them when listing.

## Known limitations

- SD and LittleFS share the same asset folder convention, but only one (`ASSETS_LOCATION`) serves the homepage's background/icons at a time — you can still browse both filesystems in the file manager if both are enabled.
- The file manager's move/delete operations run one HTTP request per selected file — batch operations on a large number of files will take a few seconds.
- The default AP password (`12345678`) and unauthenticated OTA endpoint are fine for a private home network but aren't hardened against untrusted networks — change the AP password via `setAP_ssid_pass()`, and don't expose this device's webserver to the public internet.