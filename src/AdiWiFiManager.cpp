
#include "AdiWiFiManager.h"

DNSServer DNS;
AsyncWebServer server(80);

uint8_t wifi_status = 0;
unsigned long last_scan = 0;
String _ssid, _pass;
bool scan_now = false, scan_complete = false, connect_to_new_network = false;
String _update_error_str = "";
unsigned long _current_progress_size = 0;
WiFiResult *wifiSSIDs = nullptr;
wifi_ssid_count_t wifiSSIDCount = 0;
fileinfo Filenames[MAX_FILES] __attribute__((section(".ext_ram.bss")));
int numfiles = 0;
String chosen_background = "", chosen_icon = "";
DebugLogCallback debugLogCallback = nullptr;

String AdiWiFiManager::HTML_Header() {

  String FS_BUTTONS = "";

  #ifdef SD_ENABLED
    FS_BUTTONS += "<a href='/sd_dir'>SD Files</a>";
  #endif

  #ifdef LittleFS_ENABLED
    FS_BUTTONS += "<a href='/littlefs_dir'>LittleFS Files</a>";
  #endif

  chosen_background = getRandomAssetFile("/Assets/Backgrounds");

  return R"rawliteral(
  <!DOCTYPE html>
  <html lang='en'>
  <head>
    <title>)rawliteral" + String(HOMEPAGE_H1) + R"rawliteral(</title>
    <meta charset='UTF-8'>
    <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no"/>
    <style>

    body {
      opacity: 0;
      transition: opacity 0.4s ease-in-out;
      max-width: 75em;
      margin: auto;
      font-family: Arial, Helvetica, sans-serif;
      font-size: 16px;
      color: #eeeeee;
      text-align: center;

      background-image: url('/asset_bg?f=)rawliteral" + chosen_background + R"rawliteral(');
      background-size: cover;
      background-repeat: no-repeat;
      background-position: center;
      background-attachment: fixed;
    }

    body.fade-in {
      opacity: 1;
    }

    .topnav {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      
      padding: 0.5em 1em;
      margin: 1em auto;
      width: fit-content;
      
      color: white;
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 1em;
      margin-bottom: 2em;
    }
    
    .topnav a {
      color: white;
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      text-decoration: none;
      font-size: 1.1em;
      transition: background 0.3s ease;
      border-radius: 0.75em;
      font-weight: bold;
    }
    
    .topnav a:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    .popup-overlay {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.7);
      z-index: 999;
      animation: fadeIn 0.3s ease-out;
    }
    
    .popup-overlay.show {
      display: flex;
      justify-content: center;
      align-items: center;
    }
    
    .popup-content {
      background: linear-gradient(135deg, rgba(60, 60, 80, 0.95), rgba(40, 40, 60, 0.95));
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 1.5em;
      padding: 2.5em;
      max-width: 420px;
      min-width: 320px;
      text-align: center;
      backdrop-filter: blur(10px);
      box-shadow: 0 10px 40px rgba(0, 0, 0, 0.9);
      animation: slideUp 0.3s ease-out;
    }
    
    .popup-content h2 {
      color: #ffffff;
      margin: 0 0 1.2em 0;
      font-size: 1.5em;
      font-weight: 600;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
      background: linear-gradient(135deg, #fff, #ccc);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }
    
    .popup-content p {
      color: #e8e8e8;
      margin: 0 0 2em 0;
      font-size: 1em;
      line-height: 1.5;
    }
    
    .popup-buttons {
      display: flex;
      gap: 1em;
      margin-bottom: 0;
      justify-content: center;
    }
    
    .popup-btn {
      padding: 0.85em 2em;
      border: none;
      border-radius: 0.75em;
      font-size: 1em;
      cursor: pointer;
      transition: all 0.3s ease;
      font-weight: bold;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
    }
    
    .popup-btn-confirm {
      background: linear-gradient(135deg, #00c6ff, #0072ff);
      color: white;
    }
    
    .popup-btn-confirm:hover:not(:disabled) {
      background: linear-gradient(135deg, #00d4ff, #0080ff);
      transform: translateY(-2px);
      box-shadow: 0 6px 16px rgba(0, 114, 255, 0.4);
    }
    
    .popup-btn-confirm:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    
    .popup-btn-cancel {
      background: rgba(255, 255, 255, 0.15);
      color: #ffffff;
    }
    
    .popup-btn-cancel:hover:not(:disabled) {
      background: rgba(255, 255, 255, 0.25);
      transform: translateY(-2px);
      box-shadow: 0 6px 16px rgba(255, 255, 255, 0.2);
    }
    
    .popup-btn-cancel:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    
    .popup-loading {
      display: none;
      margin-top: 1.5em;
      padding-top: 1.5em;
      border-top: 1px solid rgba(255, 255, 255, 0.1);
      color: #c8c8c8;
      font-size: 0.95em;
    }
    
    .popup-loading.show {
      display: block;
    }

    .spinner {
      display: inline-block;
      width: 16px;
      height: 16px;
      border: 2px solid rgba(255, 255, 255, 0.3);
      border-top-color: #00c6ff;
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
      margin-right: 0.5em;
      vertical-align: middle;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }
    
    @keyframes slideUp {
      from { 
        opacity: 0;
        transform: translateY(40px) scale(0.95);
      }
      to { 
        opacity: 1;
        transform: translateY(0) scale(1);
      }
    }

    @keyframes spin {
      to { transform: rotate(360deg); }
    }

    </style>
  </head>
  <body>
    <div id="popupOverlay" class="popup-overlay">
      <div class="popup-content">
        <h2 id="popupTitle">Confirm Action</h2>
        <p id="popupMessage">Are you sure?</p>
        <div class="popup-buttons">
          <button class="popup-btn popup-btn-confirm" id="popupConfirm">Confirm</button>
          <button class="popup-btn popup-btn-cancel" id="popupCancel">Cancel</button>
        </div>
        <div class="popup-loading" id="popupLoading">
          <span class="spinner"></span>
          Processing...
        </div>
      </div>
    </div>

    <script>
      let currentAction = null;

      function showPopup(action, title, message) {
        currentAction = action;
        document.getElementById('popupTitle').textContent = title;
        document.getElementById('popupMessage').textContent = message;
        document.getElementById('popupOverlay').classList.add('show');
        document.getElementById('popupLoading').classList.remove('show');
        document.getElementById('popupConfirm').style.display = 'block';
        document.getElementById('popupCancel').textContent = 'Cancel';
      }

      function closePopup() {
        document.getElementById('popupOverlay').classList.remove('show');
        currentAction = null;
      }

      document.getElementById('popupConfirm').addEventListener('click', async function() {
        if (!currentAction) return;
        
        document.getElementById('popupLoading').classList.add('show');
        document.getElementById('popupConfirm').disabled = true;
        document.getElementById('popupCancel').disabled = true;
        
        try {
          const response = await fetch(currentAction, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'}
          });
          
          if (response.ok) {
            document.getElementById('popupMessage').textContent = 'Action completed successfully!';
            document.getElementById('popupConfirm').style.display = 'none';
            document.getElementById('popupCancel').textContent = 'Close';
            document.getElementById('popupCancel').disabled = false;
            document.getElementById('popupLoading').classList.remove('show');
            
            if (currentAction === '/reboot') {
              setTimeout(() => closePopup(), 2000);
            }
          }
        } catch (error) {
          document.getElementById('popupMessage').textContent = 'Error: ' + error.message;
        } finally {
          document.getElementById('popupLoading').classList.remove('show');
        }
      });

      document.getElementById('popupCancel').addEventListener('click', closePopup);
      document.getElementById('popupOverlay').addEventListener('click', function(e) {
        if (e.target === this) closePopup();
      });

      document.addEventListener("DOMContentLoaded", () => {
        document.body.classList.add("fade-in");

        document.querySelectorAll("a").forEach(link => {
          const href = link.getAttribute("href");

          if (
            href &&
            !href.startsWith("http") &&
            !href.startsWith("#") &&
            !href.startsWith("javascript") &&
            !href.includes("?") &&
            !link.hasAttribute("download") &&
            link.target !== "_blank"
          ) {
            link.addEventListener("click", function(e) {
              e.preventDefault();
              document.body.classList.remove("fade-in");
              document.body.style.opacity = 0;
              setTimeout(() => {
                window.location.href = this.href;
              }, 300);
            });
          }
        });
      });
    </script>

    <div class='topnav'>
      <a href='/'>Home</a>
      <a href='/wifi'>Wi-Fi</a>
      )rawliteral" + String(FS_BUTTONS) + R"rawliteral(
      <a href='/update'>OTA</a>
      <a href='/system'>System</a>
    </div>
  </body>
  )rawliteral";
}

void AdiWiFiManager::Handle_Home(AsyncWebServerRequest *request) {

  chosen_icon = getRandomAssetFile("/Assets/MainIcons");

  String page = HTML_Header();
  page += R"rawliteral(
    <style>
      .home_container {
        max-width: 800px;
        margin: 4em auto;
        background: rgba(38, 38, 38, 0.5);
        border-radius: 1.5em;
        backdrop-filter: blur(8px);
        box-shadow: 0 6px 20px rgba(0, 0, 0, 0.6);
        padding: 2.5em;
        text-align: center;
        animation: fadeIn 1.2s ease-out;
      }

      .home_container img {
        width: 100px;
        height: auto;
        margin: 1em 0;
        filter: drop-shadow(0 0 6px rgba(255,255,255,0.2));
        transition: transform 0.3s ease;
      }

      .home_container img:hover {
        transform: scale(1.1);
      }

      .home_container h1 {
        font-size: 2.5em;
        margin-bottom: 0.4em;
        color: #ffffff;
        text-shadow: 2px 2px 6px rgba(0, 0, 0, 0.7);
      }

      .home_container h2 {
        font-size: 1.3em;
        font-weight: 400;
        color: #dddddd;
        margin-bottom: 1em;
        text-shadow: 1px 1px 3px rgba(0, 0, 0, 0.6);
      }

      .home_container h3 {
        margin-top: 2em;
        font-size: 1.1em;
        color: #eeeeee;
        opacity: 0.8;
        letter-spacing: 1px;
      }

      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(20px); }
        to { opacity: 1; transform: translateY(0); }
      }
    </style>

    <div class="home_container">
      <h1>)rawliteral" + String(HOMEPAGE_H1) + R"rawliteral(</h1>
      <h2>)rawliteral" + String(HOMEPAGE_H2) + R"rawliteral(</h2>
      <img src='/asset_icon?f=)rawliteral" + chosen_icon + R"rawliteral(' alt='icon'>      
      <h3>)rawliteral" + String(HOMEPAGE_H3) + R"rawliteral(</h3>
    </div>
  )rawliteral";

  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_Wifi(AsyncWebServerRequest *request) {

  String page = HTML_Header();

  page += R"rawliteral(
  <style>
    .wifi_box {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      padding: 1.5em;
      margin: 3em auto;
      width: 300px;
      color: white;
      text-align: center;
      animation: fadeIn 1.2s ease-out;
    }

    .wifi_input {
      width: 100%;
      padding: 0.75em 1em;
      border-radius: 0.75em;
      border: none;
      background-color: rgba(255, 255, 255, 0.1);
      color: white;
      font-size: 1em;
      box-sizing: border-box;
    }

    .wifi_button {
      width: 100%;
      background-color: rgba(255, 255, 255, 0.1);
      color: white;
      border: none;
      border-radius: 0.75em;
      font-size: 1.1em;
      padding: 0.9em;
      cursor: pointer;
      transition: background 0.3s, transform 0.2s;
    }

    .wifi_button:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    .wifi_network {
      display: flex;
      align-items: center;
      gap: 1em;
      margin: 1em 0;
      cursor: pointer;
      padding: 0.5em;
      border-radius: 0.75em;
      transition: background 0.3s, transform 0.2s;
    }
    .wifi_network:hover {
      background: rgba(255, 255, 255, 0.05);
      transform: scale(1.02);
    }
    
    .wifi_input::placeholder {
      color: rgba(255, 255, 255, 0.7);
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }
  </style>
  )rawliteral";

  if(WiFi.status() == WL_CONNECTED) 
  {
    page += "<div class='wifi_box'>";
    page += "<h2>Connected</h2>";
    page += "<p><strong>Network:</strong> " + WiFi.SSID() + "</p>";
    page += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    page += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
    page += "</div>";
  } 
  else if(AP_MODE) 
  {
    page += "<div class='wifi_box'>";
    page += "<h2>Access Point Active</h2>";
    page += "<p><strong>AP Name:</strong> " + String(AP_SSID) + "</p>";
    page += "<p><strong>AP IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
    page += "</div>";
  }

  page += R"rawliteral(
  <div class='wifi_box'>
    <h2>Connect to Wi-Fi</h2>

    <div id='ssid_list'>
      <img src='/wifi_loading' width='140' height='105'><p>Scanning...</p>
    </div>

    <form action="/wifi_save" method="GET" style="margin-top: 2em;">
      <div style="margin-bottom: 1em;">
        <input class="wifi_input" type="text" name="ssid" placeholder="Enter SSID">
      </div>
      <div style="margin-bottom: 1.5em;">
        <input class="wifi_input" type="password" name="password" placeholder="Enter Password">
      </div>
      <button type="submit" class="wifi_button">Connect</button>
    </form>
  </div>

  <script>
    let pollInterval;
    function fetchSSIDList() {
      fetch('/wifi_list')
        .then(response => response.text())
        .then(data => {
          document.getElementById('ssid_list').innerHTML = data;
          if (!data.includes('data-scanning')) {
            clearInterval(pollInterval);
          }
        });
    }
    fetch('/wifi_start_scan').then(() => {
      pollInterval = setInterval(fetchSSIDList, 2000);
    });
  </script>
  )rawliteral";

  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_Wifi_List(AsyncWebServerRequest *request) {

  String page;

  if(scan_complete)
  {
    for(int i = 0; i < wifiSSIDCount; i++) 
    {
      if(wifiSSIDs[i].duplicate) continue;

      int rssi = wifiSSIDs[i].RSSI;
      String iconStrength;

      if(rssi >= -60) iconStrength = "full";
      else if(rssi >= -75) iconStrength = "half";
      else iconStrength = "low";

      bool isOpen = (wifiSSIDs[i].encryptionType == WIFI_AUTH_OPEN);
      String lockStatus = isOpen ? "unlocked" : "locked";
      String iconFile = "/wifi_" + lockStatus + "_" + iconStrength;

      String ssidName = wifiSSIDs[i].SSID;

      String safeSSID = ssidName;
      safeSSID.replace("'", "\\'");
      page += "<div class='wifi_network' onclick=\"document.getElementsByName('ssid')[0].value='" + safeSSID + "'\">";
      page += "<img src='" + iconFile + "' width='24' height='24'>";
      page += "<span>" + ssidName + "</span>";
      page += "</div>";
    }
  }
  else
  {
    page = "<div data-scanning='true'><img src='/wifi_loading' width='140' height='105'><p>Scanning...</p></div>";
  }

  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_Wifi_Save(AsyncWebServerRequest *request) {

  _ssid = request->arg("ssid");
  _pass = request->arg("password");
    
  String page = HTML_Header();
  page += R"rawliteral(
  <style>
    .wifi_box {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      padding: 1.5em;
      margin: 3em auto;
      width: fit-content;
      color: white;
      text-align: center;
    }

    .wifi_box p {
      font-size: 1.1em;
    }

    .ssid_name {
      color: #ccc;
      font-weight: normal;
    }
  </style>

  <div class="wifi_box">
    <h2>Wi-Fi Credentials Saved</h2>
    <p>SSID: <span class="ssid_name">)rawliteral" + String(_ssid) + R"rawliteral(</span></p>
    <p>You will be redirected in <span id="countdown">10</span> seconds.</p>
  </div>

  <script>
    let seconds = 10;
    const countdownEl = document.getElementById('countdown');

    const countdownInterval = setInterval(() => {
      seconds--;
      countdownEl.textContent = seconds;

      if (seconds <= 0) {
        clearInterval(countdownInterval);
        window.location.href = '/';
      }
    }, 1000);
  </script>
  )rawliteral";
  request->send(200, "text/html", page);
  connect_to_new_network = true;
}

