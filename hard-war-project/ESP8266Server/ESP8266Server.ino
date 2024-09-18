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

// creating a server
ESP8266WebServer server(80);

String webpage = "<!DOCTYPE html><html lang='en'> <head> <meta charset='UTF-8' /> <meta name='viewport' content='width=device-width, initial-scale=1.0' /> <title>Student Feedback</title> <style> body { font-family: Arial, sans-serif; text-align: center; background-color: #f9f9f9; } h1 { font-size: 2em; margin-bottom: 20px; } .feedback-container { display: flex; justify-content: center; gap: 20px; } .feedback-card { border: 2px solid #8bc5f0; border-radius: 10px; padding: 20px; width: 150px; height: 200px; display: flex; flex-direction: column; justify-content: center; align-items: center; box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1); } .feedback-card h2 { font-size: 1.2em; margin-bottom: 10px; } .feedback-card p { font-size: 1.2em; color: green; } .feedback-card .status { font-weight: bold; } .facial-exp { background-color: #eaf3ff; } .movement { background-color: #eaf3ff; } .temperature { background-color: #eaf3ff; } </style> </head> <body> <h1>Student Feedback</h1> <div class='feedback-container'> <div class='feedback-card facial-exp'> <h2>Facial exp</h2> <p class='status'>Active</p> </div> <div class='feedback-card movement'> <h2>Movement</h2> <p class='status'>Normal</p> </div> <div class='feedback-card temperature'> <h2>Temperature</h2> <p class='status' id='temp'>Normal</p> </div> </div> <script> console.log('hello'); let Socket; function init() { console.log('init'); Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function (event) { processCommand(event); }; } function processCommand(event) { console.log('event'); let obj = JSON.parse(event.data); document.getElementById('temp').innerHTML = obj.temperature; console.log(event.data); } window.onload = function (event) { console.log('onload'); init(); }; </script> </body></html>";

// creating websocket server
WebSocketsServer webSocket = WebSocketsServer(81);

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

  // Initialize wifi for esp8266
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

  server.handleClient();
  webSocket.loop();

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
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.print(" %\t");
    // Serial.print("Temperature: ");
    // Serial.print(temperature);
    // Serial.println(" *C");
  }

  delay(2000);  // Delay for 2 seconds before next read

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["device"] = "ESP8266";
  doc["status"] = "OK";

  serializeJson(doc, jsonString);
  Serial.println(jsonString);

  String str = String(random(100));
  int str_len = str.length();
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  webSocket.broadcastTXT(jsonString);
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
