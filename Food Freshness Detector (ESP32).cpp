#include <WiFi.h>
#include <SimpleDHT.h>

/* -------- PIN DEFINITIONS -------- */
#define MQPIN 34
#define DHTPIN 4
#define BUZZER 23

/* -------- SENSOR OBJECT -------- */
SimpleDHT11 dht11;

/* -------- WIFI CONFIG -------- */
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

/* -------- CALIBRATION -------- */
const int FRESH_LIMIT = 130;
const int WARNING_LIMIT = 200;
const int HUM_OFFSET = 12;

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Serial.println("Starting Food Freshness Detector...");

  // WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("Open in Chrome: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {

  /* -------- READ SENSORS -------- */
  int gasValue = analogRead(MQPIN);

  byte temperature = 0;
  byte humidity = 0;
  dht11.read(DHTPIN, &temperature, &humidity, NULL);

  float calibratedHum = humidity - HUM_OFFSET;
  if (calibratedHum < 0) calibratedHum = 0;

  /* -------- STATUS LOGIC -------- */
  String status;
  digitalWrite(BUZZER, LOW);

  if (gasValue <= FRESH_LIMIT) {
    status = "Fresh";
  }
  else if (gasValue <= WARNING_LIMIT) {
    status = "Consume Soon";
  }
  else {
    status = "Spoiled";
    digitalWrite(BUZZER, HIGH);
  }

  /* -------- SERIAL OUTPUT -------- */
  Serial.print("Gas: ");
  Serial.print(gasValue);
  Serial.print(" | Temp: ");
  Serial.print(temperature);
  Serial.print(" | Humidity: ");
  Serial.print(calibratedHum);
  Serial.print(" | Status: ");
  Serial.println(status);

  /* -------- WEB SERVER -------- */
  WiFiClient client = server.available();

  if (client) {
    while (client.connected() && !client.available()) {
      delay(1);
    }

    while (client.available()) {
      client.read();
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html><head>");
    client.println("<meta http-equiv='refresh' content='3'>");
    client.println("<title>Food Freshness</title>");
    client.println("<style>");
    client.println("body { font-family: Arial; text-align: center; background: #f4f4f4; }");
    client.println(".card { background: white; padding: 20px; margin: 20px auto; width: 300px; border-radius: 10px; box-shadow: 0 0 10px #ccc; }");
    client.println(".value { font-size: 28px; font-weight: bold; }");
    client.println("</style>");
    client.println("</head><body>");

    client.println("<h2>Food Freshness Detector</h2>");
    client.println("<div class='card'>");

    client.println("<p>Gas Value</p>");
    client.println("<div class='value'>" + String(gasValue) + "</div>");

    client.println("<p>Temperature (C)</p>");
    client.println("<div class='value'>" + String(temperature) + "</div>");

    client.println("<p>Humidity (%)</p>");
    client.println("<div class='value'>" + String(calibratedHum) + "</div>");

    client.println("<h3>Status: " + status + "</h3>");

    client.println("</div>");
    client.println("</body></html>");

    client.stop();
  }

  delay(2000);
}
