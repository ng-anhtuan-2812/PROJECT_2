
// code test 27122024
#include <WiFi.h>
#include <WebServer.h>

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// Tạo webserver
WebServer server(80);


// Các khai báo hàm
float calculateTiltAngle(float ax, float ay, float az);
float calculateTotalAcceleration(float ax, float ay, float az);
float calculateSimilarity(float *window, int size);
void printMatrix(float *window, int rows, int cols);


// Chân kết nối MPU6050
#define SDA_PIN 21
#define SCL_PIN 22

// GPIO nút và LED
#define LED_PIN 2
// Ngưỡng giá trị
#define TILT_ANGLE_THRESHOLD 45.0 // Ngưỡng góc nghiêng để phát hiện thay đổi
#define ACCEL_THRESHOLD 16.0     // Ngưỡng gia tốc đột ngột
#define SIMILARITY_THRESHOLD 20.0 // Ngưỡng độ tương đồng

// Thông tin WiFi
const char* ssid = "Xom Tro";
const char* password = "bachkhoa123";




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
        .logo { 
            position: absolute; 
            top: 10px; 
            left: 10px; 
            max-width: 100px;

    </style>

  <script>
        setInterval(updateValues, 1000);

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


    </div>

</body>
</html>
)rawliteral";

int warning =0;

void updateWebPage() {
    char statusClass[20];
    char statusText[50];

    if ( warning ==1 ) {
        strcpy(statusClass, "fall-detected");
        strcpy(statusText, "Fall Detected!");
    } else {
        strcpy(statusClass, "no-fall");
        strcpy(statusText, "No Fall Detected.");
    }

 char htmlBuffer[2048];
    snprintf(htmlBuffer, sizeof(htmlBuffer), main_page, 
             statusClass, statusText 
             );

    server.send(200, "text/html", htmlBuffer);
}

// Biến lưu trữ

float prev_angle = 0.0;
const int windowSize = 10; // Số lượng mẫu trong cửa sổ trượt

// Mảng lưu trữ cửa sổ trượt
float angleWindow[windowSize] = {0}; // Lưu dữ liệu góc nghiêng
int currentIndex = 0; // Chỉ số hiện tại trong cửa sổ

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!mpu.begin()) {
    Serial.println("Không tìm thấy cảm biến MPU6050!");
    while (1);
  }

  Serial.println("MPU6050 đã được khởi động!");
  delay(1000);

  Serial.begin(115200);

    // Kết nối WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Thiết lập MPU6050
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1);
    }
    Serial.println("MPU6050 Found!");

    // Endpoint webserver
    server.on("/", updateWebPage);

    // Bắt đầu server
    server.begin();
    Serial.println("Web server started");



}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // 1. Tính góc nghiêng (Tilt Angle)
  float angle = calculateTiltAngle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
      server.handleClient();
  // Cập nhật cửa sổ trượt
  angleWindow[currentIndex] = angle;
  currentIndex = (currentIndex + 1) % windowSize;

  // 2. In dữ liệu cửa sổ dưới dạng ma trận 8x9
  printMatrix(angleWindow, 2, 5);

  // 3. Tính toán các thay đổi góc nghiêng
  float delta_angle = abs(angle - prev_angle);
  if (delta_angle > TILT_ANGLE_THRESHOLD) {
    Serial.println("Thay đổi góc nghiêng đáng kể!");
  }
  prev_angle = angle;

  // 4. Tính gia tốc tổng hợp
  float total_accel = calculateTotalAcceleration(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  if (total_accel > ACCEL_THRESHOLD) {
    Serial.println("Gia tốc đột ngột được phát hiện!");
  }

  // 5. Tính độ tương đồng (Pattern Matching)
  if (currentIndex == 0) { // Khi cửa sổ đầy đủ dữ liệu
    float similarity = calculateSimilarity(angleWindow, windowSize);
    Serial.print("Độ tương đồng: ");
    Serial.println(similarity);

    if (similarity >= SIMILARITY_THRESHOLD) {
      Serial.println("Phát hiện mẫu tương đồng!");
    }
  }

  // 6. Xác định sự kiện té ngã
  if (delta_angle > TILT_ANGLE_THRESHOLD && total_accel > ACCEL_THRESHOLD) {
    Serial.println("Cảnh báo: Phát hiện té ngã!");
    warning = 1;
  }

  delay(10); // Thời gian lặp lại
}

// Hàm tính góc nghiêng (Tilt Angle)
float calculateTiltAngle(float ax, float ay, float az) {
  return atan2(sqrt(ax * ax + ay * ay), az) * 180.0 / PI;
}

// Hàm tính gia tốc tổng hợp
float calculateTotalAcceleration(float ax, float ay, float az) {
  return sqrt(ax * ax + ay * ay + az * az);
}

// Hàm tính độ tương đồng giữa các giá trị trong cửa sổ
float calculateSimilarity(float *window, int size) {
  float similarity = 0.0;

  for (int i = 1; i < size; i++) {
    float diff = abs(window[i] - window[i - 1]);
    if (diff <= 20.0) {
      similarity += 1.0;
    } else if (diff > 20.0 && diff <= 30.0) {
      similarity += 0.5;
    }
    // Nếu diff > 30.0 thì similarity không tăng (giữ nguyên giá trị 0)
  }

  return similarity;
}

// Hàm in dữ liệu cửa sổ dưới dạng ma trận 8x9
void printMatrix(float *window, int rows, int cols) {
  Serial.println("Cửa sổ dữ liệu (dạng ma trận):");
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      int index = i * cols + j;
      Serial.print(window[index], 2); // In dữ liệu với 2 chữ số thập phân
      if (j < cols - 1) {
        Serial.print("\t"); // Dùng tab để ngăn cách các cột
      }
    }
    Serial.println(); // Xuống dòng sau mỗi hàng
  }
  Serial.println();
}
