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
const int steps_per_rev = 60;

// Wifi information
const char *ssid = "**************";
const char *password = "************";
IPAddress local_IP(192, 168, 1, 80);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Address to local temperature sensor
const String serverName = "http://192.168.1.90/temperature";

// Initialize loop variables
bool running = false; 
bool forced_run = false;
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
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>* {font-family: Helvetica;}.heatcontrol {text-align: center;}h1, h2, p, label{ max-width: 100%; width: 90%; margin: 0 auto; margin-bottom: 10px; }";
  html += "input[type='submit'] { padding: 10px; color: white; border: 1px solid; cursor: pointer; border-radius:5px;}.form-inline { display: flex; justify-content: center; } .form-inline form { margin-right: 10px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='heatcontrol'>";
  html += "<h1>Varmekontroll</h1>";
  if (duration == 0){
    if (forced_run) {
      html += "<h2>Motor er: <span style='color:green';>AKTIV</span></h2>";  
      html += "<p>Motoren er slått på med tving start knappen og kjører utenom vanlig rutine.</p>";
    }
    else {
      html += "<h2>Systemet er: <span style='color:red;'>AV</span></h2>";
      html += "<p>Sluttid er satt til 0, og systemet er derfor avslått.<br>Endre sluttid til å være høyere enn 0 for å slå på systemet.<br>Systemet vil være aktivt i så mange timer som sluttid er satt til.<br>Virketid er fra 00:00 til sluttid.</p>";
    }
  }
  else {
    if (forced_run) {
      html += "<h2>Motor er: <span style='color:green';>AKTIV</span></h2>";  
      html += "<p>Motoren er slått på med tving start knappen og kjører utenom vanlig rutine.</p>";
    }
    else {
      html += "<h2>Systemet er: <span style='color:green';>AKTIVERT</span></h2>";
      html += "<h2>Varmesystemet er på fra kl 00:00 til 0" + String(duration) + ":00</h2>";
      html += "<p>Sett inn ny slutttid for å endre hvor lenge systemet står på.<br>Hvis sluttid settes til 0 vil systemet være avslått.</p>";
    }
  }
  html += "<form method='POST' action='/set_duration'>";
  html += "<label style='margin-right:10px;'>Sluttid:</label>";
  html += "<input style='width: 35;' type='number' name='duration' value='" + String(duration) + "' />";
  html += "<br><br><input style='border-color:blue; background-color: blue;' type='submit' value='Endre sluttid' />";
  html += "</form>";
  html += "<br>";
  html += "<div class='form-inline'>";
  html += "<form method='POST' action='/start_motor'>";
  html += "<input style='background-color: green; border-color: green;' type='submit' value='Tving start motor' />";
  html += "</form>";
  html += "<form method='POST' action='/stop_motor'>";
  html += "<input style='border-color: red; background-color: red;' type='submit' value='Tving stopp motor' />";
  html += "</form>";
  html += "</div>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void handleSetDuration() {
  // This function sets the duration value based on the input given by the user.
  if (server.hasArg("duration")) {
    duration = server.arg("duration").toInt();
  }
  DisplayStatus();
  handleRoot();
}

void handleStartMotor() {
  // This function is called whenever the motor is manually started by pressing the button on the website.
  // This can be done if the temperature is so low so that the system needs to be started outside of the normal cycle
  RunStepperMotor(0);
  forced_run = true;
  DisplayStatus();
  handleRoot();
}

void handleStopMotor() {
  RunStepperMotor(1);
  forced_run = false;
  DisplayStatus();
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
  server.on("/start_motor", handleStartMotor);
  server.on("/stop_motor", handleStopMotor);
  server.begin();

  http.begin(client, serverName.c_str());
  int httpResponseCode = http.GET(); 
  if (httpResponseCode > 0) {
    temperature = http.getString().toFloat();
  }

  // Get current time in hour
  timeClient.update();
  int current_hour = timeClient.getHours();
}

void DisplayInit() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,8);
}

void RunStepperMotor(int direction) {
  digitalWrite(ENABLE,LOW); // Enable stepper motor
  digitalWrite(DIR, direction); // Choose direction
  // Stepper motor rotates steps_per_rev.
  for(int i = 0; i<steps_per_rev; i++) {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(5000);
    digitalWrite(STEP, LOW);
    delayMicroseconds(5000);
  }
  digitalWrite(ENABLE,HIGH); // Disable stepper motor 
}

void DisplayStatus() {
  DisplayInit();
  if(forced_run) {
    display.println("Aktiv");
    display.println("Tvunget");
  }
  else if(duration == 0){
    display.println("Inaktiv");
  }
  else {
    display.println("Aktivert");
    display.print("00 -> 0");
    display.println(duration);
  }
  display.display();
}


void loop()   {
  unsigned long currentTime = millis(); // Get current time in milliseconds

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

    timeClient.update(); // Update timeclient
    int current_hour = timeClient.getHours(); // Get current hour in hour
    
    // This loop checks if the current hour is less than the value of the duration and if the duration has been set to more than 0.
    // An example is if the current hour is 3 and the duration is 5, the script will check if the motor is running or not. 
    // If the motor is not running, the motor will start.
    // If the motor is running, nothing will happen, as this is the intended behaviour.

    // if the current hour hour is more than the duration, and the motor is running, the motor will be stopped.
    if (current_hour <= duration) {
      if (!running && duration != 0) {
        RunStepperMotor(0); // Starts stepper motor
        running = true;
      } 
    } 
    else {
      if (running) {
        RunStepperMotor(1); // Stops the stepper motor
        running = false;
      }
    }
    DisplayStatus();
  }
  if(first_run) {  
    DisplayStatus();
    first_run = false;
  }
  server.handleClient();
}
