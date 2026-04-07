// LIBRARIES //
#include <Arduino.h>          // Enables use of Arduino-specific functions
#include <Wire.h>             // I2C Library
#include <SPI.h>              // Required by SD library
#include <SD.h>               // Gives read/write capabilites for SD card
#include <Adafruit_Sensor.h>  // Main sensor library
#include <Adafruit_BMP280.h>  // Driver for Barometer
//#include <Adafruit_BMP085.h>
#include <Adafruit_MPU6050.h> // Driver for Accelerometer

const char* LOG_FILE_NAME = "/flight-log.txt";
//const int SD_PIN = 5;
const int SAMPLE_RATE = 100;  // How often data is logged (in milliseconds)

Adafruit_BMP280 barometer;
Adafruit_MPU6050 accelerometer;

void setup() {
  Serial.begin(115200); // Initialize ESP32
  Wire.begin();         // Initialize I2C
  SD.begin();
  barometer.begin();
  accelerometer.begin();

  // Barometer configuration
  barometer.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  // Accelerometer configuration
  accelerometer.setAccelerometerRange(MPU6050_RANGE_8_G);
  accelerometer.setGyroRange(MPU6050_RANGE_500_DEG);
  accelerometer.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Log file creation
  File logFile = SD.open(LOG_FILE_NAME, "w");
  logFile.println("Time | Altitude | Temperature | Pressure | X-Acceleration | Y-Acceleration | Z-Acceleration | Pitch | Roll | Yaw");
  logFile.close();
}

void loop() {
  // Get the barometer's data readings
  float altitude = barometer.readAltitude();
  float temperature = barometer.readTemperature();
  float pressure = barometer.readPressure();

  // Get the accelerometer's data readings
  sensors_event_t accelEvent;
  sensors_event_t gyroEvent;
  sensors_event_t tempEvent;
  accelerometer.getEvent(&accelEvent, &gyroEvent, &tempEvent);

  float accelX = accelEvent.acceleration.x;
  float accelY = accelEvent.acceleration.y;
  float accelZ = accelEvent.acceleration.z;

  float gyroX = gyroEvent.gyro.x;
  float gyroY = gyroEvent.gyro.y;
  float gyroZ = gyroEvent.gyro.z;

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
    gyroX,
    gyroY,
    gyroZ
  );

  logFile.close();

  // Wait before running the loop again
  delay(SAMPLE_RATE);
}
