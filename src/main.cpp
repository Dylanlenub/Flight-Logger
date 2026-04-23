#include <Arduino.h>          // Enables use of Arduino-specific functions
#include <Wire.h>             // I2C Library
#include <SPI.h>              // Required by SD library
#include <SD.h>               // Gives read/write capabilites for SD card
#include <Adafruit_Sensor.h>  // Main sensor library
#include <Adafruit_BMP3XX.h>  // Driver for Barometer
//#include <Adafruit_BMP085.h>
//#include <Adafruit_MPU6050.h> // Driver for Accelerometer
#include <Adafruit_BNO08x.h> //Driver for IMU
#include <WiFi.h> //Wifi library
#include <WiFiAP.h> //Access point wifi library
#include "webpage.h"

// http://192.168.4.1/

const char *ssid = "ESP32 WiFi";
const char *password = "12345678";
#include <Adafruit_BNO08x.h>  // Driver for IMU

const char* LOG_FILE_NAME = "/flight-log.txt";
const int SAMPLE_RATE = 100;  // How often data is logged (in milliseconds)

// Global sensor values (read in loop, served on request)
float altitude    = 0, temperature = 0, pressure = 0;
float accelX = 0, accelY = 0, accelZ = 0;
float pitch  = 0, roll   = 0, yaw    = 0;
float altitudeBaseline = 0.0f; 


Adafruit_BMP3XX barometer;
Adafruit_BNO08x IMU;
sh2_SensorValue_t sensorValue;
WiFiServer server(80); //port for server

//Calibrate Roll Pitch Yaw
void calibrate() {
  float sum = 0;
  int   good = 0;
  for (int i = 0; i < 20; i++) {
    if (IMU.enableReport(SH2_ROTATION_VECTOR, 10000)) {
      sum += barometer.readAltitude(1013.25);
      good++;
    }
    delay(50);
  }
  if (good > 0) {
    altitudeBaseline = sum / good;
    Serial.print(F("[CAL] Baseline set to "));
    Serial.print(altitudeBaseline, 2);
    Serial.println(F(" m"));
  } else {
    Serial.println(F("[CAL] ERROR: No valid BMP388 readings during calibration!"));
  }
}

void handleClient(){
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();
  
  if (req.indexOf("GET /status") != -1) {
    String json = "{";
    json += "\"temp\":"      + String(temperature, 1) + ",";
    json += "\"pressure\":"  + String(pressure / 100.0, 1) + ","; // Pa -> hPa
    json += "\"altitude\":"  + String(altitude, 1)    + ",";
    json += "\"pitch\":"     + String(pitch, 1)        + ",";
    json += "\"roll\":"      + String(roll, 1)         + ",";
    json += "\"yaw\":"       + String(yaw, 1)          + ",";
    json += "\"accelX\":"    + String(accelX, 2)       + ",";
    json += "\"accelY\":"    + String(accelY, 2)       + ",";
    json += "\"accelZ\":"    + String(accelZ, 2);
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.print(json);
  } else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(htmlPage);
  }

  delay(1);
  client.stop();
}

// ─────────────────────────────────────────────
//  Quaternion → Euler (degrees)
//  Convention: ZYX (aerospace) — Yaw/Pitch/Roll
// ─────────────────────────────────────────────
void quaternionToEuler(float qr, float qi, float qj, float qk,
                       float &outRoll, float &outPitch, float &outYaw) {
  // Rotation vector component layout from BNO08X:
  //   real=qw, i=qx, j=qy, k=qz
  float qw = qr, qx = qi, qy = qj, qz = qk;

  // Roll (X-axis rotation)
  float sinr_cosp = 2.0f * (qw * qx + qy * qz);
  float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
  outRoll = atan2f(sinr_cosp, cosr_cosp) * RAD_TO_DEG;

  // Pitch (Y-axis rotation) — clamped to avoid gimbal singularity
  float sinp = 2.0f * (qw * qy - qz * qx);
  if (fabsf(sinp) >= 1.0f)
    outPitch = copysignf(90.0f, sinp);
  else
    outPitch = asinf(sinp) * RAD_TO_DEG;

  // Yaw (Z-axis rotation) — 0..360
  float siny_cosp = 2.0f * (qw * qz + qx * qy);
  float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
  outYaw = atan2f(siny_cosp, cosy_cosp) * RAD_TO_DEG;
  if (outYaw < 0) outYaw += 360.0f;
}

void setup() {
  Serial.begin(115200); // Initialize ESP32
  Wire.begin();         // Initialize I2C
  SD.begin();
  barometer.begin_I2C();
  IMU.begin_I2C();

  // Barometer configuration
  barometer.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  barometer.setPressureOversampling(BMP3_OVERSAMPLING_16X);
  barometer.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_7);
  barometer.setOutputDataRate(BMP3_ODR_50_HZ);

  // IMU configuration
  IMU.enableReport(SH2_LINEAR_ACCELERATION, 10000);
  IMU.enableReport(SH2_ROTATION_VECTOR, 10000);

  // Log file creation
  File logFile = SD.open(LOG_FILE_NAME, "w");
  logFile.println("Time | Altitude | Temperature | Pressure | X-Acceleration | Y-Acceleration | Z-Acceleration | Pitch | Roll | Yaw");
  logFile.close();

  //Initialize Wifi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();

  calibrate();
}

void loop() {
  // Get the barometer's data readings
  altitude = barometer.readAltitude(1013.25) - altitudeBaseline; // Standard sea level pressure in hPa
  temperature = barometer.temperature;
  pressure = barometer.pressure;

  // Local variables for IMU data
  //float accelX = 0, accelY = 0, accelZ = 0;
  //float gyroX = 0, gyroY = 0, gyroZ = 0;
  
  //Get IMU readings
/*  while(IMU.getSensorEvent(&sensorValue)) {
    switch (sensorValue.sensorId) {

      case SH2_ACCELEROMETER:
        accelX = sensorValue.un.accelerometer.x;
        accelY = sensorValue.un.accelerometer.y;
        accelZ = sensorValue.un.accelerometer.z;
        break;

      case SH2_GYROSCOPE_CALIBRATED:
        gyroX = sensorValue.un.gyroscope.x;
        gyroY = sensorValue.un.gyroscope.y;
        gyroZ = sensorValue.un.gyroscope.z;
        break;
    }
  }
*/

  while (IMU.getSensorEvent(&sensorValue)){
    if (sensorValue.sensorId == SH2_ROTATION_VECTOR){
      quaternionToEuler(
        sensorValue.un.rotationVector.real,
        sensorValue.un.rotationVector.i,
        sensorValue.un.rotationVector.j,
        sensorValue.un.rotationVector.k,
        roll, pitch, yaw
      );
    } else if (sensorValue.sensorId == SH2_LINEAR_ACCELERATION) {
      accelX = sensorValue.un.linearAcceleration.x;
      accelY = sensorValue.un.linearAcceleration.y;
      accelZ = sensorValue.un.linearAcceleration.z;
    }
  }

  // Write the data into the flight log
  File logFile = SD.open(LOG_FILE_NAME, "a");
  
  logFile.printf(
    "%lu | %.2f | %.2f | %.2f | %.2f | %.2f | %.2f\n",
    millis(),
    altitude,
    temperature,
    pressure,
    accelX,
    accelY,
    accelZ,
    roll,
    pitch,
    yaw
  );

  logFile.close();

  //handle web client
  handleClient();

  // Wait before running the loop again
  delay(SAMPLE_RATE);
}
