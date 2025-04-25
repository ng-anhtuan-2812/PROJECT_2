#include <Wire.h>
#include <Adafruit_MPU6050.h>

Adafruit_MPU6050 mpu;

#include <stdio.h>
#include <math.h>


// Chân kết nối MPU6050 - ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Chân kết nối module SIM
#define simSerial Serial2
#define MCU_SIM_BAUDRATE 115200
#define MCU_SIM_TX_PIN 17
#define MCU_SIM_RX_PIN 16
#define MCU_SIM_EN_PIN 15

// GPIO nút và LED
#define RESET_PIN 14
#define LED_PIN 2


// Cửa sổ trượt
#define WINDOW_SIZE 50 // Kích thước cửa sổ trượt (1 giây với tần số lấy mẫu 20ms)
float tiltAngles[WINDOW_SIZE] = {0};
float accelerations[WINDOW_SIZE] = {0};
int currentIndex = 0;

// Ngưỡng phát hiện té ngã
const float TILT_ANGLE_THRESHOLD = 60.0; // Thay đổi góc nghiêng
const float AM_MAX_THRESHOLD = 19.0;    // Đỉnh gia tốc cao
const float AM_MIN_THRESHOLD = 6.0;     // Đỉnh gia tốc thấp

void initializeMPU6050() {
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 initialized. Starting fall detection...");
}

void readMPU6050Data(float &AM, float &tiltAngle) {
    Serial.println("Reading data from MPU6050...");
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;

    // Tính gia tốc tổng hợp
    AM = sqrt(ax * ax + ay * ay + az * az); // Đơn vị m/s^2

    // Tính góc nghiêng
    tiltAngle = atan2(sqrt(ax * ax + ay * ay), az) * 180.0 / PI;
}

void updateSlidingWindow(float AM, float tiltAngle) {
    accelerations[currentIndex] = AM;
    tiltAngles[currentIndex] = tiltAngle;

    currentIndex = (currentIndex + 1) % WINDOW_SIZE;
}

bool detectFall() {
    float maxAM = 0, minAM = 1000;
    float angleChange = abs(tiltAngles[(currentIndex + 1) % WINDOW_SIZE] - tiltAngles[currentIndex]);

    for (int i = 0; i < WINDOW_SIZE; i++) {
        if (accelerations[i] > maxAM) maxAM = accelerations[i];
        if (accelerations[i] < minAM) minAM = accelerations[i];
    }

    return (angleChange > TILT_ANGLE_THRESHOLD && maxAM > AM_MAX_THRESHOLD && minAM < AM_MIN_THRESHOLD);
}

void setup() {
    Serial.begin(115200);
    initializeMPU6050();
}

void loop() {
    float AM, tiltAngle;

    // Đọc dữ liệu từ MPU6050
    readMPU6050Data(AM, tiltAngle);

    // Hiển thị dữ liệu đọc được
    Serial.printf("Current AM: %.2f, Tilt Angle: %.2f\n", AM, tiltAngle);

    // Cập nhật cửa sổ trượt
    updateSlidingWindow(AM, tiltAngle);

    // Kiểm tra phát hiện té ngã
    if (detectFall()) {
        Serial.println("Fall detected!");
    }

    delay(20); // Lấy mẫu mỗi 20ms
}
