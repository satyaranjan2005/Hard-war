#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Create an MPU6050 object
MPU6050 mpu;

// DHT11 setup
#define DHTPIN D3  // DHT11 data pin connected to GPIO0
#define DHTTYPE DHT11  // Define the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

StaticJsonDocument<200> doc;
String jsonString;

void setup() {
  // Start serial communication at 115200 baud rate
  Serial.begin(115200);

  // Initialize I2C communication with specified SDA and SCL pins
  Wire.begin(D2, D1);  // SDA, SCL for MPU6050
  
  // Initialize the MPU6050 sensor
  mpu.initialize();

  // Check if the MPU6050 is connected properly
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful.");
  } else {
    Serial.println("MPU6050 connection failed!");
    while (1);  // Stay in a loop if the connection fails
  }

  // Initialize DHT11 sensor
  dht.begin();
}

void loop() {
  // Variables to hold accelerometer and gyroscope data
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  // Read the raw data from the MPU6050 sensor
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw data to human-readable values
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  float gyroX = gx / 131.0;
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;

  // Print accelerometer and gyroscope data to Serial Monitor
  Serial.print("Accel (g): X = ");
  Serial.print(accelX);
  Serial.print(", Y = ");
  Serial.print(accelY);
  Serial.print(", Z = ");
  Serial.println(accelZ);

  Serial.print("Gyro (deg/s): X = ");
  Serial.print(gyroX);
  Serial.print(", Y = ");
  Serial.print(gyroY);
  Serial.print(", Z = ");
  Serial.println(gyroZ);

  // Read temperature and humidity from DHT11 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again)
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Print DHT11 data to Serial Monitor
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");
  }

  delay(2000);  // Delay for 2 seconds before next read

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["device"] = "ESP8266";
  doc["status"] = "OK";

  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}