void AdiWiFiManager::_connectToWiFi(String ssid, String pass) {

	unsigned long started_at = millis();
	bool connecting = false; 

	if(ssid != "" && pass != "")//ssid and password provided
	{
    _DebugLog("Provided SSID: " + ssid + " and pass: " + pass);
		connecting = true;
		WiFi.begin(ssid.c_str(), pass.c_str());
	}
	else//ssid and password NOT provided
	{
		_DebugLog("Searching for saved wifi networks");

		scan_now = true;
		while(millis() - started_at < 10000) if(WIFI_Scan()) break;//start wifi scan

		if(wifiSSIDCount > 0)//if there is networks available
		{
		_preferences.begin("wifi", false);

		for(int i = 0; i < MAX_SAVED_WIFI_STATIONS; i++)//check each slot to see if any of the available networks has been saved earlier
		{
			String ssid_key = "ssid_" + String(i);
			String pass_key = "pass_" + String(i);
			String stored_ssid = _preferences.getString(ssid_key.c_str(), "");        
			String stored_pass = _preferences.getString(pass_key.c_str(), "");

      //Uncomment if you want to see all saved networks in the debug log
			//_DebugLog("Saved network " + String(i) + " - SSID: " + stored_ssid + ", PSWD: " + stored_pass);

			for(int j = 0; j < wifiSSIDCount; j++)//go through the available networks
			{
			if(wifiSSIDs[j].duplicate) continue;
			String scanned_ssid = wifiSSIDs[j].SSID;

			if(stored_ssid == scanned_ssid)
			{
				_DebugLog("Connecting to saved network " + String(i) + " - SSID: " + stored_ssid + ", PSWD: " + stored_pass);
				WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
				connecting = true;
				break;
			}
			}
			if(connecting) break;
		}
		_preferences.end();
		}
		else//no stations available to connect or scan failed
		{
		  _DebugLog("No stations to connect");
		}
	} 

	if(connecting)//check if connection is successful
	{
		wifi_status = WiFi.waitForConnectResult();

		_DebugLog("Connection result: " + String(wifi_status));

		if(wifi_status == WL_CONNECTED)
		{
		  _DebugLog("WiFi connected, IP:" + WiFi.localIP().toString());
		} 
	}

	if(wifi_status == WL_CONNECTED && ssid != "" && pass != "")//connection was successful, save this network 
	{
		_preferences.begin("wifi", false);

		int ssid_stored_at = -1;
		int emptySlot = -1;

		for(int i = 0; i < MAX_SAVED_WIFI_STATIONS; i++)//check each slot to see if the current wifi credentials has been saved
		{
		String key = "ssid_" + String(i);
		String stored_ssid = _preferences.getString(key.c_str(), "");

		if(stored_ssid == "" && emptySlot == -1) emptySlot = i;//store the index of first empty slot

    //Uncomment if you want to see all saved networks in the debug log
		//_DebugLog("Saved network " + String(i) + " - " + stored_ssid);

		if(stored_ssid == ssid)
		{
			_DebugLog("Wifi already saved at " + String(i));
			String pass_key = "pass_" + String(i);//update the password in case it has changed
			_preferences.putString(pass_key.c_str(), pass);
			ssid_stored_at = i;
			break;
		}
		}

		if(ssid_stored_at == -1)//if not saved, then add the new wifi ssid and password to eeprom
		{
		if(emptySlot != -1)//there is an empty slot
		{
			String ssid_key = "ssid_" + String(emptySlot);
			String pass_key = "pass_" + String(emptySlot);

			_preferences.putString(ssid_key.c_str(), ssid);
			_preferences.putString(pass_key.c_str(), pass);

			_DebugLog("Wifi saved to empty slot at index " + String(emptySlot));
		}
		else //if all slots are full then delete the first one and roll everything ahead
		{
			for(int i = 1; i < MAX_SAVED_WIFI_STATIONS; i++) 
      {
			  String ssid_key1 = "ssid_" + String(i - 1);
			  String pass_key1 = "pass_" + String(i - 1);

			  String ssid_key2 = "ssid_" + String(i);
			  String pass_key2 = "pass_" + String(i);

			  _preferences.putString(ssid_key1.c_str(), _preferences.getString(ssid_key2.c_str(), ""));
			  _preferences.putString(pass_key1.c_str(), _preferences.getString(pass_key2.c_str(), ""));
			}

			//add the ssid and password into the last slot

			String ssid_key = "ssid_" + String(MAX_SAVED_WIFI_STATIONS - 1);
			String pass_key = "pass_" + String(MAX_SAVED_WIFI_STATIONS - 1);

			_preferences.putString(ssid_key.c_str(), ssid);
			_preferences.putString(pass_key.c_str(), pass);

			_DebugLog("Wifi saved to last slot" + String(emptySlot));
		}
		}
		_preferences.end();
	}
}

void AdiWiFiManager::_connectToNewWiFi() {
  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  _connectToWiFi(_ssid, _pass);
  connect_to_new_network = false;
}

bool AdiWiFiManager::WIFI_Scan() {

  wifi_ssid_count_t n = WiFi.scanComplete();

  if(last_scan == 0 || (millis() - last_scan >= 60000 && scan_now && n != WIFI_SCAN_RUNNING)) 
  {
    _DebugLog("About to scan.");

    last_scan = millis();
    int scanResult = WiFi.scanNetworks(true);

    _DebugLog("Scanned Stations: " + String(scanResult));

    if(scanResult == WIFI_SCAN_FAILED) {
      _DebugLog("WIFI SCAN FAILED!");
    }
  }

  if(n >= 0)//Scan finished
  {
    scan_now = false;
    scan_complete = true;
    WIFI_CopySSIDs(n);
    WiFi.scanDelete();
    return true;
  }
  return false;
}

void AdiWiFiManager::WIFI_CopySSIDs(wifi_ssid_count_t n) {

  if(n == WIFI_SCAN_FAILED)
  {
    _DebugLog("WIFI SCAN FAILED!");
  }
  else if (n == WIFI_SCAN_RUNNING)
  {
    _DebugLog("WIFI SCAN RUNNING!");
  }
  else if (n < 0)
  {
    _DebugLog("WIFI SCAN FAILED WITH UNKNOWN ERROR CODE!");
  }
  else if (n == 0)
  {
    _DebugLog("No Networks Found");
  }
  else
  {
    _DebugLog("Scan complete. Found " + String(n) + " networks");
  }

  if(n > 0)
  {
    if(wifiSSIDs) delete[] wifiSSIDs;
    
    wifiSSIDs = new WiFiResult[n];
    wifiSSIDCount = n;

    for(wifi_ssid_count_t i = 0; i < n; i++)
    {
      wifiSSIDs[i].duplicate = false;

      WiFi.getNetworkInfo(i,
                          wifiSSIDs[i].SSID,
                          wifiSSIDs[i].encryptionType,
                          wifiSSIDs[i].RSSI,
                          wifiSSIDs[i].BSSID,
                          wifiSSIDs[i].channel);
    }

    for(int i = 0; i < n; i++)
    {
      for(int j = i + 1; j < n; j++)
      {
        if(wifiSSIDs[j].RSSI > wifiSSIDs[i].RSSI)
        {
          std::swap(wifiSSIDs[i], wifiSSIDs[j]);
        }
      }
    }

    String cssid;
    for(int i = 0; i < n; i++)
    {
      if(wifiSSIDs[i].duplicate == true) continue;

      cssid = wifiSSIDs[i].SSID;

      for(int j = i + 1; j < n; j++)
      {
        if(cssid == wifiSSIDs[j].SSID)
        {
          //_DebugLog("DUP AP: " + String(wifiSSIDs[j].SSID));
          wifiSSIDs[j].duplicate = true;
        }
      }
    }
  }
}

int AdiWiFiManager::getFileTypePriority(String filename, String ftype) {
  if(ftype == "Dir") return 0;
  if(filename.endsWith(".mp4") || filename.endsWith(".avi") || filename.endsWith(".gif") || filename.endsWith(".mjpeg")) return 1;
  if(filename.endsWith(".jpg") || filename.endsWith(".png") || filename.endsWith(".bmp")) return 2;
  if(filename.endsWith(".mp3") || filename.endsWith(".wav") || filename.endsWith(".aac")) return 3;
  if(filename.endsWith(".txt")) return 4;
  return 5;
}

