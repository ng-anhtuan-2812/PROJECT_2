

// COMBINE MPU6050 vs module SIM AC6700 -- esp32


#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <HardwareSerial.h>

// nối chân vào MPU6050 - ESP32
//  SDA - 21
//  SCL - 22
// VCC - 5V
// GND - GND 

// nối chân vào module SIM  CAP NGUON 3V3
#define simSerial               Serial2
#define MCU_SIM_BAUDRATE        115200
#define MCU_SIM_TX_PIN              17
#define MCU_SIM_RX_PIN              16
#define MCU_SIM_EN_PIN              15



#define PHONE_NUMBER                "+84947458626"


Adafruit_MPU6050 mpu;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) 
    delay(100); // Pauses for Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(100);
    }
  }
  Serial.println("MPU6050 Found!");

  // Setup motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true); // Keep it latched. Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  Serial.println("");
  delay(100);
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
    sim_at_cmd(" CO NGUOI TE NGA, VUI LONG KIEM TRA");

    // End charactor for SMS
    sim_at_send(0x1A);
}

void call(){
    String temp = "ATD";
    temp += PHONE_NUMBER;
    temp += ";";
    sim_at_cmd(temp); 

    delay(20000);

    // Hang up
    sim_at_cmd("ATH"); 
}


void moduleSim_setup(){
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

   // sent_sms();

    // Delay 5s
    delay(5000);   

    call();
}

void loop_moduleSim(){
 if (Serial.available()){
        char c = Serial.read();
        simSerial.write(c);
    }
    sim_at_wait();

}

void loop() {
	
  if (mpu.getMotionInterruptStatus()) {
    // Get new sensor events with the readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Print out the values
    Serial.print("AccelX:");
    Serial.print(a.acceleration.x);
    Serial.print(",");
    Serial.print("AccelY:");
    Serial.print(a.acceleration.y);
    Serial.print(",");
    Serial.print("AccelZ:");
    Serial.print(a.acceleration.z);
    Serial.print(", ");
    Serial.print("GyroX:");
    Serial.print(g.gyro.x);
    Serial.print(",");
    Serial.print("GyroY:");
    Serial.print(g.gyro.y);
    Serial.print(",");
    Serial.print("GyroZ:");
    Serial.print(g.gyro.z);
    Serial.println("");

    // Calculate total acceleration
    float totalAcceleration = sqrt(a.acceleration.x * a.acceleration.x + 
                                   a.acceleration.y * a.acceleration.y + 
                                   a.acceleration.z * a.acceleration.z);

    // Check for fall detection
    if (totalAcceleration > 15.0 || totalAcceleration < 0.5) {
      Serial.println("Phát hiện té ngã!");
      Serial.print("TOTAL ACCELERATION:\n");
      Serial.print(totalAcceleration);
	  moduleSim_setup();

    } else {
      Serial.println("Bình thường");
      Serial.print("TOTAL ACCELERATION:\n");
      Serial.print(totalAcceleration);
    }
  
  
  loop_moduleSim();
  
  
  }
  

  delay(100);
  delay(30000);
  
  ESP.restart();    // reset esp32 mỗi 30 giây 
  
}





