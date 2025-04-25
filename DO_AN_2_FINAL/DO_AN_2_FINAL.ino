#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>



#include <HardwareSerial.h>

#define simSerial               Serial2
#define MCU_SIM_BAUDRATE        115200
#define MCU_SIM_TX_PIN              16
#define MCU_SIM_RX_PIN              17
#define MCU_SIM_EN_PIN              15

// Please update number before test
#define PHONE_NUMBER                "+84523715215"

Adafruit_MPU6050 mpu;

// Tạo webserver
WebServer server(80);

// Các khai báo hàm
float calculateTiltAngle(float ax, float ay, float az);
float calculateTotalAcceleration(float ax, float ay, float az);
float calculateSimilarity(float *window, int size);
void printMatrix(float *window, int rows, int cols);
String generateMatrixHTML(float *window, int rows, int cols);


// Chân kết nối MPU6050
#define SDA_PIN 21
#define SCL_PIN 22

// GPIO nút và LED
#define LED_PIN 2
// Ngưỡng giá trị
#define TILT_ANGLE_THRESHOLD 30 // Ngưỡng góc nghiêng để phát hiện thay đổi
#define ACCEL_THRESHOLD 14.0     // Ngưỡng gia tốc đột ngột
// #define SIMILARITY_THRESHOLD 20 // Ngưỡng độ tương đồng

// Thông tin WiFi
const char* ssid = "MANG DAY KTX 912";
const char* password = "83585852";
const int LEDPIN = 32; 
// HTML giao diện
const char main_page[] = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fall Detection Status</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #d3e3e9; }
        .container { background-color: #fff; padding: 20px; border-radius: 10px; text-align: center; max-width: 800px; margin: auto; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }
        .fall-detected { color: red; }
        .no-fall { color: green; }
        p { font-size: 18px; }
        table { width: 100%; margin: 20px 0; border-collapse: collapse; }
        td { border: 1px solid #000; padding: 5px; text-align: center; }
        .logo { 
            position: absolute; 
            top: 10px; 
            left: 10px; 
            max-width: 100px;
        }
        .center-table { 
            margin: auto;
            border: 2px solid #000;
            text-align: center;
        }
    </style>
    <script>
        setInterval(updateValues, 200);
        function updateValues() {
            location.reload(); 
        }
    </script>
</head>
<body>
    <img src="https://mybk.hcmut.edu.vn/tuyensinh/img/logo/Logo.png" alt="Logo" class="logo">
    <div class="container">
        <h1>Fall Detection System</h1>
        <p class="%s">%s</p>
        <p>Total Acceleration: %s</p>
        <p>Delta_angle: %s</p>
        %s <!-- Bảng ma trận sẽ được chèn tại đây -->
    </div>
</body>
</html>
)rawliteral";

// Biến toàn cục
int warning = 0;
float prev_angle = 0.0;
const int windowSize = 72; // Số lượng mẫu trong cửa sổ trượt
float angleWindow[windowSize] = {0}; // Lưu dữ liệu góc nghiêng
int currentIndex = 0; // Chỉ số hiện tại trong cửa sổ
float delta_angle =0.0;


void updateWebPage() {
    char statusClass[20];
    char statusText[50];

    if (warning == 1) {
        strcpy(statusClass, "fall-detected");
        strcpy(statusText, "Fall Detected!");
    } else {
        strcpy(statusClass, "no-fall");
        strcpy(statusText, "No Fall Detected.");
    }

    // Tính gia tốc tổng hợp và độ tương đồng
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);
    float total_accel = calculateTotalAcceleration(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
    // float similarity = calculateSimilarity(angleWindow, windowSize);
    float  similarity  = 0.0; 
    float start_angle = angleWindow[1];
    float middle_angle = angleWindow[30];


    // Xác định sự kiện té ngã
    float delta_angle = abs(start_angle - middle_angle);

    // Tạo bảng HTML từ ma trận
    String matrixHTML = generateMatrixHTML(angleWindow, 8, 9);

    // Tạo HTML với các giá trị TotalAcceleration và Similarity
    char htmlBuffer[4096];
    snprintf(htmlBuffer, sizeof(htmlBuffer), main_page, 
             statusClass, statusText, 
             String(total_accel, 2).c_str(), String(delta_angle, 2).c_str(), 
             matrixHTML.c_str());

    server.send(200, "text/html", htmlBuffer);
}




void sim_at_wait()
{
    delay(100);
    while (simSerial.available()) {
        Serial.write(simSerial.read());
    }
}

bool sim_at_cmd(String cmd){
    simSerial.println(cmd);
    sim_at_wait();
    return true;
}

bool sim_at_send(char c){
    simSerial.write(c);
    return true;
}

void sent_sms()
{
    sim_at_cmd("AT+CMGF=1");
    String temp = "AT+CMGS=\"";
    temp += (String)PHONE_NUMBER;
    temp += "\"";
    sim_at_cmd(temp);
    sim_at_cmd("CO NGUOI BI NGA, VUI LONG KIEM TRA");

    // End charactor for SMS
    sim_at_send(0x1A);
}

void call()
{
    String temp = "ATD";
    temp += PHONE_NUMBER;
    temp += ";";
    sim_at_cmd(temp); 

    delay(20000);

    // Hang up
    sim_at_cmd("ATH"); 
}





void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (!mpu.begin()) {
        Serial.println("Không tìm thấy cảm biến MPU6050!");
        while (1);
    }

    // This statement will declare pin 22 as digital output 

      pinMode(LEDPIN, OUTPUT);

    Serial.println("MPU6050 đã được khởi động!");
    delay(1000);

    // Kết nối WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Endpoint webserver
    server.on("/", updateWebPage);

    // Bắt đầu server
    server.begin();
    Serial.println("Web server started");

    /*  Power enable  */
    pinMode(MCU_SIM_EN_PIN,OUTPUT); 
    digitalWrite(MCU_SIM_EN_PIN,LOW);

    delay(20);
    Serial.begin(115200);
    Serial.println("\n\n\n\n-----------------------\nSystem started!!!!");

    // Delay 8s for power on
    delay(8000);
    simSerial.begin(MCU_SIM_BAUDRATE, SERIAL_8N1, MCU_SIM_RX_PIN, MCU_SIM_TX_PIN);

    // Check AT Command
    sim_at_cmd("AT");

    // Product infor
    sim_at_cmd("ATI");

    // Check SIM Slot
    sim_at_cmd("AT+CPIN?");

    // Check Signal Quality
    sim_at_cmd("AT+CSQ");

    sim_at_cmd("AT+CIMI");

    pinMode(2,OUTPUT); 
    digitalWrite(2,HIGH);

    sent_sms();

    //Delay 5s
    delay(5000);   

    call();

    delay(100);


}