String AdiWiFiManager::getRandomAssetFile(const String &folder) {

  String files[MAX_ASSET_FILES];
  int count = 0;

  File dir = ASSETS_LOCATION.open(folder);
  if(dir && dir.isDirectory()) 
  {
    File file = dir.openNextFile();
    while(file && count < MAX_ASSET_FILES) 
    {
      if(!file.isDirectory()) 
      {
        String name = file.name();
        int lastSlash = name.lastIndexOf('/');
        if(lastSlash != -1) name = name.substring(lastSlash + 1);
        String lower = name; lower.toLowerCase();
        if(lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp") || lower.endsWith(".gif") || lower.endsWith(".png"))
        {
          files[count++] = name;
        }
      }
      file.close();
      file = dir.openNextFile();
    }
    dir.close();
  }
  return (count == 0) ? "" : files[random(0, count)];
}

//SD Card Functions

#ifdef SD_ENABLED

int AdiWiFiManager::SD_countFilesInDirectory(String path) {
  int count = 0;
  File dir = SD.open(path);
  if(dir) 
  {
    File file = dir.openNextFile();
    while(file) 
    {
      count++;
      file.close();
      file = dir.openNextFile();
    }
    dir.close();
  }
  return count;
}

void AdiWiFiManager::SD_Directory(String path = "/") {
  numfiles = 0;
  File root = SD.open(path);
  if(root) 
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while(file && numfiles < MAX_FILES) 
    {
      const char* name = file.name();
      String filename = (name[0] == '/' ? String(name + 1) : String(name));
      
      int lastSlash = filename.lastIndexOf('/');
      if(lastSlash != -1) filename = filename.substring(lastSlash + 1);
      
      Filenames[numfiles].filename = filename;
      Filenames[numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      
      if(file.isDirectory()) {
        String fullPath = path;
        if(!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        int fileCount = SD_countFilesInDirectory(fullPath);
        Filenames[numfiles].fsize = String(fileCount) + " items";
      } else {
        Filenames[numfiles].fsize = ConvBinUnits(file.size(), 1);
      }
      
      file.close();
      file = root.openNextFile();
      numfiles++;
    }
    root.close();
  }
  
  for(int i = 0; i < numfiles - 1; i++) {
    for(int j = i + 1; j < numfiles; j++) {
      int priority_i = getFileTypePriority(Filenames[i].filename, Filenames[i].ftype);
      int priority_j = getFileTypePriority(Filenames[j].filename, Filenames[j].ftype);
      
      if(priority_i > priority_j) {
        fileinfo temp = Filenames[i];
        Filenames[i] = Filenames[j];
        Filenames[j] = temp;
      }
    }
  }
}

void AdiWiFiManager::SD_createDirectoryRecursive(const String& path) {
  String currentPath = "";
  int start = 1;
  int end = path.indexOf('/', start);

  while(end != -1) 
  {
    currentPath += "/" + path.substring(start, end);
    if(!SD.exists(currentPath)) 
    {
      if(!SD.mkdir(currentPath)) _DebugLog("Failed to create directory: " + currentPath);
    }
    start = end + 1;
    end = path.indexOf('/', start);
  }

  if(start < path.length()) 
  {
    currentPath += "/" + path.substring(start);
    if(!SD.exists(currentPath)) SD.mkdir(currentPath);
  }
}

void AdiWiFiManager::SD_deleteRecursive(String path) {
  File file = SD.open(path);
  if(!file) return;
  
  if(file.isDirectory()) 
  {
    file.rewindDirectory();
    File entry = file.openNextFile();
    while(entry) 
    {
      String entryPath = path;
      if(!entryPath.endsWith("/")) entryPath += "/";
      
      const char* name = entry.name();
      String entryName = (name[0] == '/' ? String(name + 1) : String(name));
      int lastSlash = entryName.lastIndexOf('/');
      if(lastSlash != -1) entryName = entryName.substring(lastSlash + 1);
      
      entryPath += entryName;
      
      if(entry.isDirectory()) 
      {
        entry.close();
        SD_deleteRecursive(entryPath);
      } 
      else 
      {
        entry.close();
        SD.remove(entryPath);
      }
      entry = file.openNextFile();
    }
    file.close();
    SD.rmdir(path);
  } 
  else 
  {
    file.close();
    SD.remove(path);
  }
}

void AdiWiFiManager::Handle_SD_Dir(AsyncWebServerRequest *request) {
  String currentPath = "/";
  if(request->hasParam("path")) 
  {
    currentPath = request->getParam("path")->value();
    if(!currentPath.startsWith("/")) currentPath = "/" + currentPath;
  }
  
  String Fname1, Fname2;
  String icon1, icon2;
  String Fsize1, Fsize2;
  int index = 0;
  SD_Directory(currentPath);

  String page = HTML_Header();
  page += R"rawliteral(
  <style>
    .file_box {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      padding: 2em;
      margin: 3em auto;
      width: fit-content;
      color: white;
      font-family: "Segoe UI", sans-serif;
      font-size: 1.05em;
      animation: fadeIn 1.2s ease-out;
    }

    .path_display {
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      border-radius: 0.75em;
      margin-bottom: 1.5em;
      font-family: monospace;
      text-align: center;
    }

    .file_controls {
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 1em;
      margin-bottom: 2em;
      animation: fadeIn 1.2s ease-out;
    }

    .file_controls a {
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      color: white;
      border-radius: 0.75em;
      text-decoration: none;
      font-weight: bold;
      transition: background 0.2s ease;
    }

    .file_controls a:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    table {
      width: 100%;
      border-collapse: collapse;
      background-color: rgba(255, 255, 255, 0.05);
      border-radius: 0.75em;
      overflow: hidden;
      table-layout: fixed;
    }

    th, td {
      padding: 0.9em;
      text-align: left;
      border-bottom: 1px solid rgba(255,255,255,0.2);
      word-wrap: break-word;
      vertical-align: top;
    }

    .file_group {
      display: flex;
      flex-direction: column;
      gap: 0.6em;
      padding: 1.2em;
      transition: background 0.2s ease;
      border-radius: 0.75em;
    }

    .file_group:hover {
      background-color: rgba(255, 255, 255, 0.07);
      cursor: pointer;
    }

    .file_name {
      display: flex;
      align-items: center;
      gap: 0.8em;
      font-weight: 500;
      font-size: 1.1em;
    }

    .file_name img {
      width: 32px;
      height: 32px;
      object-fit: contain;
    }

    td.divider {
      width: 2px;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .popup_box .spinner {
      display: block;
      width: 32px; height: 32px;
      border: 4px solid rgba(255,255,255,0.2);
      border-top-color: #00c6ff;
      border-radius: 50%;
      margin: 0 auto 0.8em auto;
      animation: spin 0.8s linear infinite;
    }

    @keyframes spin { to { transform: rotate(360deg); } }
    .popup_box.processing .popup_buttons { display: none; }
  </style>

  <div class='file_box'>
    <h2>📁 SD File Manager</h2>
    <div class='path_display'>)rawliteral";
  
  page += currentPath;
  page += R"rawliteral(</div>
    <div class='file_controls'>)rawliteral";
  
  if(currentPath != "/") {
    page += "<a href=\"#\" onclick=\"goBack(); return false;\">Go Back</a>";
  }

  page += "<a href='/sdupload?path=" + currentPath + "'>Upload</a>";
 
  page += R"rawliteral(
      <a href="#" onclick="showCreateFolder(); return false;">New Folder</a>
      <a href="#" id="downloadBtn" onclick="handleActionButton('download'); return false;">Download</a>
      <a href="#" id="renameBtn" onclick="handleActionButton('rename'); return false;">Rename</a>
      <a href="#" id="moveBtn" onclick="handleActionButton('move'); return false;">Move</a>
      <a href="#" id="deleteBtn" onclick="handleActionButton('delete'); return false;">Delete</a>
    </div>
  )rawliteral";

  if(numfiles > 0)
  {
    page += "<table>";

    while(index < numfiles) 
    {
      Fname1 = Filenames[index].filename;
      Fsize1 = Filenames[index].fsize;

      if(Filenames[index].ftype == "Dir")
      {
        icon1 = "/folder_icon";
      }
      else
      {
        if (Fname1.endsWith(".jpg") || Fname1.endsWith(".png") || Fname1.endsWith(".bmp")) icon1 = "/img_icon";
        else if (Fname1.endsWith(".mp4") || Fname1.endsWith(".avi") || Fname1.endsWith(".gif") || Fname1.endsWith(".mjpeg")) icon1 = "/video_icon";
        else if (Fname1.endsWith(".mp3") || Fname1.endsWith(".wav") || Fname1.endsWith(".aac")) icon1 = "/audio_icon";
        else if (Fname1.endsWith(".txt")) icon1 = "/txt_icon";
        else icon1 = "/file_icon";
      }

      if(index + 1 < numfiles)
      {
        Fname2 = Filenames[index + 1].filename;
        Fsize2 = Filenames[index + 1].fsize;

        if(Filenames[index + 1].ftype == "Dir")
        {
          icon2 = "/folder_icon";
        }
        else
        {
        if (Fname2.endsWith(".jpg") || Fname2.endsWith(".png") || Fname2.endsWith(".bmp")) icon2 = "/img_icon";
        else if (Fname2.endsWith(".mp4") || Fname2.endsWith(".avi") || Fname2.endsWith(".gif") || Fname2.endsWith(".mjpeg")) icon2 = "/video_icon";
        else if (Fname2.endsWith(".mp3") || Fname2.endsWith(".wav") || Fname2.endsWith(".aac")) icon2 = "/audio_icon";
        else if (Fname2.endsWith(".txt")) icon2 = "/txt_icon";
        else icon2 = "/file_icon";
        }
      }
      else
      {
        Fname2 = "";
        Fsize2 = "";
        icon2 = "";
      }

      page += "<tr>";
      page += "<td colspan='3'>";
      page += "<div class='file_group' data-filename='" + Fname1 +"' data-type='" + Filenames[index].ftype + "'>";
      page += "<div class='file_name'>";
      page += "<img src='" + icon1 + "'>";
      page += Fname1;
      page += "</div>";
      if(Filenames[index].ftype == "Dir") {
        page += "<div><strong>Items: </strong>" + Fsize1 + "</div>";
      } else {
        page += "<div><strong>Size: </strong>" + Fsize1 + "</div>";
      }
      page += "</div>";
      page += "</td>";
      page += "<td class='divider'></td>";

      if(index + 1 < numfiles) 
      {
        page += "<td colspan='3'>";
        page += "<div class='file_group' data-filename='" + Fname2 +"' data-type='" + Filenames[index + 1].ftype + "'>";
        page += "<div class='file_name'>";
        page += "<img src='" + icon2 + "'>";
        page += Fname2;
        page += "</div>";
        if(Filenames[index + 1].ftype == "Dir") {
          page += "<div><strong>Items: </strong>" + Fsize2 + "</div>";
        } else {
          page += "<div><strong>Size: </strong>" + Fsize2 + "</div>";
        }
        page += "</div>";
        page += "</td>";
        page += "</tr>";
      } 
      else
      {
        page += "<td colspan='3'></td>";
      }
      page += "</tr>";
      index += 2;
    }
    page += R"rawliteral(
      </table>
      </div>

      <div id="filePopup" class="popup_overlay">
        <div class="popup_box">
          <div id="filePopup_text"></div>
          <div class="popup_buttons">
            <button class="confirm_btn" onclick="startFileAction()">Yes</button>
            <button class="cancel_btn" onclick="closeFilePopup()">Cancel</button>
          </div>
        </div>
      </div>

      <script>
        let mode = '';
        let selectedFiles = [];
        const currentPath = ')rawliteral";
        page += currentPath;
        page += R"rawliteral(';

        function goBack() {
          const parts = currentPath.split('/').filter(p => p);
          parts.pop();
          const newPath = '/' + parts.join('/');
          window.location.href = '/sd_dir?path=' + encodeURIComponent(newPath);
        }

        function resetSelection() {
          selectedFiles = [];
          document.querySelectorAll('.file_group').forEach(el => el.style.backgroundColor = '');
        }

        function exitMode() {
          mode = '';
          resetSelection();
          document.querySelector('.file_box h2').textContent = '📁 SD File Manager';
          document.getElementById('deleteBtn').textContent = 'Delete';
          document.getElementById('moveBtn').textContent = 'Move';
          document.getElementById('downloadBtn').textContent = 'Download';
          document.getElementById('renameBtn').textContent = 'Rename';
        }

        function handleActionButton(action) {
          if (mode !== action) {
            mode = action;
            resetSelection();
            const labels = {
              delete: 'Select files/folders to delete',
              move: 'Select files/folders to move',
              download: 'Click on a file to download',
              rename: 'Click on a file/folder to rename'
            };
            document.querySelector('.file_box h2').textContent = labels[action];
            document.getElementById(action + 'Btn').textContent = 'Cancel';
            return;
          }

          if (action === 'delete' || action === 'move') {
            if (selectedFiles.length === 0) { exitMode(); return; }
            const popup = document.getElementById('filePopup');
            const popupText = document.getElementById('filePopup_text');
            if (action === 'delete') {
              popupText.innerHTML = 'Delete ' + selectedFiles.length + ' item(s)?';
            } else {
              popupText.innerHTML = 'Move ' + selectedFiles.length + ' item(s) to:<br><input id="dest_path" type="text" placeholder="/destination/folder" value="' + currentPath + '" style="margin-top: 1em; width: 100%; padding: 0.5em;">';
            }
            popup.style.display = 'flex';
          } else {
            exitMode();
          }
        }

        function fileClickHandler(e) {
          const filename = e.currentTarget.getAttribute('data-filename');
          const type = e.currentTarget.getAttribute('data-type');
          const popup = document.getElementById('filePopup');
          const popupText = document.getElementById('filePopup_text');
          const fullPath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;

          if (mode === 'delete' || mode === 'move') {
            e.stopPropagation();
            const idx = selectedFiles.indexOf(fullPath);
            if (idx > -1) {
              selectedFiles.splice(idx, 1);
              e.currentTarget.style.backgroundColor = '';
            } else {
              selectedFiles.push(fullPath);
              e.currentTarget.style.backgroundColor = mode === 'delete' ? 'rgba(255,0,0,0.3)' : 'rgba(0,120,255,0.3)';
            }
            const label = mode === 'delete' ? 'Delete' : 'Move';
            document.getElementById(mode + 'Btn').textContent = selectedFiles.length > 0 ? `Confirm ${label} (${selectedFiles.length})` : 'Cancel';
            document.querySelector('.file_box h2').textContent = `Select files/folders to ${mode} (${selectedFiles.length} selected)`;
            return;
          }

          if (mode === 'download') {
            popupText.innerHTML = 'Download "<b>' + filename + '</b>"?';
            selectedFiles = [fullPath];
            popup.style.display = 'flex';
            return;
          }

          if (mode === 'rename') {
            popupText.innerHTML = 'Rename "<b>' + filename + '</b>": <br><input id="new_name" type="text" placeholder="New name" style="margin-top: 1em; width: 100%;">';
            selectedFiles = [fullPath];
            popup.style.display = 'flex';
            return;
          }

          if (type === 'Dir') {
            window.location.href = '/sd_dir?path=' + encodeURIComponent(fullPath);
          } else {
            const ext = filename.split('.').pop().toLowerCase();
            const images = ['jpg', 'jpeg', 'png', 'bmp', 'gif'];
            const videos = ['mp4', 'avi', 'webm', 'mjpeg'];
            const audio  = ['mp3', 'wav', 'aac'];
            const text   = ['txt'];
            if (images.includes(ext) || videos.includes(ext) || audio.includes(ext) || text.includes(ext)) {
              window.open(fullPath, '_blank');
            } else {
              alert("No preview available for this file type.");
            }
          }
        }

        document.querySelectorAll('.file_group').forEach(el => el.addEventListener('click', fileClickHandler));

        async function startFileAction() {
          if (mode === 'createfolder') {
            const folderName = document.getElementById('folder_name').value.trim();
            if (!folderName) { alert("Enter folder name"); return; }
            window.location.href = '/sdcreatefolder?path=' + encodeURIComponent(currentPath) +
                                  '&name=' + encodeURIComponent(folderName);
            closeFilePopup();
            return;
          }

          if (selectedFiles.length === 0) return;

          const popup = document.getElementById('filePopup');
          const popupBox = popup.querySelector('.popup_box');
          const popupText = document.getElementById('filePopup_text');

          if (mode === 'download') {
            const filePath = selectedFiles[0];
            const filenameOnly = filePath.split('/').pop();
            closeFilePopup();
            fetch('/sddownload?filename=' + encodeURIComponent(filePath))
              .then(response => {
                if (!response.ok) throw new Error('Server returned ' + response.status);
                return response.blob();
              })
              .then(blob => {
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = filenameOnly;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
                exitMode();
              })
              .catch(err => alert('Download failed: ' + err.message));
          }

          else if (mode === 'delete') {
            popupBox.classList.add('processing');
            popupText.innerHTML = '<div class="spinner"></div><div>Deleting ' + selectedFiles.length + ' item(s)...</div>';

            let deleteCount = 0;
            for (let i = 0; i < selectedFiles.length; i++) {
              try {
                const response = await fetch('/sddelete?filename=' + encodeURIComponent(selectedFiles[i]) + '&path=' + encodeURIComponent(currentPath));
                if (response.ok) deleteCount++;
              } catch (err) { console.error('Delete failed:', err); }
            }
            window.location.href = '/sd_dir?path=' + encodeURIComponent(currentPath);
          }

          else if (mode === 'move') {
            const destPath = document.getElementById('dest_path').value;
            if (!destPath || destPath.trim() === "") { alert("Enter destination path"); return; }

            popupBox.classList.add('processing');
            popupText.innerHTML = '<div class="spinner"></div><div>Moving ' + selectedFiles.length + ' item(s)...</div>';
            let moveCount = 0;
            for (let i = 0; i < selectedFiles.length; i++) {
              try {
                const response = await fetch('/sdmove?source=' + encodeURIComponent(selectedFiles[i]) +
                                              '&destination=' + encodeURIComponent(destPath) +
                                              '&path=' + encodeURIComponent(currentPath));
                if (response.ok) moveCount++;
              } catch (err) { console.error('Move failed:', err); }
            }
            window.location.href = '/sd_dir?path=' + encodeURIComponent(currentPath);
          }

          else if (mode === 'rename') {
            const newName = document.getElementById('new_name').value;
            if (newName && newName.trim() !== "") {
              window.location.href = "/sdrename?old=" + encodeURIComponent(selectedFiles[0]) + "&new=" + encodeURIComponent(newName) + "&path=" + encodeURIComponent(currentPath);
            } else {
              alert("Enter a new filename");
            }
          }
        }

        function closeFilePopup() {
          const popup = document.getElementById('filePopup');
          popup.style.display = 'none';
          popup.querySelector('.popup_box').classList.remove('processing');

          if (mode === 'delete' || mode === 'move') {
            exitMode();
          } else {
            selectedFiles = [];
            document.querySelectorAll('.file_group').forEach(el => el.style.backgroundColor = '');
          }
        }

        function showCreateFolder() {
          const popup = document.getElementById('filePopup');
          const popupText = document.getElementById('filePopup_text');
          
          popupText.innerHTML = 'Create new folder in current directory:<br><input id="folder_name" type="text" placeholder="Folder name" style="margin-top: 1em; width: 100%; padding: 0.5em;">';
          popup.style.display = 'flex';
          
          mode = 'createfolder';
        }
      </script>

      <style>
        .popup_overlay {
          position: fixed;
          top: 0; left: 0;
          width: 100%; height: 100%;
          background-color: rgba(0, 0, 0, 0.7);
          display: none;
          align-items: center;
          justify-content: center;
          z-index: 9999;
        }

        .popup_box {
          background: rgba(27, 27, 27, 1);
          padding: 2em;
          border-radius: 1em;
          color: white;
          text-align: center;
          width: 90%;
          max-width: 400px;
          box-shadow: 0 4px 16px rgba(0,0,0,0.5);
        }

        .popup_buttons {
          margin-top: 1.5em;
          display: flex;
          justify-content: space-around;
        }

        .popup_buttons button {
          padding: 0.6em 1.5em;
          border: none;
          border-radius: 0.5em;
          cursor: pointer;
          font-weight: bold;
          color: white;
          transition: background 0.2s ease;
        }

        .confirm_btn {
          background-color: rgba(91, 91, 91, 1);
        }

        .confirm_btn:hover {
          background-color: #00e676;
        }

        .cancel_btn {
          background-color: rgba(91, 91, 91, 1);
        }

        .cancel_btn:hover {
          background-color: #ef5350;
        }
      </style>
    )rawliteral";
  }
  else
  {
    page += "<h3>No Files Found</h3>";
    page += "</div>";
  }
  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_SD_File_Upload(AsyncWebServerRequest *request) {
  if(request->method() == HTTP_GET) 
  {
    String uploadPath = "/";
    if(request->hasParam("path")) 
    {
      uploadPath = request->getParam("path")->value();
      if(!uploadPath.startsWith("/")) uploadPath = "/" + uploadPath;
    }

    String page = HTML_Header();
    page += R"rawliteral(
    <style>
      .file_box {
        background: rgba(38, 38, 38, 0.5);
        backdrop-filter: blur(5px);
        box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
        border-radius: 1em;
        padding: 2em;
        margin: 3em auto;
        width: fit-content;
        color: white;
        font-family: "Segoe UI", sans-serif;
        font-size: 1.05em;
        animation: fadeIn 1.2s ease-out;
      }

      .upload_form {
        display: flex;
        flex-direction: column;
        gap: 1.5em;
        align-items: center;
      }

      .custom_file_input {
        position: relative;
        display: inline-block;
        overflow: hidden;
        border-radius: 0.75em;
        background: rgba(255, 255, 255, 0.1);
        cursor: pointer;
        font-weight: bold;
        padding: 0.75em 1.2em;
        color: white;
        transition: background 0.2s ease;
        width: 100%;
        text-align: center;
      }

      .custom_file_input:hover {
        background-color: rgba(255, 255, 255, 0.3);
        transform: scale(1.02);
      }

      .custom_file_input input[type="file"] {
        position: absolute;
        left: 0;
        top: 0;
        opacity: 0;
        cursor: pointer;
        width: 100%;
        height: 100%;
      }

      .filename_note {
        font-size: 0.9em;
        font-style: italic;
        color: #ccc;
      }

      .upload_button, .toggle_button {
        width: 100%;
        background: rgba(255, 255, 255, 0.1);
        padding: 0.75em 1.5em;
        color: white;
        border: none;
        border-radius: 0.75em;
        font-weight: bold;
        cursor: pointer;
        position: relative;
        overflow: hidden;
        transition: background 0.2s ease;
      }

      .upload_button:hover, .toggle_button:hover {
        background-color: rgba(255, 255, 255, 0.3);
        transform: scale(1.02);
      }

      .upload_button .progress_fill {
        background: linear-gradient(90deg, #00c6ff, #0072ff);
        position: absolute;
        left: 0;
        top: 0;
        height: 100%;
        width: 0%;
        z-index: 0;
        transition: width 0.2s ease;
      }

      .upload_button span {
        position: relative;
        z-index: 1;
      }

      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(20px); }
        to { opacity: 1; transform: translateY(0); }
      }
    </style>

    <div class="file_box">
      <h2>Upload Files/Folders</h2>

      <div class="upload_form">
        <label class="custom_file_input">
          <span id="inputLabel">Choose Files</span>
          <input id="fileInput" type="file" multiple>
        </label>

        <button type="button" class="toggle_button" onclick="toggleUploadMode()">
          <span id="modeText">Switch to Folder Mode</span>
        </button>

        <div id="fileNameNote" class="filename_note">No files selected</div>

        <button type="button" class="upload_button" id="uploadBtn">
          <div class="progress_fill" id="progressFill"></div>
          <span id="uploadText">Upload</span>
        </button>
      </div>
    </div>
    <script>
      const uploadPath = ')rawliteral" + uploadPath + R"rawliteral(';
      const fileInput = document.getElementById('fileInput');
      const fileNameNote = document.getElementById('fileNameNote');
      const uploadBtn = document.getElementById('uploadBtn');
      const uploadText = document.getElementById('uploadText');
      const progressFill = document.getElementById('progressFill');
      const inputLabel = document.getElementById('inputLabel');
      const modeText = document.getElementById('modeText');
      
      let folderMode = false;

      function toggleUploadMode() {
        folderMode = !folderMode;
        fileInput.value = '';
        
        if (folderMode) {
          fileInput.setAttribute('webkitdirectory', '');
          fileInput.setAttribute('directory', '');
          fileInput.removeAttribute('multiple');
          inputLabel.textContent = 'Choose Folder';
          modeText.textContent = 'Switch to File Mode';
          fileNameNote.textContent = 'No folder selected';
        } else {
          fileInput.removeAttribute('webkitdirectory');
          fileInput.removeAttribute('directory');
          fileInput.setAttribute('multiple', '');
          inputLabel.textContent = 'Choose Files';
          modeText.textContent = 'Switch to Folder Mode';
          fileNameNote.textContent = 'No files selected';
        }
      }

      fileInput.addEventListener('change', () => {
        const files = fileInput.files;
        if (files.length === 0) {
          fileNameNote.textContent = folderMode ? 'No folder selected' : 'No files selected';
        } else if (files.length === 1) {
          fileNameNote.textContent = `Selected: ${files[0].name}`;
        } else {
          fileNameNote.textContent = `Selected: ${files.length} files`;
        }
      });

      function getTotalSize(files) {
        let total = 0;
        for (let i = 0; i < files.length; i++) {
          total += files[i].size;
        }
        return total;
      }

      uploadBtn.addEventListener('click', async () => {
        const files = fileInput.files;
        if (files.length === 0) return alert('Please select files');

        uploadBtn.disabled = true;
        let totalUploaded = 0;

        for (let i = 0; i < files.length; i++) {
          const file = files[i];
          const formData = new FormData();
          formData.append('filename', file);
          const relPath = file.webkitRelativePath || file.name;
          const base = uploadPath === '/' ? '' : uploadPath;
          formData.append('filepath', base + '/' + relPath);

          try {
            await new Promise((resolve, reject) => {
              const xhr = new XMLHttpRequest();
              
              xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                  const filePercent = (e.loaded / e.total) * 100;
                  const overallPercent = ((totalUploaded + e.loaded) / getTotalSize(files)) * 100;
                  progressFill.style.width = overallPercent + '%';
                  uploadText.textContent = `${Math.round(overallPercent)}%`;
                }
              };

              xhr.onload = () => {
                if (xhr.status === 200) {
                  totalUploaded += file.size;
                  resolve();
                } else {
                  reject();
                }
              };

              xhr.onerror = () => reject();
              
              xhr.open('POST', '/sdupload', true);
              xhr.send(formData);
            });
          } catch (err) {
            console.error('Upload failed:', err);
          }
        }
        uploadText.textContent = 'Done!';
        setTimeout(() => {
          window.location.href = '/sd_dir?path=' + encodeURIComponent(uploadPath);
        }, 800);
      });
    </script>
    )rawliteral";
    request->send(200, "text/html", page);
    return;
  }
}

