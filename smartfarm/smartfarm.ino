#include <Arduino.h>
#include <Mhz19.h> //mhz19
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <SoftwareSerial.h> //espsoftwareserial 
#include <Wire.h>
#include "SHTSensor.h" //arduino-sht
#include <ESPmDNS.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <LiquidCrystal_I2C.h> // <<that name
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SHTSensor sht;  //21 SDA 22 SCL
Mhz19 co2;
SoftwareSerial softwareSerial(25, 26);  //RX, TX


const char* ssid = "ssid";
const char* password = "passwd";

//LiquidCrystal_I2C lcd(0x27,20,4);

WebServer server(80);
int temperature = 0;
void setup() {
  Serial.begin(115200);
  softwareSerial.begin(9600);
  Wire.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.setTextColor(SSD1306_WHITE);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Preheating...");
  display.display();

  co2.begin(&softwareSerial);
  co2.setMeasuringRange(Mhz19MeasuringRange::Ppm_5000);
  co2.enableAutoBaseCalibration();

  Serial.println("Preheating...");  // Preheating, 3 minutes
  while (!co2.isReady()) {
    delay(1000);
    Serial.print(".");
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Done!!");
  display.display();

  if (sht.init()) {
      Serial.print("init(): success\n");
  } else {
      Serial.print("init(): failed\n");
  }
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH); // only supported by SHT3x
  delay(1000);

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("wifi...");
  display.display();

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
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Done!!");
  display.println(WiFi.localIP());
  display.setTextSize(3);
  display.print(":>");
  display.display();
  
  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString());

  Serial.println("Ready...");
  digitalWrite(5, 1);
}

String GenerateMetrics(){
  sht.readSample();
  String message = "";

  message += "AirTemp ";
  message += sht.getTemperature();
  message += "\n";

  message += "AirHumidity ";
  message += sht.getHumidity();
  message += "\n";

  message += "z19b_co2 ";
  message += getCO2();
  message += "\n";

  message += "soil_moisture ";
  message += analogRead(33);
  message += "\n";
  return message;
  Serial.print(message);
}

void HandleRoot()
{
  sht.readSample();
  temperature = sht.getTemperature();
  server.send(200, "text/plain", GenerateMetrics());
  Serial.print(GenerateMetrics());
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
