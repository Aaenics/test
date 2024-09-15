#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP8266.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// Firebase credentials
const char* firebaseHost = "https://all-in-agri-default-rtdb.firebaseio.com/";
const char* firebaseAuth = "AIzaSyCskTPMKHMATcjJ5OXh4j_kLa-pNrxoaBo";
const char* firebaseWindPath = "/wind";
const char* firebasePressurePath = "/pressure";
const char* firebaseTempPath = "/temp";
const char* firebaseHumidityPath = "/humidity";

// OpenWeatherMap API credentials
const char* apiKey = "5505e4995e00f1755c61da65f6e74231";
const char* cityID = "2294768";

// Define the pins
#define DHTPIN D4  // DHT11 pin
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Initialize LCD (20x4 display)
FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  
  // Initialize WiFiManager
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("WeatherMonitoringAP")) {
    Serial.println("Failed to connect and hit timeout");
    
    // Display "Not Connected" on the LCD
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Not Connected");
    
    // Optionally, you can keep the device running without WiFi
    return;  // Return here instead of restarting the device
  }

  Serial.println("Connected to WiFi");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected to WiFi");

  // Initialize Firebase
  Firebase.begin(firebaseHost, firebaseAuth);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  float pressure = 0.0;
  float windSpeed = 0.0;

  // Declare WiFiClient
  WiFiClient client;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://api.openweathermap.org/data/2.5/weather?id=") + cityID + "&appid=" + apiKey + "&units=metric";
    
    http.begin(client, url);
    
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      pressure = doc["main"]["pressure"].as<float>();
      windSpeed = doc["wind"]["speed"].as<float>();

      Serial.print("Pressure: ");
      if (Firebase.set(firebaseData, firebasePressurePath, pressure)) {
        Serial.println("pressure sent to Firebase successfully!");
      }
      Serial.print(pressure);
      Serial.println(" hPa");

      Serial.print("Wind Speed: ");
      Serial.print(windSpeed);
      if (Firebase.set(firebaseData, firebaseWindPath, windSpeed)) {
        Serial.println("windSpeed sent to Firebase successfully!");
      }
      Serial.println(" m/s");
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end();
  }

  // Prepare Firestore document data as a JSON object
  DynamicJsonDocument json(1024);
  json["fields"]["temperature"]["doubleValue"] = temperature;
  json["fields"]["humidity"]["doubleValue"] = humidity;
  json["fields"]["pressure"]["doubleValue"] = pressure;
  json["fields"]["windSpeed"]["doubleValue"] = windSpeed;

  if (Firebase.set(firebaseData, firebaseTempPath, temperature)) {
    Serial.println("temperature sent to Firebase successfully!");
  }
  
  if (Firebase.set(firebaseData, firebaseHumidityPath, humidity)) {
    Serial.println("humidity sent to Firebase successfully!");
  }

  // Send data to Firestore using HTTP
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(firebaseHost) + "/weatherData?key=" + firebaseAuth;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    String jsonStr;
    serializeJson(json, jsonStr);

    int httpCode = http.POST(jsonStr);

    if (httpCode > 0) {
      Serial.println("Data sent successfully to Firestore");
      Serial.println(http.getString());
    } else {
      Serial.print("Error sending data: ");
      Serial.println(http.errorToString(httpCode));
    }

    http.end();
  }

  // Display the weather data on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");

  lcd.setCursor(0, 1);
  
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(0, 2);
  lcd.print("Pressure: ");
  lcd.print(pressure);
  lcd.print("hPa");

  lcd.setCursor(0, 3);
  lcd.print("Wind: ");
  lcd.print(windSpeed);
  lcd.print(" m/s");
  
  

  delay(60000);  // Delay before sending the next data (1 minute)
}