void AdiWiFiManager::on_SD_File_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {

  static int start = 0;
  static int uploadtime = 0;
  static int uploadsize = 0;
  static bool uploadAborted = false;
  static String filepath = "";

  if(!index) 
  {
    uploadAborted = false;
    filepath = "/";

    if(request->hasArg("filepath"))
    {
      filepath += request->arg("filepath");
    }
    else
    {
      filepath += filename;
    }

    size_t freeSpace = SD.totalBytes() - SD.usedBytes();
    if(request->contentLength() > 0 && request->contentLength() > freeSpace) 
    {
      _DebugLog("Upload rejected: not enough space for " + filepath + " (" + String(request->contentLength()) + " needed, " + String(freeSpace) + " free)");
      uploadAborted = true;
      request->send(507, "text/plain", "Not enough storage space");
      return;
    }

    int lastSlash = filepath.lastIndexOf('/');
    if(lastSlash > 0)
    {
      String dirPath = filepath.substring(0, lastSlash);
      SD_createDirectoryRecursive(dirPath);
    }

    if(request->_tempFile) request->_tempFile.close();

    request->_tempFile = SD.open(filepath, FILE_WRITE);
    if(!request->_tempFile)
    {
      _DebugLog("Failed to create file: " + filepath);
      return;
    }
    _DebugLog("Started upload: " + filepath);
    start = millis();
  }

  if(uploadAborted) return;

  if(request->_tempFile && len) 
  {
    size_t freeSpace = SD.totalBytes() - SD.usedBytes();
    if(len > freeSpace) 
    {
      _DebugLog("Upload aborted mid-transfer: ran out of space");
      request->_tempFile.close();
      SD.remove(filepath);
      uploadAborted = true;
      request->send(507, "text/plain", "Ran out of storage space");
      return;
    }

    size_t written = request->_tempFile.write(data, len);
    if(written != len) {
      _DebugLog("Write error: expected " + String(len) + ", wrote " + String(written));
    }
  }

  if(final && request->_tempFile) 
  {
    uploadsize = request->_tempFile.size();
    request->_tempFile.flush();
    request->_tempFile.close();
    uploadtime = millis() - start;
    float speed = (uploadsize / 1024.0) / (uploadtime / 1000.0);
    _DebugLog("Upload finished: " + String(uploadsize) + " bytes in " + String(uploadtime) + "ms (" + String(speed, 2) + " KB/s)");
    request->send(200);
  }
}

void AdiWiFiManager::Handle_SD_File_Download(AsyncWebServerRequest *request) {
  if(!request->hasParam("filename")) 
  {
    request->send(400, "text/plain", "Missing filename");
    _DebugLog("Download Handler failed, missing filename");
    return;
  }
  String filename = request->getParam("filename")->value();
  if(!SD.exists(filename)) 
  {
    request->send(404, "text/plain", "File not found");
    _DebugLog("Download Handler failed, file not found");
    return;
  }
  
  File file = SD.open(filename);
  if(file.isDirectory()) {
    file.close();
    request->send(400, "text/plain", "Cannot download folders directly");
    return;
  }
  file.close();
  
  String contentType = "application/octet-stream";
  if (filename.endsWith(".png")) contentType = "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) contentType = "image/jpeg";
  else if (filename.endsWith(".txt")) contentType = "text/plain";
  else if (filename.endsWith(".mp4")) contentType = "video/mp4";
  else if (filename.endsWith(".wav")) contentType = "audio/wav";
  request->send(SD, filename, contentType, true);
}

void AdiWiFiManager::Handle_SD_File_Delete(AsyncWebServerRequest *request) {
  if(!request->hasParam("filename")) 
  {
    request->send(400, "text/html", "Missing 'filename' parameter");
    return;
  }
  String filename = request->getParam("filename")->value();
  if(!filename.startsWith("/")) filename = "/" + filename;
  
  String currentPath = "/";
  if(request->hasParam("path")) 
  {
    currentPath = request->getParam("path")->value();
  } 
  else 
  {
    int lastSlash = filename.lastIndexOf('/');
    if(lastSlash > 0) currentPath = filename.substring(0, lastSlash);
  }
  
  if(SD.exists(filename)) 
  {
    SD_deleteRecursive(filename);
    _DebugLog("Deleted: " + filename);
    request->send(200, "text/plain", "OK");
  } 
  else 
  {
    _DebugLog("Failed to delete, file not found");
    request->send(404, "text/plain", "Not found");
  }
}

void AdiWiFiManager::Handle_SD_File_Rename(AsyncWebServerRequest *request) {
  if(!request->hasParam("old") || !request->hasParam("new")) 
  {
    request->send(400, "text/html", "Missing 'old' or 'new' filename parameter");
    return;
  }
  String oldName = request->getParam("old")->value();
  String newName = request->getParam("new")->value();
  if(!oldName.startsWith("/")) oldName = "/" + oldName;
  
  String currentPath = "/";
  if(request->hasParam("path")) currentPath = request->getParam("path")->value();
  else 
  {
    int lastSlash = oldName.lastIndexOf('/');
    if(lastSlash > 0) currentPath = oldName.substring(0, lastSlash);
  }
  
  String newFullPath = currentPath;
  if(!newFullPath.endsWith("/")) newFullPath += "/";
  newFullPath += newName;
  
  if(oldName != newFullPath && oldName != "/" && newFullPath != "/") 
  {
    if(SD.exists(oldName)) 
    {
      if(!SD.exists(newFullPath)) 
      {
        if(SD.rename(oldName, newFullPath)) 
        {
          _DebugLog("Renamed from " + oldName + " to " + newFullPath);
        } 
        else 
        {
          _DebugLog("Failed to rename");
        }
      }
      else
      {
        _DebugLog("A file with the new name already exists");
      }
    }
    else
    {
      _DebugLog("Original file does not exist");
    }
  }
  request->redirect("/sd_dir?path=" + currentPath);
}

void AdiWiFiManager::Handle_SD_File_Move(AsyncWebServerRequest *request) {
  if(!request->hasParam("source") || !request->hasParam("destination")) 
  {
    request->send(400, "text/html", "Missing parameters");
    return;
  }
  
  String sourcePath = request->getParam("source")->value();
  String destFolder = request->getParam("destination")->value();
  
  if(!sourcePath.startsWith("/")) sourcePath = "/" + sourcePath;
  if(!destFolder.startsWith("/")) destFolder = "/" + destFolder;
  if(!destFolder.endsWith("/")) destFolder += "/";
  
  String currentPath = "/";
  if(request->hasParam("path")) currentPath = request->getParam("path")->value();
  
  // Extract filename from source
  int lastSlash = sourcePath.lastIndexOf('/');
  String filename = sourcePath.substring(lastSlash + 1);
  
  String destPath = destFolder + filename;
  
  if(sourcePath != destPath && SD.exists(sourcePath)) 
  {
    if(!SD.exists(destPath)) 
    {
      if(SD.rename(sourcePath, destPath)) 
      {
        _DebugLog("Moved from " + sourcePath + " to " + destPath);
      } 
      else 
      {
        _DebugLog("Failed to move");
      }
    }
    else
    {
      _DebugLog("File already exists at destination");
    }
  }
  
  request->redirect("/sd_dir?path=" + currentPath);
}

