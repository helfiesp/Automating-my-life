// This program was built to fix an old water based central heating system.
// The system would not properly turn on and off, and would consume massive amounts of power.
// To fix this issue, this program was made to turn the system on and off on a timer.
// The user can activate or change the run-time of the system via the webserver.

// Board used: Nodemcu v3
// Stepper motor used: Nema 17
// Stepper motor driver board: A4988
// Display used: 128x64 OLED

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Screen setup
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define stepper motor pins
const int DIR = 13;
const int STEP = 12;
const int ENABLE = 14;

// Define stepper motor speed
const int steps_per_rev = 100;

// Wifi information
const char *ssid = "WIFI";
const char *password = "PASSWORD";
IPAddress local_IP(192, 168, 1, 80);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Address to local temperature sensor
const String serverName = "http://192.168.1.90/temperature";

// Initialize loop variables
bool running = false; 
bool first_run = true;
float temperature;
int duration = 0; // Default duration

// Define a variable to store the last time temperature and hour were checked
unsigned long lastCheckTime = 0;
// Define a constant for the check interval in milliseconds (30 minutes)
const unsigned long checkInterval = 1800000;


// Initialize NTP, Wifi and HTTP clients
WiFiUDP ntpUDP;
WiFiClient client;
HTTPClient http;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);


ESP8266WebServer server(80); // Create web server on port 80

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'>";
  html += "<style>* {font-family: Calibri;}.heatcontrol {text-align: center;}</style>";
  html += "</head><body>";
  html += "<div class='heatcontrol'>";
  html += "<h1>Varmekontroll</h1>";
  if (duration == 0){
    html += "<h2>Systemet er: AV</h2>";
    html += "<p>Sluttid er satt til 0, og systemet er derfor avslått.<br>Endre sluttid til å være høyere enn 0 for å slå på systemet.<br>Systemet vil være aktivt i så mange timer som sluttid er satt til.<br>Virketid er fra 00:00 til sluttid.</p>";
  }
  else {
    html += "<h2>Systemet er: PÅ</h2>";
    html += "<h2>Varmesystemet er på fra kl 00:00 til 0" + String(duration) + ":00</h2>";
    html += "<p>Sett inn ny slutttid for å endre hvor lenge systemet står på.<br>Hvis sluttid settes til 0 vil systemet være avslått.</p>";
  }
  html += "<form method='POST' action='/set_duration'>";
  html += "<label style='margin-right:10px;'>Sluttid:</label>";
  html += "<input style='width: 35;' type='number' name='duration' value='" + String(duration) + "' />";
  html += "<br><br><input style='padding:5px;' type='submit' value='Endre sluttid' />";
  html += "</form>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void handleSetDuration() {
  if (server.hasArg("duration")) {
    duration = server.arg("duration").toInt();
  }
  handleRoot();
}

void setup() {
  // Begin serial on 115200
  Serial.begin(115200);

  // Connect to Wifi
  Serial.print("Configuring access point...");
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STA Failed to configure");
    }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected, IP: ");
  Serial.print(WiFi.localIP());

  // Enable stepper motor pins 
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(ENABLE, OUTPUT);
  // Disable stepper motor
  digitalWrite(ENABLE,HIGH);

  // Start display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  timeClient.begin();
  delay(2000);

  // Start web server
  server.on("/", handleRoot);
  server.on("/set_duration", handleSetDuration);
  server.begin();

  http.begin(client, serverName.c_str());
  int httpResponseCode = http.GET(); 
  if (httpResponseCode > 0) {
    temperature = http.getString().toFloat();
  }

  // Get current time in hour
  timeClient.update();
  int current_hour = timeClient.getHours();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,8);
}

void RunStepperMotor(int direction) {
  // Set direction of motor
  digitalWrite(DIR, direction);
  
  // Enable motor
  digitalWrite(ENABLE, LOW);

  // Step motor for the specified duration
  int steps = steps_per_rev * duration;
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(500); // Adjust delay as needed for desired motor speed
    digitalWrite(STEP, LOW);
    delayMicroseconds(500); // Adjust delay as needed for desired motor speed
  }

  // Disable motor
  digitalWrite(ENABLE, HIGH);
  
  // Stop motor
  digitalWrite(STEP, LOW);
}

void loop() {
  // Get current time in milliseconds
  unsigned long currentTime = millis();

  // Check if it's time to check the temperature and current hour
  if (currentTime - lastCheckTime >= checkInterval) {
    // Update the last check time
    lastCheckTime = currentTime;

    // Perform temperature check
    http.begin(client, serverName.c_str());
    int httpResponseCode = http.GET(); 
    if (httpResponseCode > 0) {
      temperature = http.getString().toFloat();
    }
    Serial.println(temperature);

    // Get current time in hour
    timeClient.update();
    int current_hour = timeClient.getHours();
    
    // Start mainloop
    if (current_hour <= duration && duration != 0) {
      if (running) {
        RunStepperMotor(1);
        running = false;
      }
    } 
    else {
      if (!running) {
        RunStepperMotor(0);
        running = true;
      }
    }

  display.clearDisplay();
    if (running) {
      display.println("Running...");
    } else {
      display.println("192.168.1.80");
    }
    display.print(temperature);
    display.println(" C");
    display.display();

  }
  if(first_run) {  
    if(duration == 0){
      display.println("Status: AV");
    }
    else {
      display.print("Status: ");
      display.println(duration);
    }
    display.print(temperature);
    display.println(" C");
    display.display();
    first_run = false;
  }
  server.handleClient();
}
