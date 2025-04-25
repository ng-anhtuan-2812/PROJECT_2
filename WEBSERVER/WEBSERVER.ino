#include <WiFi.h>
#include <WebServer.h>

// Thông tin WiFi
const char* ssid = "Xom Tro";
const char* password = "bachkhoa123";

// Tạo webserver
WebServer server(80);

// Trang HTML chính
const char main_page[] = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fall Detection Status</title>
    <style>
        body {
            font-family: Arial, sans-serif;
             background-color: #d3e3e9;
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
            width: 800px;
        }
        h1 {
            font-size: 24px;
            color: #333;
            margin-bottom: 20px;
        }
        p {
            font-size: 18px;
            color: #555;
        }
        .fall-detected {
            color: red;
        }
        .no-fall {
            color: green;
        }
        .info {
            margin-top: 15px;
            font-size: 14px;
            color: #333;
        }
    </style>
 
</head>
<body>
  
   <div class="HEAD">
    <div style="position: absolute; top: 10px; left: 10px;">
        <img src=" https://mybk.hcmut.edu.vn/tuyensinh/img/logo/Logo.png" alt="Logo HCMUT" style="width: 150px;">
    </div>

    <div class="container">
        <h1>PROJECT: IDENTIFYING AND WARNING OF FALLS FOR THE ELDERLY AND DISABLED</h1>
        <h1>Fall Detection Status</h1>
        <p class="%s">%s</p>
        <div class="info">
            <p><strong>PHONE NUMBER ALERT:</strong> +84947458626</p>
            <p><strong>Acceleration X:</strong> <span id="accelX">0</span> m/s²</p>
            <p><strong>Acceleration Y:</strong> <span id="accelY">0</span> m/s²</p>
            <p><strong>Acceleration Z:</strong> <span id="accelZ">0</span> m/s²</p>
            <p><strong>Gyro Magnitude:</strong> <span id="gyroMagnitude">0</span></p>
            <p><strong>Tilt Angle:</strong> <span id="tiltAngle">0</span>°</p>
        </div>
    </div>
</body>
</html>
)rawliteral";



// Pin LED
const int ledPin = 2;

void setup() {
  // Thiết lập pin LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Kết nối WiFi
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

     // Hiển thị địa chỉ IP
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Thiết lập các đường dẫn web
  server.on("/", []() {
    server.send(200, "text/html", main_page);
  });



  // Bắt đầu server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Xử lý client
  server.handleClient();
}