void AdiWiFiManager::Handle_SD_Create_Folder(AsyncWebServerRequest *request) {
  if(!request->hasParam("path") || !request->hasParam("name")) 
  {
    request->send(400, "text/html", "Missing parameters");
    return;
  }
  
  String basePath = request->getParam("path")->value();
  String folderName = request->getParam("name")->value();
  
  if(!basePath.startsWith("/")) basePath = "/" + basePath;
  if(!basePath.endsWith("/")) basePath += "/";
  
  String fullPath = basePath + folderName;
  
  if(!SD.exists(fullPath)) 
  {
    if(SD.mkdir(fullPath)) 
    {
      _DebugLog("Created folder: " + fullPath);
    } 
    else 
    {
      _DebugLog("Failed to create folder");
    }
  }
  else
  {
    _DebugLog("Folder already exists");
  }
  
  request->redirect("/sd_dir?path=" + fullPath);
}

#endif

//LittleFSf Functions

#ifdef LittleFS_ENABLED

int AdiWiFiManager::LittleFS_countFilesInDirectory(String path) {
  int count = 0;
  File dir = LittleFS.open(path);
  if(dir) 
  {
    File file = dir.openNextFile();
    while(file) 
    {
      count++;
      file.close();
      file = dir.openNextFile();
    }
    dir.close();
  }
  return count;
}

void AdiWiFiManager::LittleFS_Directory(String path = "/") {
  numfiles = 0;
  File root = LittleFS.open(path);
  if(root) 
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while(file && numfiles < MAX_FILES) 
    {
      const char* name = file.name();
      String filename = (name[0] == '/' ? String(name + 1) : String(name));
      
      int lastSlash = filename.lastIndexOf('/');
      if(lastSlash != -1) filename = filename.substring(lastSlash + 1);
      
      Filenames[numfiles].filename = filename;
      Filenames[numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      
      if(file.isDirectory()) {
        String fullPath = path;
        if(!fullPath.endsWith("/")) fullPath += "/";
        fullPath += filename;
        int fileCount = LittleFS_countFilesInDirectory(fullPath);
        Filenames[numfiles].fsize = String(fileCount) + " items";
      } else {
        Filenames[numfiles].fsize = ConvBinUnits(file.size(), 1);
      }
      
      file.close();
      file = root.openNextFile();
      numfiles++;
    }
    root.close();
  }
  
  for(int i = 0; i < numfiles - 1; i++) {
    for(int j = i + 1; j < numfiles; j++) {
      int priority_i = getFileTypePriority(Filenames[i].filename, Filenames[i].ftype);
      int priority_j = getFileTypePriority(Filenames[j].filename, Filenames[j].ftype);
      
      if(priority_i > priority_j) {
        fileinfo temp = Filenames[i];
        Filenames[i] = Filenames[j];
        Filenames[j] = temp;
      }
    }
  }
}

void AdiWiFiManager::LittleFS_createDirectoryRecursive(const String& path) {
  String currentPath = "";
  int start = 1;
  int end = path.indexOf('/', start);

  while(end != -1) 
  {
    currentPath += "/" + path.substring(start, end);
    if(!LittleFS.exists(currentPath)) 
    {
      if(!LittleFS.mkdir(currentPath)) _DebugLog("Failed to create directory: " + currentPath);
    }
    start = end + 1;
    end = path.indexOf('/', start);
  }

  if(start < path.length()) 
  {
    currentPath += "/" + path.substring(start);
    if(!LittleFS.exists(currentPath)) LittleFS.mkdir(currentPath);
  }
}

void AdiWiFiManager::LittleFS_deleteRecursive(String path) {
  File file = LittleFS.open(path);
  if(!file) return;
  
  if(file.isDirectory()) 
  {
    file.rewindDirectory();
    File entry = file.openNextFile();
    while(entry) 
    {
      String entryPath = path;
      if(!entryPath.endsWith("/")) entryPath += "/";
      
      const char* name = entry.name();
      String entryName = (name[0] == '/' ? String(name + 1) : String(name));
      int lastSlash = entryName.lastIndexOf('/');
      if(lastSlash != -1) entryName = entryName.substring(lastSlash + 1);
      
      entryPath += entryName;
      
      if(entry.isDirectory()) 
      {
        entry.close();
        LittleFS_deleteRecursive(entryPath);
      } 
      else 
      {
        entry.close();
        LittleFS.remove(entryPath);
      }
      entry = file.openNextFile();
    }
    file.close();
    LittleFS.rmdir(path);
  } 
  else 
  {
    file.close();
    LittleFS.remove(path);
  }
}

