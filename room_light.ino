#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "";
const char* password = "";

IPAddress staticIP(192, 168, 1, 103);
IPAddress gateway(192, 168, 1, 1);  
IPAddress subnet(255, 255, 255, 0);    

#define BAUD_RATE 115200

const char* PARAM_INPUT_1 = "state";

const int relayPins[] = {0, 2}; 
int displayedRelayStates[] = {HIGH, HIGH}; 

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<html lang="tr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Room Web Server - Equ</title>
  <link rel="icon" href="https://cdn.glitch.global/d03d2042-9967-4cf1-9859-fbce716a58fe/d03d2042-9967-4cf1-9859-fbce716a58fe_thumbnails_Dark%20eye.ico?v=1718718684891" type="image/x-icon"> 
  <style>
    html {font-family: Arial; display: inline-block; text-align: center; background-color: #1f1f1f; color: white;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
<h2>Room Web Server</h2>
%BUTTONPLACEHOLDER%

<script>

function toggleCheckbox(element, id) {
  var xhr = new XMLHttpRequest();
  if(element.checked) {
    xhr.open("GET", "/update?id=" + id + "&state=0", true); 
  } else {
    xhr.open("GET", "/update?id=" + id + "&state=1", true);
  }
  xhr.send();
}

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      for(var i = 0; i < data.length; i++) {
        var id = data[i].id;
        var state = data[i].state;
        var inputChecked = (state == 0) ? true : false;
        document.getElementById("output" + id).checked = inputChecked;
        document.getElementById("outputState" + id).innerHTML = (inputChecked ? "On" : "Off");
      }
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000);

</script>
</body>
</html>)rawliteral";

String processor(const String& var) {
  if(var == "BUTTONPLACEHOLDER") {
    String buttons = "";

    buttons += "<h4>Yellow light - State <span id=\"outputState0\"></span></h4>";
    buttons += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this, 0)\" id=\"output0\"><span class=\"slider\"></span></label>";

    buttons += "<h4>White light - State <span id=\"outputState1\"></span></h4>";
    buttons += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this, 1)\" id=\"output1\"><span class=\"slider\"></span></label>";

    return buttons;
  }
  return String();
}


void setupWiFi() {
  Serial.printf("\r\nWifi: Connecting");

  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("\r\nWifi: CONNECTED!\r\nWiFi: IP-Address %s\r\n", WiFi.localIP().toString().c_str());
}

void setup() {
  Serial.begin(BAUD_RATE); 
  Serial.printf("\r\n\r\n");
  setupWiFi();

  for (int i = 0; i < 2; i++) { 
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String idMessage;
    if (request->hasParam("id")) {
        idMessage = request->getParam("id")->value();
        int id = idMessage.toInt();

        digitalWrite(relayPins[id], LOW); 
        delay(100); 
        digitalWrite(relayPins[id], HIGH); 

        // Web arayüzünde durumu değiştir
        displayedRelayStates[id] = !displayedRelayStates[id];

        String response = "{\"id\":" + String(id) + ",\"state\":" + String(displayedRelayStates[id]) + "}";
        request->send(200, "application/json", response);
    }
  });

  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String response = "[";
    for (int i = 0; i < 2; i++) { 
        response += "{\"id\":" + String(i) + ",\"state\":" + String(displayedRelayStates[i]) + "}";
        if (i < 1) {
            response += ",";
        }
    }
    response += "]";
    request->send(200, "application/json", response);
  });

  server.begin();
}

void loop() {
}