void loop() {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    // Tính góc nghiêng
    float angle = calculateTiltAngle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);

    // Cập nhật cửa sổ trượt
    angleWindow[currentIndex] = angle;
    currentIndex = (currentIndex + 1) % windowSize;

    float start_angle = angleWindow[1];
    float middle_angle = angleWindow[30];


    // Xác định sự kiện té ngã
    float delta_angle = abs(start_angle - middle_angle);
   
    Serial.print("delta_angle: ");
    Serial.println(delta_angle); // In giá trị delta_angle
    float total_accel = calculateTotalAcceleration(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
    if (warning == 0 && delta_angle > TILT_ANGLE_THRESHOLD && total_accel  > ACCEL_THRESHOLD) {
        warning = 1;

        if (Serial.available()){
        char c = Serial.read();
        simSerial.write(c);
    }
    delay(10);
    call();
    // digitalWrite(LEDPIN, HIGH); 
    sent_sms();
    delay(10);
    sim_at_wait();
    

    } 
    // else {
    //     warning = 0;
    // }

    server.handleClient();
    delay(100);
}

float calculateTiltAngle(float ax, float ay, float az) {
    return atan2(sqrt(ax * ax + ay * ay), az) * 180.0 / PI;
}

float calculateTotalAcceleration(float ax, float ay, float az) {
    return sqrt(ax * ax + ay * ay + az * az);
}

// float calculateSimilarity(float *window, int size) {
//     float similarity = 0.0;
//     for (int i = 1; i < size; i++) {
//         float diff = abs(window[i] - window[i - 1]);
//         if (diff <= 20.0) {
//             similarity += 1.0;
//         } else if (diff > 20.0 && diff <= 30.0) {
//             similarity += 0.5;
//         }
//     }
//     return similarity;
// }

String generateMatrixHTML(float *window, int rows, int cols) {
    String html = "<table class='center-table'>";
    for (int i = 0; i < rows; i++) {
        html += "<tr>";
        for (int j = 0; j < cols; j++) {
            int index = i * cols + j;
            html += "<td>" + String(window[index], 2) + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    return html;
}