void AdiWiFiManager::Handle_LittleFS_Dir(AsyncWebServerRequest *request) {
  String currentPath = "/";
  if(request->hasParam("path")) 
  {
    currentPath = request->getParam("path")->value();
    if(!currentPath.startsWith("/")) currentPath = "/" + currentPath;
  }
  
  String Fname1, Fname2;
  String icon1, icon2;
  String Fsize1, Fsize2;
  int index = 0;
  LittleFS_Directory(currentPath);

  String page = HTML_Header();
  page += R"rawliteral(
  <style>
    .file_box {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      padding: 2em;
      margin: 3em auto;
      width: fit-content;
      color: white;
      font-family: "Segoe UI", sans-serif;
      font-size: 1.05em;
      animation: fadeIn 1.2s ease-out;
    }

    .path_display {
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      border-radius: 0.75em;
      margin-bottom: 1.5em;
      font-family: monospace;
      text-align: center;
    }

    .file_controls {
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 1em;
      margin-bottom: 2em;
      animation: fadeIn 1.2s ease-out;
    }

    .file_controls a {
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      color: white;
      border-radius: 0.75em;
      text-decoration: none;
      font-weight: bold;
      transition: background 0.2s ease;
    }

    .file_controls a:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    table {
      width: 100%;
      border-collapse: collapse;
      background-color: rgba(255, 255, 255, 0.05);
      border-radius: 0.75em;
      overflow: hidden;
      table-layout: fixed;
    }

    th, td {
      padding: 0.9em;
      text-align: left;
      border-bottom: 1px solid rgba(255,255,255,0.2);
      word-wrap: break-word;
      vertical-align: top;
    }

    .file_group {
      display: flex;
      flex-direction: column;
      gap: 0.6em;
      padding: 1.2em;
      transition: background 0.2s ease;
      border-radius: 0.75em;
    }

    .file_group:hover {
      background-color: rgba(255, 255, 255, 0.07);
      cursor: pointer;
    }

    .file_name {
      display: flex;
      align-items: center;
      gap: 0.8em;
      font-weight: 500;
      font-size: 1.1em;
    }

    .file_name img {
      width: 32px;
      height: 32px;
      object-fit: contain;
    }

    td.divider {
      width: 2px;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .popup_box .spinner {
      display: block;
      width: 32px; height: 32px;
      border: 4px solid rgba(255,255,255,0.2);
      border-top-color: #00c6ff;
      border-radius: 50%;
      margin: 0 auto 0.8em auto;
      animation: spin 0.8s linear infinite;
    }

    @keyframes spin { to { transform: rotate(360deg); } }
    .popup_box.processing .popup_buttons { display: none; }
  </style>

  <div class='file_box'>
    <h2>📁 LittleFS File Manager</h2>
    <div class='path_display'>)rawliteral";
  
  page += currentPath;
  page += R"rawliteral(</div>
    <div class='file_controls'>)rawliteral";
  
  if(currentPath != "/") {
    page += "<a href=\"#\" onclick=\"goBack(); return false;\">Go Back</a>";
  }

  page += "<a href='/littlefsupload?path=" + currentPath + "'>Upload</a>";
 
  page += R"rawliteral(
      <a href="#" onclick="showCreateFolder(); return false;">New Folder</a>
      <a href="#" id="downloadBtn" onclick="handleActionButton('download'); return false;">Download</a>
      <a href="#" id="renameBtn" onclick="handleActionButton('rename'); return false;">Rename</a>
      <a href="#" id="moveBtn" onclick="handleActionButton('move'); return false;">Move</a>
      <a href="#" id="deleteBtn" onclick="handleActionButton('delete'); return false;">Delete</a>
    </div>
  )rawliteral";

  if(numfiles > 0)
  {
    page += "<table>";

    while(index < numfiles) 
    {
      Fname1 = Filenames[index].filename;
      Fsize1 = Filenames[index].fsize;

      if(Filenames[index].ftype == "Dir")
      {
        icon1 = "/folder_icon";
      }
      else
      {
        if (Fname1.endsWith(".jpg") || Fname1.endsWith(".png") || Fname1.endsWith(".bmp")) icon1 = "/img_icon";
        else if (Fname1.endsWith(".mp4") || Fname1.endsWith(".avi") || Fname1.endsWith(".gif") || Fname1.endsWith(".mjpeg")) icon1 = "/video_icon";
        else if (Fname1.endsWith(".mp3") || Fname1.endsWith(".wav") || Fname1.endsWith(".aac")) icon1 = "/audio_icon";
        else if (Fname1.endsWith(".txt")) icon1 = "/txt_icon";
        else icon1 = "/file_icon";
      }

      if(index + 1 < numfiles)
      {
        Fname2 = Filenames[index + 1].filename;
        Fsize2 = Filenames[index + 1].fsize;

        if(Filenames[index + 1].ftype == "Dir")
        {
          icon2 = "/folder_icon";
        }
        else
        {
        if (Fname2.endsWith(".jpg") || Fname2.endsWith(".png") || Fname2.endsWith(".bmp")) icon2 = "/img_icon";
        else if (Fname2.endsWith(".mp4") || Fname2.endsWith(".avi") || Fname2.endsWith(".gif") || Fname2.endsWith(".mjpeg")) icon2 = "/video_icon";
        else if (Fname2.endsWith(".mp3") || Fname2.endsWith(".wav") || Fname2.endsWith(".aac")) icon2 = "/audio_icon";
        else if (Fname2.endsWith(".txt")) icon2 = "/txt_icon";
        else icon2 = "/file_icon";
        }
      }
      else
      {
        Fname2 = "";
        Fsize2 = "";
        icon2 = "";
      }

      page += "<tr>";
      page += "<td colspan='3'>";
      page += "<div class='file_group' data-filename='" + Fname1 +"' data-type='" + Filenames[index].ftype + "'>";
      page += "<div class='file_name'>";
      page += "<img src='" + icon1 + "'>";
      page += Fname1;
      page += "</div>";
      if(Filenames[index].ftype == "Dir") {
        page += "<div><strong>Items: </strong>" + Fsize1 + "</div>";
      } else {
        page += "<div><strong>Size: </strong>" + Fsize1 + "</div>";
      }
      page += "</div>";
      page += "</td>";
      page += "<td class='divider'></td>";

      if(index + 1 < numfiles) 
      {
        page += "<td colspan='3'>";
        page += "<div class='file_group' data-filename='" + Fname2 +"' data-type='" + Filenames[index + 1].ftype + "'>";
        page += "<div class='file_name'>";
        page += "<img src='" + icon2 + "'>";
        page += Fname2;
        page += "</div>";
        if(Filenames[index + 1].ftype == "Dir") {
          page += "<div><strong>Items: </strong>" + Fsize2 + "</div>";
        } else {
          page += "<div><strong>Size: </strong>" + Fsize2 + "</div>";
        }
        page += "</div>";
        page += "</td>";
        page += "</tr>";
      } 
      else
      {
        page += "<td colspan='3'></td>";
      }
      page += "</tr>";
      index += 2;
    }
    page += R"rawliteral(
      </table>
      </div>

      <div id="filePopup" class="popup_overlay">
        <div class="popup_box">
          <div id="filePopup_text"></div>
          <div class="popup_buttons">
            <button class="confirm_btn" onclick="startFileAction()">Yes</button>
            <button class="cancel_btn" onclick="closeFilePopup()">Cancel</button>
          </div>
        </div>
      </div>

      <script>
        let mode = '';
        let selectedFiles = [];
        const currentPath = ')rawliteral";
        page += currentPath;
        page += R"rawliteral(';

        function goBack() {
          const parts = currentPath.split('/').filter(p => p);
          parts.pop();
          const newPath = '/' + parts.join('/');
          window.location.href = '/littlefs_dir?path=' + encodeURIComponent(newPath);
        }

        function resetSelection() {
          selectedFiles = [];
          document.querySelectorAll('.file_group').forEach(el => el.style.backgroundColor = '');
        }

        function exitMode() {
          mode = '';
          resetSelection();
          document.querySelector('.file_box h2').textContent = '📁 LittleFS File Manager';
          document.getElementById('deleteBtn').textContent = 'Delete';
          document.getElementById('moveBtn').textContent = 'Move';
          document.getElementById('downloadBtn').textContent = 'Download';
          document.getElementById('renameBtn').textContent = 'Rename';
        }

        function handleActionButton(action) {
          if (mode !== action) {
            mode = action;
            resetSelection();
            const labels = {
              delete: 'Select files/folders to delete',
              move: 'Select files/folders to move',
              download: 'Click on a file to download',
              rename: 'Click on a file/folder to rename'
            };
            document.querySelector('.file_box h2').textContent = labels[action];
            document.getElementById(action + 'Btn').textContent = 'Cancel';
            return;
          }

          if (action === 'delete' || action === 'move') {
            if (selectedFiles.length === 0) { exitMode(); return; }
            const popup = document.getElementById('filePopup');
            const popupText = document.getElementById('filePopup_text');
            if (action === 'delete') {
              popupText.innerHTML = 'Delete ' + selectedFiles.length + ' item(s)?';
            } else {
              popupText.innerHTML = 'Move ' + selectedFiles.length + ' item(s) to:<br><input id="dest_path" type="text" placeholder="/destination/folder" value="' + currentPath + '" style="margin-top: 1em; width: 100%; padding: 0.5em;">';
            }
            popup.style.display = 'flex';
          } else {
            exitMode();
          }
        }

        function fileClickHandler(e) {
          const filename = e.currentTarget.getAttribute('data-filename');
          const type = e.currentTarget.getAttribute('data-type');
          const popup = document.getElementById('filePopup');
          const popupText = document.getElementById('filePopup_text');
          const fullPath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;

          if (mode === 'delete' || mode === 'move') {
            e.stopPropagation();
            const idx = selectedFiles.indexOf(fullPath);
            if (idx > -1) {
              selectedFiles.splice(idx, 1);
              e.currentTarget.style.backgroundColor = '';
            } else {
              selectedFiles.push(fullPath);
              e.currentTarget.style.backgroundColor = mode === 'delete' ? 'rgba(255,0,0,0.3)' : 'rgba(0,120,255,0.3)';
            }
            const label = mode === 'delete' ? 'Delete' : 'Move';
            document.getElementById(mode + 'Btn').textContent = selectedFiles.length > 0 ? `Confirm ${label} (${selectedFiles.length})` : 'Cancel';
            document.querySelector('.file_box h2').textContent = `Select files/folders to ${mode} (${selectedFiles.length} selected)`;
            return;
          }

          if (mode === 'download') {
            popupText.innerHTML = 'Download "<b>' + filename + '</b>"?';
            selectedFiles = [fullPath];
            popup.style.display = 'flex';
            return;
          }

          if (mode === 'rename') {
            popupText.innerHTML = 'Rename "<b>' + filename + '</b>": <br><input id="new_name" type="text" placeholder="New name" style="margin-top: 1em; width: 100%;">';
            selectedFiles = [fullPath];
            popup.style.display = 'flex';
            return;
          }

          if (type === 'Dir') {
            window.location.href = '/littlefs_dir?path=' + encodeURIComponent(fullPath);
          } else {
            const ext = filename.split('.').pop().toLowerCase();
            const images = ['jpg', 'jpeg', 'png', 'bmp', 'gif'];
            const videos = ['mp4', 'avi', 'webm', 'mjpeg'];
            const audio  = ['mp3', 'wav', 'aac'];
            const text   = ['txt'];
            if (images.includes(ext) || videos.includes(ext) || audio.includes(ext) || text.includes(ext)) {
              window.open(fullPath, '_blank');
            } else {
              alert("No preview available for this file type.");
            }
          }
        }

        document.querySelectorAll('.file_group').forEach(el => el.addEventListener('click', fileClickHandler));

        async function startFileAction() {
          if (mode === 'createfolder') {
            const folderName = document.getElementById('folder_name').value.trim();
            if (!folderName) { alert("Enter folder name"); return; }
            window.location.href = '/littlefscreatefolder?path=' + encodeURIComponent(currentPath) +
                                  '&name=' + encodeURIComponent(folderName);
            closeFilePopup();
            return;
          }

          if (selectedFiles.length === 0) return;

          const popup = document.getElementById('filePopup');
          const popupBox = popup.querySelector('.popup_box');
          const popupText = document.getElementById('filePopup_text');

          if (mode === 'download') {
            const filePath = selectedFiles[0];
            const filenameOnly = filePath.split('/').pop();
            closeFilePopup();
            fetch('/littlefsdownload?filename=' + encodeURIComponent(filePath))
              .then(response => {
                if (!response.ok) throw new Error('Server returned ' + response.status);
                return response.blob();
              })
              .then(blob => {
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = filenameOnly;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
                exitMode();
              })
              .catch(err => alert('Download failed: ' + err.message));
          }

          else if (mode === 'delete') {
            popupBox.classList.add('processing');
            popupText.innerHTML = '<div class="spinner"></div><div>Deleting ' + selectedFiles.length + ' item(s)...</div>';

            let deleteCount = 0;
            for (let i = 0; i < selectedFiles.length; i++) {
              try {
                const response = await fetch('/littlefsdelete?filename=' + encodeURIComponent(selectedFiles[i]) + '&path=' + encodeURIComponent(currentPath));
                if (response.ok) deleteCount++;
              } catch (err) { console.error('Delete failed:', err); }
            }
            window.location.href = '/littlefs_dir?path=' + encodeURIComponent(currentPath);
          }

          else if (mode === 'move') {
            const destPath = document.getElementById('dest_path').value;
            if (!destPath || destPath.trim() === "") { alert("Enter destination path"); return; }

            popupBox.classList.add('processing');
            popupText.innerHTML = '<div class="spinner"></div><div>Moving ' + selectedFiles.length + ' item(s)...</div>';

            let moveCount = 0;
            for (let i = 0; i < selectedFiles.length; i++) {
              try {
                const response = await fetch('/littlefsmove?source=' + encodeURIComponent(selectedFiles[i]) +
                                              '&destination=' + encodeURIComponent(destPath) +
                                              '&path=' + encodeURIComponent(currentPath));
                if (response.ok) moveCount++;
              } catch (err) { console.error('Move failed:', err); }
            }
            window.location.href = '/littlefs_dir?path=' + encodeURIComponent(currentPath);
          }

          else if (mode === 'rename') {
            const newName = document.getElementById('new_name').value;
            if (newName && newName.trim() !== "") {
              window.location.href = "/littlefsrename?old=" + encodeURIComponent(selectedFiles[0]) + "&new=" + encodeURIComponent(newName) + "&path=" + encodeURIComponent(currentPath);
            } else {
              alert("Enter a new filename");
            }
          }
        }

        function closeFilePopup() {
          const popup = document.getElementById('filePopup');
          popup.style.display = 'none';
          popup.querySelector('.popup_box').classList.remove('processing');

          if (mode === 'delete' || mode === 'move') {
            exitMode();
          } else {
            selectedFiles = [];
            document.querySelectorAll('.file_group').forEach(el => el.style.backgroundColor = '');
          }
        }

        function showCreateFolder() {
          const popup = document.getElementById('filePopup');
          const popupText = document.getElementById('filePopup_text');
          
          popupText.innerHTML = 'Create new folder in current directory:<br><input id="folder_name" type="text" placeholder="Folder name" style="margin-top: 1em; width: 100%; padding: 0.5em;">';
          popup.style.display = 'flex';
          
          mode = 'createfolder';
        }
      </script>

      <style>
        .popup_overlay {
          position: fixed;
          top: 0; left: 0;
          width: 100%; height: 100%;
          background-color: rgba(0, 0, 0, 0.7);
          display: none;
          align-items: center;
          justify-content: center;
          z-index: 9999;
        }

        .popup_box {
          background: rgba(27, 27, 27, 1);
          padding: 2em;
          border-radius: 1em;
          color: white;
          text-align: center;
          width: 90%;
          max-width: 400px;
          box-shadow: 0 4px 16px rgba(0,0,0,0.5);
        }

        .popup_buttons {
          margin-top: 1.5em;
          display: flex;
          justify-content: space-around;
        }

        .popup_buttons button {
          padding: 0.6em 1.5em;
          border: none;
          border-radius: 0.5em;
          cursor: pointer;
          font-weight: bold;
          color: white;
          transition: background 0.2s ease;
        }

        .confirm_btn {
          background-color: rgba(91, 91, 91, 1);
        }

        .confirm_btn:hover {
          background-color: #00e676;
        }

        .cancel_btn {
          background-color: rgba(91, 91, 91, 1);
        }

        .cancel_btn:hover {
          background-color: #ef5350;
        }
      </style>
    )rawliteral";
  }
  else
  {
    page += "<h3>No Files Found</h3>";
    page += "</div>";
  }
  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_LittleFS_File_Upload(AsyncWebServerRequest *request) {
  if(request->method() == HTTP_GET) 
  {
    String uploadPath = "/";
    if(request->hasParam("path")) 
    {
      uploadPath = request->getParam("path")->value();
      if(!uploadPath.startsWith("/")) uploadPath = "/" + uploadPath;
    }

    String page = HTML_Header();
    page += R"rawliteral(
    <style>
      .file_box {
        background: rgba(38, 38, 38, 0.5);
        backdrop-filter: blur(5px);
        box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
        border-radius: 1em;
        padding: 2em;
        margin: 3em auto;
        width: fit-content;
        color: white;
        font-family: "Segoe UI", sans-serif;
        font-size: 1.05em;
        animation: fadeIn 1.2s ease-out;
      }

      .upload_form {
        display: flex;
        flex-direction: column;
        gap: 1.5em;
        align-items: center;
      }

      .custom_file_input {
        position: relative;
        display: inline-block;
        overflow: hidden;
        border-radius: 0.75em;
        background: rgba(255, 255, 255, 0.1);
        cursor: pointer;
        font-weight: bold;
        padding: 0.75em 1.2em;
        color: white;
        transition: background 0.2s ease;
        width: 100%;
        text-align: center;
      }

      .custom_file_input:hover {
        background-color: rgba(255, 255, 255, 0.3);
        transform: scale(1.02);
      }

      .custom_file_input input[type="file"] {
        position: absolute;
        left: 0;
        top: 0;
        opacity: 0;
        cursor: pointer;
        width: 100%;
        height: 100%;
      }

      .filename_note {
        font-size: 0.9em;
        font-style: italic;
        color: #ccc;
      }

      .upload_button, .toggle_button {
        width: 100%;
        background: rgba(255, 255, 255, 0.1);
        padding: 0.75em 1.5em;
        color: white;
        border: none;
        border-radius: 0.75em;
        font-weight: bold;
        cursor: pointer;
        position: relative;
        overflow: hidden;
        transition: background 0.2s ease;
      }

      .upload_button:hover, .toggle_button:hover {
        background-color: rgba(255, 255, 255, 0.3);
        transform: scale(1.02);
      }

      .upload_button .progress_fill {
        background: linear-gradient(90deg, #00c6ff, #0072ff);
        position: absolute;
        left: 0;
        top: 0;
        height: 100%;
        width: 0%;
        z-index: 0;
        transition: width 0.2s ease;
      }

      .upload_button span {
        position: relative;
        z-index: 1;
      }

      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(20px); }
        to { opacity: 1; transform: translateY(0); }
      }
    </style>

    <div class="file_box">
      <h2>Upload Files/Folders</h2>

      <div class="upload_form">
        <label class="custom_file_input">
          <span id="inputLabel">Choose Files</span>
          <input id="fileInput" type="file" multiple>
        </label>

        <button type="button" class="toggle_button" onclick="toggleUploadMode()">
          <span id="modeText">Switch to Folder Mode</span>
        </button>

        <div id="fileNameNote" class="filename_note">No files selected</div>

        <button type="button" class="upload_button" id="uploadBtn">
          <div class="progress_fill" id="progressFill"></div>
          <span id="uploadText">Upload</span>
        </button>
      </div>
    </div>
    <script>
      const uploadPath = ')rawliteral" + uploadPath + R"rawliteral(';
      const fileInput = document.getElementById('fileInput');
      const fileNameNote = document.getElementById('fileNameNote');
      const uploadBtn = document.getElementById('uploadBtn');
      const uploadText = document.getElementById('uploadText');
      const progressFill = document.getElementById('progressFill');
      const inputLabel = document.getElementById('inputLabel');
      const modeText = document.getElementById('modeText');
      
      let folderMode = false;

      function toggleUploadMode() {
        folderMode = !folderMode;
        fileInput.value = '';
        
        if (folderMode) {
          fileInput.setAttribute('webkitdirectory', '');
          fileInput.setAttribute('directory', '');
          fileInput.removeAttribute('multiple');
          inputLabel.textContent = 'Choose Folder';
          modeText.textContent = 'Switch to File Mode';
          fileNameNote.textContent = 'No folder selected';
        } else {
          fileInput.removeAttribute('webkitdirectory');
          fileInput.removeAttribute('directory');
          fileInput.setAttribute('multiple', '');
          inputLabel.textContent = 'Choose Files';
          modeText.textContent = 'Switch to Folder Mode';
          fileNameNote.textContent = 'No files selected';
        }
      }

      fileInput.addEventListener('change', () => {
        const files = fileInput.files;
        if (files.length === 0) {
          fileNameNote.textContent = folderMode ? 'No folder selected' : 'No files selected';
        } else if (files.length === 1) {
          fileNameNote.textContent = `Selected: ${files[0].name}`;
        } else {
          fileNameNote.textContent = `Selected: ${files.length} files`;
        }
      });

      function getTotalSize(files) {
        let total = 0;
        for (let i = 0; i < files.length; i++) {
          total += files[i].size;
        }
        return total;
      }

      uploadBtn.addEventListener('click', async () => {
        const files = fileInput.files;
        if (files.length === 0) return alert('Please select files');

        uploadBtn.disabled = true;
        let totalUploaded = 0;

        for (let i = 0; i < files.length; i++) {
          const file = files[i];
          const formData = new FormData();
          formData.append('filename', file);
          const relPath = file.webkitRelativePath || file.name;
          const base = uploadPath === '/' ? '' : uploadPath;
          formData.append('filepath', base + '/' + relPath);

          try {
            await new Promise((resolve, reject) => {
              const xhr = new XMLHttpRequest();
              
              xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                  const filePercent = (e.loaded / e.total) * 100;
                  const overallPercent = ((totalUploaded + e.loaded) / getTotalSize(files)) * 100;
                  progressFill.style.width = overallPercent + '%';
                  uploadText.textContent = `${Math.round(overallPercent)}%`;
                }
              };

              xhr.onload = () => {
                if (xhr.status === 200) {
                  totalUploaded += file.size;
                  resolve();
                } else {
                  reject();
                }
              };

              xhr.onerror = () => reject();
              
              xhr.open('POST', '/littlefsupload', true);
              xhr.send(formData);
            });
          } catch (err) {
            console.error('Upload failed:', err);
          }
        }
        uploadText.textContent = 'Done!';
        setTimeout(() => {
          window.location.href = '/littlefs_dir?path=' + encodeURIComponent(uploadPath);
        }, 800);
      });
    </script>
    )rawliteral";
    request->send(200, "text/html", page);
    return;
  }
}

