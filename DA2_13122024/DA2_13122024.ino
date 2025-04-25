#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

// Chân kết nối MPU6050 - ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Chân kết nối module SIM lên ESP32 
#define simSerial Serial2
#define MCU_SIM_BAUDRATE 115200
#define MCU_SIM_TX_PIN 17
#define MCU_SIM_RX_PIN 16
#define MCU_SIM_EN_PIN 15

// GPIO nút và LED
#define RESET_PIN 14
#define LED_PIN 2

// WiFi credentials
const char* ap_ssid = "Xom Tro";
const char* ap_password = "bachkhoa123";

String phoneNumber = "+84947458626"; // Số điện thoại cảnh báo mặc định
WebServer server(80);
Adafruit_MPU6050 mpu;

bool fallDetected = false;
float currentAccelX = 0;
float currentAccelY = 0;
float currentAccelZ = 0;

void setupMPU6050() {
    Serial.println("Initializing MPU6050...");

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(100);
        }
    }

    Serial.println("MPU6050 Found!");
    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true);
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);
}

void setupSIMModule() {
    pinMode(MCU_SIM_EN_PIN, OUTPUT);
    digitalWrite(MCU_SIM_EN_PIN, LOW);
    delay(20);

    simSerial.begin(MCU_SIM_BAUDRATE, SERIAL_8N1, MCU_SIM_RX_PIN, MCU_SIM_TX_PIN);

    sim_at_cmd("AT");
    sim_at_cmd("ATI");
    sim_at_cmd("AT+CPIN?");
    sim_at_cmd("AT+CSQ");
    sim_at_cmd("AT+CIMI");
}

void sim_at_wait() {
    delay(100);
    while (simSerial.available()) {
        Serial.write(simSerial.read());
    }
}

bool sim_at_cmd(String cmd) {
    simSerial.println(cmd);
    sim_at_wait();
    return true;
}

bool sim_at_send(char c) {
    simSerial.write(c);
    return true;
}

void sent_sms() {
    sim_at_cmd("AT+CMGF=1");
    String temp = "AT+CMGS=\"" + phoneNumber + "\"";
    sim_at_cmd(temp);
    sim_at_cmd("CO NGUOI TE NGA, VUI LONG KIEM TRA");
    sim_at_send(0x1A);
}

void call() {
    String temp = "ATD" + phoneNumber + ";";
    sim_at_cmd(temp);
    delay(20000);
    sim_at_cmd("ATH");
}

float calculateMagnitude(float x, float y, float z) {
    return sqrt(x * x + y * y + z * z);
}

float calculateTiltAngle(float ax, float ay, float az) {
    return atan2(sqrt(ax * ax + ay * ay), az) * 180 / PI;
}

void detectFall() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    currentAccelX = a.acceleration.x;
    currentAccelY = a.acceleration.y;
    currentAccelZ = a.acceleration.z;

    float totalAcceleration = calculateMagnitude(a.acceleration.x, a.acceleration.y, a.acceleration.z);
    float gyroMagnitude = calculateMagnitude(g.gyro.x, g.gyro.y, g.gyro.z);
    float tiltAngle = calculateTiltAngle(a.acceleration.x, a.acceleration.y, a.acceleration.z);

    if (totalAcceleration > 15.0 || totalAcceleration < 0.5) {
        Serial.println("Potential fall detected!");
        delay(500); // Chờ để đảm bảo trạng thái ổn định

        if (gyroMagnitude > 1.5 && (tiltAngle > 45 && tiltAngle < 135)) {
            Serial.println("Fall confirmed!");
            fallDetected = true;
            sent_sms();
        } else {
            Serial.println("False alarm.");
        }
    }
}

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fall Detection Settings</title>
</head>
<body>
    <h1>Fall Detection Configuration</h1>
    <form action="/set-phone" method="POST">
        <label for="phone">Phone Number:</label>
        <input type="text" id="phone" name="phone" value="%s">
        <button type="submit">Set Phone Number</button>
    </form>
    <p><strong>Current Phone Number:</strong> %s</p>
</body>
</html>
    )rawliteral";

    char buffer[html.length() + 100];
    sprintf(buffer, html.c_str(), phoneNumber.c_str(), phoneNumber.c_str());
    server.send(200, "text/html", buffer);
}

void handleSetPhoneNumber() {
    if (server.hasArg("phone")) {
        phoneNumber = server.arg("phone");
        Serial.println("Updated phone number: " + phoneNumber);
        server.send(200, "text/plain", "Phone number updated.");
    } else {
        server.send(400, "text/plain", "Phone number not provided.");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    setupMPU6050();
    setupSIMModule();

    WiFi.begin(ap_ssid, ap_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    server.on("/", handleRoot);
    server.on("/set-phone", HTTP_POST, handleSetPhoneNumber);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();
    detectFall();

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

    if (digitalRead(RESET_PIN) == LOW) {
        digitalWrite(LED_PIN, LOW); // Tắt LED khi reset
        delay(100);
        ESP.restart();
    }
}
