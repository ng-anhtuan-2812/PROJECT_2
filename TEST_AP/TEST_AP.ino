#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <Arduino.h>

#define EEPROM_SIZE 64
#define SSID_ADDR 0
#define PASSWORD_ADDR 32

const char* ap_ssid = "ESP32-AP";
const char* ap_password = "12345678"; // Ensure this password is at least 8 characters long

const int LEDPIN = 22; 
const int PushButton = 16;
const unsigned long holdTime = 3000; // Thời gian giữ nút nhấn (ms)
unsigned long buttonPressTime = 0; // Thời điểm nút nhấn được nhấn

WebServer server(80);

void saveWiFiCredentials(const char* ssid, const char* password) {
  EEPROM.begin(EEPROM_SIZE);

  // Save SSID
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  }

  // Save Password
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(PASSWORD_ADDR + i, password[i]);
  }

  EEPROM.commit();
  EEPROM.end();
}

void getWiFiCredentials(char* ssid, char* password) {
  EEPROM.begin(EEPROM_SIZE);

  // Read SSID
  for (int i = 0; i < 32; ++i) {
    ssid[i] = EEPROM.read(SSID_ADDR + i);
  }

  // Read Password
  for (int i = 0; i < 32; ++i) {
    password[i] = EEPROM.read(PASSWORD_ADDR + i);
  }

  EEPROM.end();
}

void clearWiFiCredentials() {
  EEPROM.begin(EEPROM_SIZE);

  // Clear SSID and Password
  for (int i = 0; i < 64; ++i) {
    EEPROM.write(i, 0);
  }

  EEPROM.commit();
  EEPROM.end();
}

void connectToWiFi() {
  char ssid[32] = {0};
  char password[32] = {0};

  getWiFiCredentials(ssid, password);

  if (strlen(ssid) > 0) {
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) { // 30 seconds timeout
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi Connected.");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      // Disable AP mode to save power
      WiFi.softAPdisconnect(true);
      Serial.println("AP mode disabled to save power.");
    } else {
      Serial.println("\nFailed to connect to WiFi.");
      WiFi.disconnect(); // Ensure WiFi is disconnected before starting SmartConfig
    }
  } else {
    Serial.println("No WiFi credentials found in EEPROM.");
  }
}