void AdiWiFiManager::on_LittleFS_File_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {

  static int start = 0;
  static int uploadtime = 0;
  static int uploadsize = 0;
  static bool uploadAborted = false;
  static String filepath = "";

  if(!index) 
  {
    uploadAborted = false;
    filepath = "/";

    if(request->hasArg("filepath"))
    {
      filepath += request->arg("filepath");
    }
    else
    {
      filepath += filename;
    }

    size_t freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
    if(request->contentLength() > 0 && request->contentLength() > freeSpace) 
    {
      _DebugLog("Upload rejected: not enough space for " + filepath + " (" + String(request->contentLength()) + " needed, " + String(freeSpace) + " free)");
      uploadAborted = true;
      request->send(507, "text/plain", "Not enough storage space");
      return;
    }

    int lastSlash = filepath.lastIndexOf('/');
    if(lastSlash > 0)
    {
      String dirPath = filepath.substring(0, lastSlash);
      LittleFS_createDirectoryRecursive(dirPath);
    }

    if(request->_tempFile) request->_tempFile.close();

    request->_tempFile = LittleFS.open(filepath, FILE_WRITE);
    if(!request->_tempFile)
    {
      _DebugLog("Failed to create file: " + filepath);
      return;
    }
    _DebugLog("Started upload: " + filepath);
    start = millis();
  }

  if(uploadAborted) return;

  if(request->_tempFile && len) 
  {
    size_t freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
    if(len > freeSpace) 
    {
      _DebugLog("Upload aborted mid-transfer: ran out of space");
      request->_tempFile.close();
      LittleFS.remove(filepath);
      uploadAborted = true;
      request->send(507, "text/plain", "Ran out of storage space");
      return;
    }

    size_t written = request->_tempFile.write(data, len);
    if(written != len) {
      _DebugLog("Write error: expected " + String(len) + ", wrote " + String(written));
    }
  }

  if(final && request->_tempFile) 
  {
    uploadsize = request->_tempFile.size();
    request->_tempFile.flush();
    request->_tempFile.close();
    uploadtime = millis() - start;
    float speed = (uploadsize / 1024.0) / (uploadtime / 1000.0);
    _DebugLog("Upload finished: " + String(uploadsize) + " bytes in " + String(uploadtime) + "ms (" + String(speed, 2) + " KB/s)");
    request->send(200);
  }
}

void AdiWiFiManager::Handle_LittleFS_File_Download(AsyncWebServerRequest *request) {
  if(!request->hasParam("filename")) 
  {
    request->send(400, "text/plain", "Missing filename");
    _DebugLog("Download Handler failed, missing filename");
    return;
  }
  String filename = request->getParam("filename")->value();
  if(!LittleFS.exists(filename)) 
  {
    request->send(404, "text/plain", "File not found");
    _DebugLog("Download Handler failed, file not found");
    return;
  }
  
  File file = LittleFS.open(filename);
  if(file.isDirectory()) {
    file.close();
    request->send(400, "text/plain", "Cannot download folders directly");
    return;
  }
  file.close();
  
  String contentType = "application/octet-stream";
  if (filename.endsWith(".png")) contentType = "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) contentType = "image/jpeg";
  else if (filename.endsWith(".txt")) contentType = "text/plain";
  else if (filename.endsWith(".mp4")) contentType = "video/mp4";
  else if (filename.endsWith(".wav")) contentType = "audio/wav";
  request->send(LittleFS, filename, contentType, true);
}

void AdiWiFiManager::Handle_LittleFS_File_Delete(AsyncWebServerRequest *request) {
  if(!request->hasParam("filename")) 
  {
    request->send(400, "text/html", "Missing 'filename' parameter");
    return;
  }
  String filename = request->getParam("filename")->value();
  if(!filename.startsWith("/")) filename = "/" + filename;
  
  String currentPath = "/";
  if(request->hasParam("path")) 
  {
    currentPath = request->getParam("path")->value();
  } 
  else 
  {
    int lastSlash = filename.lastIndexOf('/');
    if(lastSlash > 0) currentPath = filename.substring(0, lastSlash);
  }
  
  if(LittleFS.exists(filename)) 
  {
    LittleFS_deleteRecursive(filename);
    _DebugLog("Deleted: " + filename);
    request->send(200, "text/plain", "OK");
  } 
  else 
  {
    _DebugLog("Failed to delete, file not found");
    request->send(404, "text/plain", "Not found");
  }
}

void AdiWiFiManager::Handle_LittleFS_File_Rename(AsyncWebServerRequest *request) {
  if(!request->hasParam("old") || !request->hasParam("new")) 
  {
    request->send(400, "text/html", "Missing 'old' or 'new' filename parameter");
    return;
  }
  String oldName = request->getParam("old")->value();
  String newName = request->getParam("new")->value();
  if(!oldName.startsWith("/")) oldName = "/" + oldName;
  
  String currentPath = "/";
  if(request->hasParam("path")) currentPath = request->getParam("path")->value();
  else 
  {
    int lastSlash = oldName.lastIndexOf('/');
    if(lastSlash > 0) currentPath = oldName.substring(0, lastSlash);
  }
  
  String newFullPath = currentPath;
  if(!newFullPath.endsWith("/")) newFullPath += "/";
  newFullPath += newName;
  
  if(oldName != newFullPath && oldName != "/" && newFullPath != "/") 
  {
    if(LittleFS.exists(oldName)) 
    {
      if(!LittleFS.exists(newFullPath)) 
      {
        if(LittleFS.rename(oldName, newFullPath)) 
        {
          _DebugLog("Renamed from " + oldName + " to " + newFullPath);
        } 
        else 
        {
          _DebugLog("Failed to rename");
        }
      }
      else
      {
        _DebugLog("A file with the new name already exists");
      }
    }
    else
    {
      _DebugLog("Original file does not exist");
    }
  }
  request->redirect("/littlefs_dir?path=" + currentPath);
}

void AdiWiFiManager::Handle_LittleFS_File_Move(AsyncWebServerRequest *request) {
  if(!request->hasParam("source") || !request->hasParam("destination")) 
  {
    request->send(400, "text/html", "Missing parameters");
    return;
  }
  
  String sourcePath = request->getParam("source")->value();
  String destFolder = request->getParam("destination")->value();
  
  if(!sourcePath.startsWith("/")) sourcePath = "/" + sourcePath;
  if(!destFolder.startsWith("/")) destFolder = "/" + destFolder;
  if(!destFolder.endsWith("/")) destFolder += "/";
  
  String currentPath = "/";
  if(request->hasParam("path")) currentPath = request->getParam("path")->value();
  
  // Extract filename from source
  int lastSlash = sourcePath.lastIndexOf('/');
  String filename = sourcePath.substring(lastSlash + 1);
  
  String destPath = destFolder + filename;
  
  if(sourcePath != destPath && LittleFS.exists(sourcePath)) 
  {
    if(!LittleFS.exists(destPath)) 
    {
      if(LittleFS.rename(sourcePath, destPath)) 
      {
        _DebugLog("Moved from " + sourcePath + " to " + destPath);
      } 
      else 
      {
        _DebugLog("Failed to move");
      }
    }
    else
    {
      _DebugLog("File already exists at destination");
    }
  }
  
  request->redirect("/littlefs_dir?path=" + currentPath);
}

void AdiWiFiManager::Handle_LittleFS_Create_Folder(AsyncWebServerRequest *request) {
  if(!request->hasParam("path") || !request->hasParam("name")) 
  {
    request->send(400, "text/html", "Missing parameters");
    return;
  }
  
  String basePath = request->getParam("path")->value();
  String folderName = request->getParam("name")->value();
  
  if(!basePath.startsWith("/")) basePath = "/" + basePath;
  if(!basePath.endsWith("/")) basePath += "/";
  
  String fullPath = basePath + folderName;
  
  if(!LittleFS.exists(fullPath)) 
  {
    if(LittleFS.mkdir(fullPath)) 
    {
      _DebugLog("Created folder: " + fullPath);
    } 
    else 
    {
      _DebugLog("Failed to create folder");
    }
  }
  else
  {
    _DebugLog("Folder already exists");
  }
  
  request->redirect("/littlefs_dir?path=" + fullPath);
}

#endif


void AdiWiFiManager::Handle_OTA(AsyncWebServerRequest *request) {
  String page = HTML_Header();
  page += R"rawliteral(
  <style>
    .file_box {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(5px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.5);
      border-radius: 1em;
      padding: 2em;
      margin: 3em auto;
      width: fit-content;
      color: white;
      font-family: "Segoe UI", sans-serif;
      font-size: 1.05em;
      animation: fadeIn 1.2s ease-out;
    }

    .upload_form {
      display: flex;
      flex-direction: column;
      gap: 1.5em;
      align-items: center;
    }

    .custom_file_input {
      position: relative;
      display: inline-block;
      overflow: hidden;
      border-radius: 0.75em;
      background: rgba(255, 255, 255, 0.1);
      cursor: pointer;
      font-weight: bold;
      padding: 0.75em 1.2em;
      color: white;
      transition: background 0.2s ease;
      width: 100%;
      text-align: center;
    }

    .custom_file_input:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    .custom_file_input input[type="file"] {
      position: absolute;
      left: 0;
      top: 0;
      opacity: 0;
      cursor: pointer;
      width: 100%;
      height: 100%;
    }

    .filename_note {
      font-size: 0.9em;
      font-style: italic;
      color: #ccc;
    }

    .upload_button {
      width: 100%;
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.5em;
      color: white;
      border: none;
      border-radius: 0.75em;
      font-weight: bold;
      cursor: pointer;
      position: relative;
      overflow: hidden;
      transition: background 0.2s ease;
    }

    .upload_button:hover {
      background-color: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    .upload_button .progress_fill {
      background: linear-gradient(90deg, #00c6ff, #0072ff);
      position: absolute;
      left: 0;
      top: 0;
      height: 100%;
      width: 0%;
      z-index: 0;
      transition: width 0.2s ease;
    }

    .upload_button span {
      position: relative;
      z-index: 1;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }

  </style>

  <div class="file_box">
    <h2>OTA Firmware Update</h2>

    <form id="otaForm" class="upload_form" onsubmit="startOTA(event)">
      <label class="custom_file_input">
        Choose .bin File
        <input id="fileInput" type="file">
      </label>

      <div id="fileNameNote" class="filename_note">No file selected</div>

      <button type="submit" class="upload_button">
        <div class="progress_fill" id="progressFill"></div>
        <span id="uploadText">Start Update</span>
      </button>
    </form>
  </div>

  <script>
    const fileInput = document.getElementById('fileInput');
    const fileNameNote = document.getElementById('fileNameNote');
    const uploadText = document.getElementById('uploadText');
    const progressFill = document.getElementById('progressFill');

    fileInput.addEventListener('change', () => {
      const file = fileInput.files[0];
      fileNameNote.textContent = file ? `Selected: ${file.name}` : 'No file selected';
    });

    async function startOTA(e) {
      e.preventDefault();

      const file = fileInput.files[0];
      if (!file) return alert("Please select a .bin file first.");

      uploadText.textContent = "Starting...";

      const startRes = await fetch("/ota/start");
      if (!startRes.ok) {
        uploadText.textContent = "Start Failed!";
        return;
      }

      uploadText.textContent = "Uploading...";

      const formData = new FormData();
      formData.append("update", file, "firmware.bin");

      const xhr = new XMLHttpRequest();
      xhr.open("POST", "/ota/upload", true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const percent = (e.loaded / e.total) * 100;
          progressFill.style.width = percent + "%";
        }
      };

      xhr.onload = () => {
        if (xhr.status === 200) {
          uploadText.textContent = "Upload Complete!";
          setTimeout(() => location.reload(), 3000);
        } else {
          uploadText.textContent = "Upload Failed!";
          progressFill.style.width = "0%";
        }
      };

      xhr.onerror = () => {
        uploadText.textContent = "Upload Error!";
      };

      xhr.send(formData);
    }
  </script>
  )rawliteral";

  request->send(200, "text/html", page);
}

String AdiWiFiManager::ConvBinUnits(uint64_t bytes, int resolution) {
  if(bytes < 1024) {
    return String((long long)bytes) + " B";
  }
  else if(bytes < 1024 * 1024) {
    return String((bytes / 1024.0), resolution) + " KB";
  }
  else if(bytes < (1024ULL * 1024 * 1024)) {
    return String((bytes / 1024.0 / 1024.0), resolution) + " MB";
  }
  else if(bytes < (1024ULL * 1024 * 1024 * 1024)) {
    return String((bytes / 1024.0 / 1024.0 / 1024.0), resolution) + " GB";
  }
  else return "";
}

String AdiWiFiManager::EncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case (WIFI_AUTH_OPEN):
      return "OPEN";
    case (WIFI_AUTH_WEP):
      return "WEP";
    case (WIFI_AUTH_WPA_PSK):
      return "WPA PSK";
    case (WIFI_AUTH_WPA2_PSK):
      return "WPA2 PSK";
    case (WIFI_AUTH_WPA_WPA2_PSK):
      return "WPA WPA2 PSK";
    case (WIFI_AUTH_WPA2_ENTERPRISE):
      return "WPA2 ENTERPRISE";
    case (WIFI_AUTH_MAX):
      return "WPA2 MAX";
    default:
      return "";
  }
}

