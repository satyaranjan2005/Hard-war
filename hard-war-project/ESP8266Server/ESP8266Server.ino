#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoWebsockets.h>

// set up wifi for esp8266
#define WIFI_SSID "Satyaranjan"
#define WIFI_PASSWORD "11223344"

// DHT11 setup
#define DHTPIN D3  // DHT11 data pin connected to GPIO0
#define DHTTYPE DHT11  // Define the type of DHT sensor

// creating a server
ESP8266WebServer server(80);

// creating websocket server
WebSocketsServer webSocket = WebSocketsServer(81);

// Create an MPU6050 object
MPU6050 mpu;

// DTH Sensor object
DHT dht(DHTPIN, DHTTYPE);

// JSON Object for sending sensor data in webpage.
StaticJsonDocument<200> doc;
String jsonString;

// HTML for webpage
String webpage = "<!DOCTYPE html><html lang='en'> <head> <meta charset='UTF-8' /> <meta name='viewport' content='width=device-width, initial-scale=1.0' /> <title>Student Feedback</title> <style> body { font-family: Arial, sans-serif; text-align: center; background-color: #f9f9f9; } h1 { font-size: 2em; margin-bottom: 20px; } .feedback-container { display: flex; justify-content: center; gap: 20px; } .feedback-card { border: 2px solid #8bc5f0; border-radius: 10px; padding: 20px; width: 150px; height: 200px; display: flex; flex-direction: column; justify-content: center; align-items: center; box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1); } .feedback-card h2 { font-size: 1.2em; margin-bottom: 10px; } .feedback-card p { font-size: 1.2em; color: green; } .feedback-card .status { font-weight: bold; } .facial-exp { background-color: #eaf3ff; } .movement { background-color: #eaf3ff; } .temperature { background-color: #eaf3ff; } </style> </head> <body> <h1>Student Feedback</h1> <div class='feedback-container'> <div class='feedback-card facial-exp'> <h2>Facial exp</h2> <p class='status'>Active</p> </div> <div class='feedback-card movement'> <h2>Movement</h2> <p class='status'>Normal</p> </div> <div class='feedback-card temperature'> <h2>Temperature</h2> <p class='status' id='temp'>Normal</p> </div> </div> <script> console.log('hello'); let Socket; function init() { console.log('init'); Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function (event) { processCommand(event); }; } function processCommand(event) { console.log('event'); let obj = JSON.parse(event.data); document.getElementById('temp').innerHTML = obj.temperature; console.log(event.data); } window.onload = function (event) { console.log('onload'); init(); }; </script> </body></html>";

// Threshold values for detecting sudden movement
const int accelerationThreshold = 2000;  // Adjust this based on sensitivity
const int gyroscopeThreshold = 500;  // Threshold for angular velocity

// Constants for storing normal temperature bounds
const float lowerTemperatureBound = 36.1;
const float upperTemperatureBound = 37.2;
float temperature;

// Variables to hold accelerometer and gyroscope data
int16_t ax, ay, az;
int16_t gx, gy, gz;

bool motion = false;
bool bodyTemperatureStatus;
bool dhtStatus;
bool mpuStatus;

void setup() {
  // Start serial communication at 115200 baud rate
  Serial.begin(115200);

  // Initialize wifi for esp8266
  initializeESP8266Wifi();

  // Initialize I2C communication with specified SDA and SCL pins
  Wire.begin(D2, D1);  // SDA, SCL for MPU6050
  
  // Initialize the MPU6050 sensor
  mpu.initialize();

  // Initialize DHT11 sensor
  dht.begin();
}

void loop() {

  server.handleClient();
  webSocket.loop();

  // Read the raw data from the MPU6050 sensor
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // if this returns true, send "Moving!" to html page and make label red!
  motion = isMotionDetected(ax, ay, az, gx, gy, gz);

  // Read temperature and humidity from DHT11 sensor
  temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again)
  dhtStatus = showDHT11Status();
  mpuStatus = showMPU6050Status();

  bodyTemperatureStatus = getBodyTemperatureStatus(temperature);

  delay(2000);  // Delay for 2 seconds before next read
  
  sendSensorDataToWebPage();
}

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void handleStatus() {
  String message = "ESP8266 Status: \n";
  message += "IP Address: ";
  message += WiFi.localIP().toString();
  message += "\nWi-Fi Signal Strength (RSSI): ";
  message += WiFi.RSSI();
  message += " dBm\n";
  server.send(200, "text/plain", message);
}

void sendSensorDataToWebPage() {
  doc["temperature"] = temperature;
  doc["bodyTemperatureStatus"] = bodyTemperatureStatus;
  doc["motion"] = motion;
  doc["mpuStatus"] = mpuStatus;
  doc["dhtStatus"] = dhtStatus;

  serializeJson(doc, jsonString);
  Serial.println(jsonString);

  webSocket.broadcastTXT(jsonString);
}

void initializeESP8266Wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

  // server data
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.on("/status", handleStatus);
  server.begin();
  webSocket.begin();
  Serial.println("HTTP server started");

  Serial.print("ESP8266 IP address: ");
  Serial.println(WiFi.localIP());
}

bool showMPU6050Status() {
 // Check if the MPU6050 is connected properly
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful.");
    return true;
    // Write code so that it updates the webpage with relevent movement status from isMotionDetected function
  } else {
    Serial.println("MPU6050 connection failed!");
    return false;
    // Write code so that it updates the webpage that "Failed to read MPU6050!"
    // while (1);  // Stay in a loop if the connection fails
  }
}

bool showDHT11Status() {
  // Check DHT11 Sensor sensor
  if (isnan(temperature)) {
    Serial.println("Failed to get data from dht11");
    return false;
    // Write code so that it updates the webpage that "Failed to read from DHT sensor!"
  } else {
    Serial.println("Successfully connected to DHT11"); 
    return true;
    // Write code so that it updates the webpage with sensor data"
  }
}

// Returns true if sudden motionn is detected.
bool isMotionDetected(int16_t xAcc, int16_t yAcc, int16_t zAcc, int16_t xGyro, int16_t yGyro, int16_t zGyro) {
  // Check for sudden movement based on accelerometer thresholds
  if (abs(xAcc) > accelerationThreshold || abs(yAcc) > accelerationThreshold || abs(zAcc) > accelerationThreshold) {
    Serial.println("Sudden movement detected (acceleration)");
    Serial.print("AX: "); Serial.print(ax);
    Serial.print("AY: "); Serial.print(ay);
    Serial.print("AZ: "); Serial.println(az);
    return true;
  }
  // Check for sudden movement based on gyroscope thresholds
  if (abs(xGyro) > gyroscopeThreshold || abs(yGyro) > gyroscopeThreshold || abs(zGyro) > gyroscopeThreshold) {
    Serial.println("Sudden movement detected (rotation)");
    Serial.print("GX: "); Serial.print(gx);
    Serial.print("GY: "); Serial.print(gy);
    Serial.print("GZ: "); Serial.println(gz);
    return false;
  }
}

// Returns stated of human body temperature based on the readings from the sensors
bool getBodyTemperatureStatus(float sensorTemperature) {
  // Normal human body temperature
  if(sensorTemperature <= lowerTemperatureBound && sensorTemperature >= upperTemperatureBound) {
     return false
  } else {
    return true
  }
}