//Quét WIFI
void scanWifi(std::function<void(String, int32_t)> callback) {
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");

    // Tạo một mảng để lưu thông tin về các mạng
    struct Network {
      String ssid;
      int32_t rssi;
    };
    Network networks[n];

    // Lấy thông tin về các mạng
    for (int i = 0; i < n; i++) {
      networks[i].ssid = WiFi.SSID(i);
      networks[i].rssi = WiFi.RSSI(i);
    }

    // Sắp xếp các mạng theo RSSI giảm dần
    for (int i = 0; i < n - 1; i++) {
      for (int j = i + 1; j < n; j++) {
        if (networks[j].rssi > networks[i].rssi) {
          Network temp = networks[i];
          networks[i] = networks[j];
          networks[j] = temp;
        }
      }
    }

    // Gọi hàm callback cho 5 mạng mạnh nhất
    for (int i = 0; i < 5 && i < n; i++) {
      callback(networks[i].ssid, networks[i].rssi);
    }
  }
  WiFi.scanDelete();	
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Manager</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #87CEEB; /* Màu xanh da trời */
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }

        .container {
            background-color: #fff;
            padding: 20px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            border-radius: 10px;
            text-align: center;
            width: 300px;
        }

        h1 {
            font-size: 24px;
            color: #333;
            margin-bottom: 20px;
        }

        label {
            display: block;
            font-size: 16px;
            margin-top: 10px;
            color: #555;
            text-align: left;
        }

        input[type="text"],
        input[type="password"] {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            margin-bottom: 15px;
            border: 1px solid #ccc;
            border-radius: 5px;
            box-sizing: border-box;
        }

        input[type="submit"] {
            width: 100%;
            padding: 10px;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            margin-top: 10px;
            box-sizing: border-box;
        }

        input[type="submit"]:hover {
            background-color: #ddd;
        }

        #save {
            background-color: #4CAF50;
            color: white;
        }

        #scan {
            background-color: #2196F3;
            color: white;
        }

        #clear {
            background-color: #f44336;
            color: white;
        }

        form {
            margin-bottom: 15px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 WiFi Manager</h1>
        <form action="/connect" method="POST">
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid">
            <label for="password">Password:</label>
            <input type="password" id="password" name="password">
            <input type="submit" value="Save WiFi" id="save">
        </form>
        <form action="/scan" method="POST">
            <input type="submit" value="Scan WiFi" id="scan">
        </form>
        <form action="/clear" method="POST">
            <input type="submit" value="Clear WiFi Credentials" id="clear">
             
        </form>
    </div>
</body>
</html>


  )rawliteral";

  server.send(200, "text/html", html);
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  if (ssid.length() > 0 && password.length() >= 8) {
    saveWiFiCredentials(ssid.c_str(), password.c_str());
    server.send(200, "text/html", "<h2>WiFi Credentials Saved. Rebooting...</h2>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(200, "text/html", "<h2>Invalid SSID or Password. Please try again.</h2>");
  }
}
void handleScan() {
String html = "<!DOCTYPE HTML><html><body><h1>WiFi Scan Results</h1><table><tr><th>SSID</th><th>RSSI</th></tr>";
  
  // Quét WiFi và thêm kết quả vào mã HTML
  scanWifi([&](String ssid, int32_t rssi) {
    html += "<tr><td>" + ssid + "</td><td>" + String(rssi) + "</td></tr>";
  });

  html += "</table><br><a href='/'>Back</a></body></html>";

  // Gửi trang web về cho người dùng
  server.send(200, "text/html", html);
}

void handleClear() {
  clearWiFiCredentials();
  server.send(200, "text/html", "<h2>WiFi Credentials Cleared. Rebooting...</h2>");
  delay(2000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  pinMode(PushButton, INPUT);
  WiFi.mode(WIFI_AP_STA);

  // Start AP mode for configuration if needed
  bool apStarted = WiFi.softAP(ap_ssid, ap_password);
  if (apStarted) {
    Serial.println("AP Started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP!");
  }

  // Try to connect to WiFi
  connectToWiFi();

  // Start Web Server
  server.on("/", handleRoot);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/clear", HTTP_POST, handleClear);
  server.on("/scan", HTTP_POST, handleScan);
  server.on("/connect_to_wifi", HTTP_GET, []() {
  String ssid = server.arg("ssid");

  String html = "<!DOCTYPE HTML><html><body><h1>Connect to WiFi</h1><form action='/connect' method='POST'><label>SSID:</label><br><input type='text' name='ssid' value='" + ssid + "' readonly><br><label>Password:</label><br><input type='password' name='password'><br><br><input type='submit' value='Connect'></form><br><a href='/scan'>Back</a></body></html>";
  
  // Gửi trang web về cho người dùng
  server.send(200, "text/html", html);
});
  server.begin();
  Serial.println("Web Server Started");

  // If connected to WiFi, print the IP address
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to WiFi. IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Not connected to WiFi. Connect to ESP32-AP to configure.");
  }

}

void loop() {
  server.handleClient();

  // Re-enable AP mode if WiFi connection is lost
  if (WiFi.status() != WL_CONNECTED && !WiFi.softAPgetStationNum()) {
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("WiFi connection lost. AP mode re-enabled.");
  }

  int Push_button_state = digitalRead(PushButton);


    if ( Push_button_state == HIGH )

{ 
  if (buttonPressTime == 0) {
      buttonPressTime = millis(); // Lưu lại thời điểm nút nhấn được nhấn xuống
    }

    if (millis() - buttonPressTime >= holdTime) {
     
      digitalWrite(LEDPIN, HIGH); 
      handleClear();
    }

}

else 

{
 buttonPressTime = 0;
digitalWrite(LEDPIN, LOW); 

}



}