void AdiWiFiManager::Display_System_Info(AsyncWebServerRequest *request) {

  #ifdef SD_ENABLED
    SD_Directory();
  #endif

  #ifdef LittleFS_ENABLED
    LittleFS_Directory();
  #endif

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  String page = HTML_Header();

  page += R"rawliteral(
  <style>
    .info_card {
      background: rgba(38, 38, 38, 0.5);
      backdrop-filter: blur(6px);
      box-shadow: 0 4px 18px rgba(0, 0, 0, 0.6);
      border-radius: 1.2em;
      padding: 2em;
      margin: 2em auto;
      width: fit-content;
      color: white;
      font-family: "Segoe UI", sans-serif;
      font-size: 1em;
      animation: fadeIn 1.2s ease-out;
    }

    .info_card h3 {
      font-size: 1.5em;
      margin-bottom: 1em;
      text-align: center;
    }

    .info_card h4 {
      margin-top: 1.5em;
      margin-bottom: 0.8em;
    }

    .info_card form {
      margin-top: 2em;
    }

    .info_card table {
      border-collapse: collapse;
      width: 100%;
      margin-top: 1em;
    }

    .info_card th, .info_card td {
      padding: 0.5em 1em;
      border: 1px solid rgba(255, 255, 255, 0.2);
      text-align: left;
    }

    .info_card select, .info_card button {
      color: white;
      background: rgba(255, 255, 255, 0.1);
      padding: 0.75em 1.2em;
      text-decoration: none;
      font-size: 1.1em;
      transition: background 0.3s ease;
      border: none;
      border-radius: 0.75em;
      font-weight: bold;
      cursor: pointer;
      margin-top: 1em;
      margin-bottom: 1em;
    }

    .info_card select:hover, .info_card button:hover {
      background: rgba(255, 255, 255, 0.3);
      transform: scale(1.02);
    }

    .input-group {
      display: flex;
      align-items: center;
      gap: 0.75em;
      margin: 8px 0;
      background: rgba(255, 255, 255, 0.05);
      padding: 0.6em 0.9em;
      border-radius: 0.75em;
      flex-wrap: nowrap;
    }

    .input-group label {
      flex: 0 0 120px;
      text-align: left;
      font-weight: 600;
    }

    .input-group select {
      background: rgba(255, 255, 255, 0.1);
      color: white;
      border: 1px solid rgba(255, 255, 255, 0.2);
      border-radius: 0.5em;
      padding: 0.4em 0.6em;
      appearance: none;
      font-size: 0.95em;
      flex: 1;
    }

    .input-group select option {
      background-color: rgba(38, 38, 38, 0.95);
      color: #f0f0f0;
      padding: 0.5em;
    }

    .button-group {
      display: flex;
      flex-direction: column;
      gap: 0.8em;
      margin-top: 1.5em;
    }

    .button-group button {
      width: 100%;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    } 
  </style>
  )rawliteral";

  page += "<div class='info_card'>";
  page += "<h3>System Information</h3><table>";
  page += "<tr><th>Build Date</th><td>" + String(BUILD_) + "</td></tr>";
  page += "<tr><th>Free PSRAM</th><td>" + ConvBinUnits(ESP.getFreePsram(), 1) + "</td></tr>";
  page += "</table>";

  page += "<h4>CPU Info</h4><table>";
  page += "<tr><th>CPU Cores</th><td>" + String(chip_info.cores) + "</td></tr>";
  page += "<tr><th>Chip Revision</th><td>" + String(chip_info.revision) + "</td></tr>";
  page += "<tr><th>Flash Type</th><td>" + String((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External") + "</td></tr>";
  page += "<tr><th>Flash Size</th><td>" + ConvBinUnits(ESP.getFlashChipSize(), 1) + "</td></tr>";
  page += "<tr><th>Free Heap</th><td>" + ConvBinUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  page += "</table>";

  page += "<h4>WiFi Info</h4><table>";
  page += "<tr><th>LAN IP</th><td>" + WiFi.localIP().toString() + "</td></tr>";
  page += "<tr><th>MAC Address</th><td>" + WiFi.BSSIDstr() + "</td></tr>";
  page += "<tr><th>SSID</th><td>" + WiFi.SSID() + "</td></tr>";
  page += "<tr><th>RSSI</th><td>" + String(WiFi.RSSI()) + " dB</td></tr>";
  page += "<tr><th>Channel</th><td>" + String(WiFi.channel()) + "</td></tr>";
  
  String currentEncryption = "Unknown";
  String connectedSSID = WiFi.SSID();
  for(int i = 0; i < wifiSSIDCount; i++)
  {
    if(wifiSSIDs[i].SSID == connectedSSID) 
    {
      currentEncryption = EncryptionType((wifi_auth_mode_t)wifiSSIDs[i].encryptionType);
      break;
    }
  }
  page += "<tr><th>Encryption</th><td>" + currentEncryption + "</td></tr>";
  page += "</table>";

  #ifdef SD_ENABLED
    page += "<h4>SD Card</h4><table>";
    page += "<tr><th>Total Space</th><td>" + ConvBinUnits(SD.totalBytes(), 1) + "</td></tr>";
    page += "<tr><th>Used Space</th><td>" + ConvBinUnits(SD.usedBytes(), 1) + "</td></tr>";
    page += "<tr><th>Free Space</th><td>" + ConvBinUnits(SD.totalBytes() - SD.usedBytes(), 1) + "</td></tr>";
    page += "</table>";
  #endif

  #ifdef LittleFS_ENABLED
    page += "<h4>LittleFS</h4><table>";
    page += "<tr><th>Total Space</th><td>" + ConvBinUnits(LittleFS.totalBytes(), 1) + "</td></tr>";
    page += "<tr><th>Used Space</th><td>" + ConvBinUnits(LittleFS.usedBytes(), 1) + "</td></tr>";
    page += "<tr><th>Free Space</th><td>" + ConvBinUnits(LittleFS.totalBytes() - LittleFS.usedBytes(), 1) + "</td></tr>";
    page += "</table>";
  #endif


  page += "<div class='button-group'>";
  page += "<button onclick=\"showPopup('/reboot', 'Reboot Device', 'This will restart the device. Continue?')\">Reboot</button>";
  page += "</div>";

  page += "</div>";

  request->send(200, "text/html", page);
}

void AdiWiFiManager::Handle_Page_Not_Found(AsyncWebServerRequest *request) {
  String page = HTML_Header();
  page += R"rawliteral(
    <style>
      .notfound {
        max-width: 800px;
        margin: 4em auto;
        background: rgba(38, 38, 38, 0.5);
        border-radius: 1.5em;
        backdrop-filter: blur(8px);
        box-shadow: 0 6px 20px rgba(0, 0, 0, 0.6);
        padding: 2.5em;
        text-align: center;
        animation: fadeIn 1.2s ease-out;
      }

      .notfound h1 {
        font-size: 2.5em;
        margin-bottom: 0.4em;
        color: #ffffff;
        text-shadow: 2px 2px 6px rgba(0, 0, 0, 0.7);
      }

      .notfound h2 {
        font-size: 1.3em;
        font-weight: 400;
        color: #dddddd;
        margin-bottom: 1em;
        text-shadow: 1px 1px 3px rgba(0, 0, 0, 0.6);
      }

      .notfound h3 {
        margin-top: 2em;
        font-size: 1.1em;
        color: #eeeeee;
        opacity: 0.8;
        letter-spacing: 1px;
      }

      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(20px); }
        to { opacity: 1; transform: translateY(0); }
      }
    </style>

    <div class="notfound">
      <h1>Error 404</h1>
      <h2>Page Not Found</h2>
      <h3>The page you were looking for was not found, it may have been moved or is currently unavailable.</h3>
    </div>
  )rawliteral";
  request->send(200, "text/html", page);
}

//Public Functions

// Start the web server
void AdiWiFiManager::StartWebserver() {

  if(webserver_running) 
  { 
    _DebugLog("Webserver already running"); 
    return; 
  }

  if(!AP_MODE && !WiFi.isConnected()) 
  { 
    _DebugLog("Either AP or STA mode must be active to start the webserver"); 
    return; 
  }

  if(AP_MODE) //captive server only if running on access point mode
  {
    DNS.setErrorReplyCode(DNSReplyCode::NoError);

    if(!DNS.start(53, "*", WiFi.softAPIP())) Serial.println("Could not start Captive DNS Server!");
  }

  if(MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
  }
  else
  {
    _DebugLog("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_Home(request);
  });

  server.on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_Home(request);
  });

  server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_Home(request);
  });

  server.on("/redirect", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_Home(request);
  });

  server.on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(204);
  });

  server.on("/connecttest.txt", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_Home(request);
  });

  server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_Wifi(request);
  });

  server.on("/wifi_start_scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
    scan_now = true;
    scan_complete = false;
    last_scan = 0;
    request->send(204);
  });

  server.on("/wifi_list", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_Wifi_List(request);
  });

  server.on("/wifi_save", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_Wifi_Save(request);
  });

#ifdef SD_ENABLED

  server.on("/sd_dir", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_SD_Dir(request);
  });

  server.on("/sdupload", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_File_Upload(request);
  });

  server.on("/sdupload", HTTP_POST, 
    [this](AsyncWebServerRequest *request) {},
    [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
      on_SD_File_Upload(request, filename, index, data, len, final);
    }
  );

  server.on("/sddownload", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_File_Download(request);
  });

  server.on("/sddelete", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_File_Delete(request);
  });

  server.on("/sdrename", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_File_Rename(request);
  });

  server.on("/sdmove", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_File_Move(request);
  });

  server.on("/sdcreatefolder", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_SD_Create_Folder(request);
  });

#endif

#ifdef LittleFS_ENABLED

  server.on("/littlefs_dir", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Handle_LittleFS_Dir(request);
  });

  server.on("/littlefsupload", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_File_Upload(request);
  });

  server.on("/littlefsupload", HTTP_POST, 
    [this](AsyncWebServerRequest *request) {},
    [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
      on_LittleFS_File_Upload(request, filename, index, data, len, final);
    }
  );

  server.on("/littlefsdownload", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_File_Download(request);
  });

  server.on("/littlefsdelete", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_File_Delete(request);
  });

  server.on("/littlefsrename", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_File_Rename(request);
  });

  server.on("/littlefsmove", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_File_Move(request);
  });

  server.on("/littlefscreatefolder", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_LittleFS_Create_Folder(request);
  });

#endif

  server.on("/system", HTTP_GET, [this](AsyncWebServerRequest * request) {
    Display_System_Info(request);
  });

  server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });

  server.serveStatic("/wifi_loading", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_loading.gif").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_locked_full", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_locked_full.png").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_locked_half", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_locked_half.png").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_locked_low", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_locked_low.png").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_unlocked_full", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_unlocked_full.png").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_unlocked_half", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_unlocked_half.png").setCacheControl("max-age=86400");
  server.serveStatic("/wifi_unlocked_low", ASSETS_LOCATION, "/Assets/OtherIcons/wifi_unlocked_low.png").setCacheControl("max-age=86400");

  server.on("/asset_bg", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if(!request->hasParam("f"))
    {
      request->send(404); 
      return;
    }
    
    String filename = request->getParam("f")->value();
    if(filename.indexOf('/') != -1 || filename.indexOf("..") != -1)
    { 
      request->send(400); 
      return;
    }

    String path = "/Assets/Backgrounds/" + filename;
    if(!ASSETS_LOCATION.exists(path)) 
    { 
      request->send(404); 
      return; 
    }
    request->send(ASSETS_LOCATION, path);
  });

  server.on("/asset_icon", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if(!request->hasParam("f")) 
    { 
      request->send(404); 
      return; 
    }

    String filename = request->getParam("f")->value();
    if(filename.indexOf('/') != -1 || filename.indexOf("..") != -1) 
    { 
      request->send(400); 
      return; 
    }

    String path = "/Assets/MainIcons/" + filename;
    if(!ASSETS_LOCATION.exists(path)) 
    { 
      request->send(404); 
      return; 
    }
    request->send(ASSETS_LOCATION, path);
  });

  server.serveStatic("/img_icon", ASSETS_LOCATION, "/Assets/OtherIcons/img_icon.png").setCacheControl("max-age=86400");
  server.serveStatic("/video_icon", ASSETS_LOCATION, "/Assets/OtherIcons/video_icon.png").setCacheControl("max-age=86400");
  server.serveStatic("/txt_icon", ASSETS_LOCATION, "/Assets/OtherIcons/txt_icon.png").setCacheControl("max-age=86400");
  server.serveStatic("/file_icon", ASSETS_LOCATION, "/Assets/OtherIcons/file_icon.png").setCacheControl("max-age=86400");
  server.serveStatic("/folder_icon", ASSETS_LOCATION, "/Assets/OtherIcons/folder_icon.png").setCacheControl("max-age=86400");
  server.serveStatic("/audio_icon", ASSETS_LOCATION, "/Assets/OtherIcons/audio_icon.png").setCacheControl("max-age=86400");

  server.serveStatic("/", ASSETS_LOCATION, "/");

  server.on("/update", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Handle_OTA(request);
  });
  
  server.on("/ota/start", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      StreamString str;
      Update.printError(str);
      _update_error_str = str.c_str();
      _DebugLog(_update_error_str.c_str());
    }
    request->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
  });

  server.on("/ota/upload", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(
        (Update.hasError()) ? 400 : 200, 
        "text/plain", 
        (Update.hasError()) ? _update_error_str.c_str() : "OK"
      );
      response->addHeader("Connection", "close");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index) {
        _current_progress_size = 0;
        _DebugLog("OTA upload started: " + filename);
      }
      if(len) {
        size_t written = Update.write(data, len);
        if(written != len) {
          _DebugLog("Write failed. Written " + String(written) + " of " + String(len));
          return;
        }
        _current_progress_size += len;
      }
      if(final) {
        _DebugLog("OTA upload finished. Total size: " + String(_current_progress_size) + " bytes");
        if(!Update.end(true)) {
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _DebugLog(_update_error_str.c_str());
        } else {
          _DebugLog("Update complete. Rebooting...");
          ESP.restart();
        }
      }
    }
  );

  server.onNotFound([this](AsyncWebServerRequest *request) {
    Handle_Page_Not_Found(request);
  });

  server.begin();
  webserver_running = true;
  _DebugLog("Webserver started");
}

// Stop the web server
void AdiWiFiManager::StopWebserver() {

  if(!webserver_running) return; 

  server.end();
  MDNS.end();
  DNS.stop();

  webserver_running = false;
  _DebugLog("Webserver stopped!");
}

// If set to true, the webserver will remain active after a successful WiFi connection.
void AdiWiFiManager::WB_StaysActive(bool active) {
  wb_stays_active = active;
}

// Connect to WiFi using the provided SSID and password. If the connection fails, it can optionally start an access point.
void AdiWiFiManager::connectToWiFi(bool ap_on_fail, String sta_ssid, String sta_pass) {

	_DebugLog("Attempting WiFi Connection..");
  AP_MODE = false;
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);

  _connectToWiFi("", "");

  if(wifi_status != WL_CONNECTED)
  {
    WiFi.disconnect(true, true);
    _DebugLog("Could not connect to wifi..");

    if(ap_on_fail)
    {
      AP_MODE = true;

      if(wifi_status != WL_CONNECTED)
      {
        _DebugLog("- Starting AP..");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_AP);
        delay(100);
        _DebugLog("Configuring access point: " + String(AP_SSID) + " with password: " + String(AP_PASS));
        WiFi.softAP(AP_SSID, AP_PASS);
        delay(500);
        _DebugLog("AP Started, IP:" + WiFi.softAPIP().toString());

        StartWebserver();
      }
    }
  }
  else
  {
    if(!wb_stays_active) StopWebserver();
  }
}

// -2: Scan Failed, -1: Scan Running, 0: No Networks Found
wifi_ssid_count_t AdiWiFiManager::getScanResults(WiFiResult* &results) {
  results = wifiSSIDs;
  return wifiSSIDCount;
}

void AdiWiFiManager::setDebugCallback(DebugLogCallback callback) {
  debugLogCallback = callback;
}

uint8_t AdiWiFiManager::getWiFiStatus() { 
  return wifi_status; 
}

void AdiWiFiManager::loop() {
  if(wifi_status != WL_CONNECTED && AP_MODE) DNS.processNextRequest();
  if(scan_now) WIFI_Scan();
  if(connect_to_new_network) _connectToNewWiFi();
}

void AdiWiFiManager::disconnect() {
	WiFi.disconnect(true);
	AP_MODE = false;
}

void AdiWiFiManager::eraseSavedWiFi() {
	_preferences.begin("wifi", false); // Note: Namespace name is limited to 15 chars
	_preferences.clear(); // erase all stored passwords
	delay(300);
	_preferences.end();
}