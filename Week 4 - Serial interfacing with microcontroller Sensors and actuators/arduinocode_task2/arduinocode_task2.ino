#include <Wire.h>
#include <MPU6050.h>
#include <Servo.h>

MPU6050 imu;
Servo servoMotor;

// === Pins ===
const int LED_GREEN = 4;
const int LED_RED   = 3;
const int SERVO_PIN = 9;

// === State Variables ===
bool cardVerified = false;
unsigned long motionStartTime = 0;
unsigned long motionTimeout = 5000; // 5 seconds max to complete motion

// === Motion Detection Settings ===
float xStart = 0, zStart = 0;
bool motionStarted = false;
int checkpoints = 0;

// === Setup ===
void setup() {
  Serial.begin(9600);
  Wire.begin();
  imu.initialize();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  servoMotor.attach(SERVO_PIN);

  // Default: both LEDs OFF, servo locked
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  servoMotor.write(90);

  if (!imu.testConnection()) {
    Serial.println("❌ MPU6050 not connected!");
    while (1);
  }

  Serial.println("System ready. Waiting for Python command...");
}

// === Loop ===
void loop() {
  // --- 1️⃣ Wait for data from Python ---
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'A') {
      cardVerified = true;
      Serial.println("✅ RFID verified. Start motion detection...");
    }
    else if (command == 'D') {
      cardVerified = false;
      Serial.println("❌ RFID denied. Red LED ON 2s...");
      digitalWrite(LED_RED, HIGH);
      delay(2000);
      digitalWrite(LED_RED, LOW);
    }
  }

  // --- 2️⃣ Motion detection if card is valid ---
  if (cardVerified) {
    if (detectCircularMotion()) {
      Serial.println("✅ Circular motion verified!");
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      servoMotor.write(180);  // unlock
      delay(1000);
      servoMotor.write(90);   // lock again
      delay(1000);
      digitalWrite(LED_GREEN, LOW);
      cardVerified = false;   // reset for next scan
    }
  }

  delay(100);
}

// === 3️⃣ Circular motion detection ===
bool detectCircularMotion() {
  int16_t ax, ay, az;
  imu.getAcceleration(&ax, &ay, &az);
  float x = (float)ax / 16384.0;
  float z = (float)az / 16384.0;

  if (!motionStarted && (abs(x) > 0.15 || abs(z) > 0.15)) {
    motionStarted = true;
    motionStartTime = millis();
    checkpoints = 0;
    xStart = x;
    zStart = z;
  }

  if (motionStarted) {
    // Detect approximate "circle" quadrants
    if ((x > 0.4 && abs(z) < 0.2) ||   // right
        (x < -0.4 && abs(z) < 0.2) ||  // left
        (z > 0.4 && abs(x) < 0.2) ||   // up
        (z < -0.4 && abs(x) < 0.2)) {  // down
      checkpoints++;
      Serial.print("Checkpoint "); Serial.println(checkpoints);
      delay(150);
    }

    // Completed 4 directions = circle done
    if (checkpoints >= 4) {
      motionStarted = false;
      checkpoints = 0;
      return true;
    }

    // Timeout if taking too long
    if (millis() - motionStartTime > motionTimeout) {
      motionStarted = false;
      checkpoints = 0;
      Serial.println("⏱ Motion timeout!");
      digitalWrite(LED_RED, HIGH);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      return false;
    }
  }

  return false;
}