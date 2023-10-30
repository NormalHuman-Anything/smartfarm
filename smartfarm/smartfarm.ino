#include <Arduino.h>
#include <Mhz19.h> //mhz19
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <SoftwareSerial.h> //espsoftwareserial 
#include <Wire.h>
#include "SHTSensor.h" //arduino-sht
#include <ESPmDNS.h>
#include <LiquidCrystal_I2C.h> // <<that name

SHTSensor sht;  //21 SDA 22 SCL
Mhz19 co2;
SoftwareSerial softwareSerial(25, 26);  //RX, TX

const char* ssid = "iptime";
const char* password = "ppap1542xd";

LiquidCrystal_I2C lcd(0x27,20,4);

WebServer server(80);
int temperature = 0;
void setup() {
  Serial.begin(115200);
  softwareSerial.begin(9600);

  Wire.begin();

  lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();

  co2.begin(&softwareSerial);
  co2.setMeasuringRange(Mhz19MeasuringRange::Ppm_5000);
  co2.enableAutoBaseCalibration();

    Serial.println("Preheating...");  // Preheating, 3 minutes
  while (!co2.isReady()) {
    delay(50);
    Serial.print(".");
  }

  if (sht.init()) {
      Serial.print("init(): success\n");
  } else {
      Serial.print("init(): failed\n");
  }
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH); // only supported by SHT3x

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  lcd.setCursor(0,0);
  lcd.print(WiFi.localIP());  
  
  server.on("/", HandleRoot);
  //server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString());

  Serial.println("Ready...");
  digitalWrite(5, 1);
}

String GenerateMetrics(){
  sht.readSample();
  String message = "";

  message += "sht.getTemperature ";
  message += sht.getTemperature();
  message += "\n";

  message += "sht.getHumidity ";
  message += sht.getHumidity();
  message += "\n";

  message += "mh-z19b_co2 ";
  message += getCO2();
  message += "\n";

  message += "moisture ";
  message += analogRead(33);
  message += "\n";
  return message;
}

void HandleRoot()
{
  sht.readSample();
  temperature = sht.getTemperature();
  server.send(200, "text/plain", GenerateMetrics());
}
void HandleNotFound()
{
  String message = "Error\n";
  server.send(404, "text/html", message);
}

int getCO2(){
  int carbonDioxide = co2.getCarbonDioxide();
  if(carbonDioxide >= 0){
    Serial.println(String(carbonDioxide) + " ppm");
  }
  return carbonDioxide;
}

void loop() {
  //Serial.println(getTemp);
  //Serial.print("\n");
  //Serial.println(getHumidity);
  //Serial.print("\n");
  //Serial.println(getCO2));
  //Serial.print("\n");
  sht.readSample();
  digitalWrite(5, 1);
  server.handleClient();
  delay(2);
}
