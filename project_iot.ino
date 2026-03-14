  // Import required libraries
  #ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #endif
  #include <ESPAsyncWebServer.h>

  // Replace with your network credentials
  const char* ssid = "yourssid";
  const char* password = "yourname";

  const char* http_username = "housemate";
  const char* http_password = "housematepass";

  const char* PARAM_INPUT_1 = "state";

  // Set to true to define Relay as Normally Open (NO)
  bool relayNO = true;

  const int relayPin = 5;    // GPIO2 pin connected to the relay
  const int buzzerPin = 18;  // GPIO pin connected to the buzzer

  // Create AsyncWebServer object on port 80
  AsyncWebServer server(80);

  const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <title>Home Sweet Home!</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {font-family: Arial; display: inline-block; text-align: center;}
      h2 {padding: 60px; text-align: center; background: #1abc9c; color: white; font-size: 30px;}
      body {max-width: 600px; margin:0px auto; padding-bottom: 10px; background-color: #FFF8F3; justify-content: center}
      .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
      .switch input {display: none}
      .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
      .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
      input:checked+.slider {background-color: #2196F3}
      input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    </style>
  </head>
  <body>
    <h2>Home Sweet Home Housemate!</h2>
    <button onclick="logoutButton()">Logout</button>
    <p>To open/close the door, please click the button below</p>
    <p>Output - UNLOCK: <span id="state">%STATE%</span></p>
    %BUTTONPLACEHOLDER%
  <script>function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();

    if(element.checked){ 
      xhr.open("GET", "/update?state=1", true); 
      document.getElementById("state").innerHTML = "ON";    
    } else { 
      xhr.open("GET", "/update?state=0", true); 
      document.getElementById("state").innerHTML = "OFF";      
    }
    xhr.send();
  }
  function logoutButton() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/logout", true);
    xhr.send();
    setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
  }
  </script>
  </body>
  </html>
  )rawliteral";

  const char logout_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <p>Logged out or <a href="/">return to homepage</a>.</p>
    <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
  </body>
  </html>
  )rawliteral";

  // Replaces placeholder with button section in your web page
  String processor(const String& var) {
    if (var == "BUTTONPLACEHOLDER") {
      String buttons = "";
      String outputStateValue = outputState();
      buttons += "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
      return buttons;
    }
    if (var == "STATE") {
      if (digitalRead(relayPin)) {
        return "ON";
      } else {
        return "OFF";
      }
    }
    return String();
  }

  String outputState() {
    if (digitalRead(relayPin)) {
      return "checked";
    } else {
      return "";
    }
  }

  void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);

    pinMode(relayPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    //keep it in locked mode
    digitalWrite(relayPin, LOW);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to WiFi");

    // Print ESP Local IP Address
    Serial.print("ESP32 Web Server's IP address: ");
    Serial.println(WiFi.localIP());

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (!request->authenticate(http_username, http_password)) {
        request->requestAuthentication();
        beepBuzzer(4);
      } else if(request->authenticate(http_username, http_password)){
        request->send_P(200, "text/html", index_html, processor);
        beepBuzzer(2);
      }
    });

    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(401);
    });

    server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send_P(200, "text/html", logout_html, processor);
    });

    // Send a GET request to <ESP_IP>/update?state=<inputMessage>
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (!request->authenticate(http_username, http_password)) {
        return request->requestAuthentication();
      }

      String inputMessage;
      String inputParam;
      // GET input1 value on <ESP_IP>/update?state=<inputMessage>
      if (request->hasParam(PARAM_INPUT_1)) {
        inputMessage = request->getParam(PARAM_INPUT_1)->value();
        inputParam = PARAM_INPUT_1;
        digitalWrite(relayPin, inputMessage.toInt());
        if (inputMessage.toInt() == 1) {
          digitalWrite(relayPin, HIGH);
          beepBuzzer(1);
        } else if (inputMessage.toInt() == 0) {
          digitalWrite(relayPin, LOW);
          beepBuzzer(1);
        }

        Serial.println(inputMessage);
      } else {
        inputMessage = "No message sent";
        inputParam = "none";
      }
      request->send(200, "text/plain", "OK");
    });

    // Start server
    server.begin();
  }

  void beepBuzzer(int times) {
    for (int i = 0; i < times; i++) {
      digitalWrite(buzzerPin, HIGH);
      delay(300);
      digitalWrite(buzzerPin, LOW);
      delay(300);
    }
  }

  void loop() {
    // Nothing to do here
  }