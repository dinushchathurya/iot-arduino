#if defined(ESP32)
  #include <WiFi.h>
#endif
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DATABASE_URL ""
#define API_KEY ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define BUILTIN_LED 2
#define LED_01_RED 19
#define LED_01_GREEN 5
#define LED_01_BLUE 18
#define LED_02_RED 17
#define LED_02_GREEN 16
#define LED_02_BLUE 15
#define LED_03_RED 26
#define LED_03_GREEN 33
#define LED_03_BLUE 25
#define LED_04_RED 32
#define LED_04_GREEN 27
#define LED_04_BLUE 14

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  initializePins();
  initializeSerial();
  initializeDisplay();

  connectToWiFi();
  initializeFirebase();

  if (Firebase.ready() && signupOK) {
    displayWifiConnection(WIFI_SSID);
  }
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 3000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    updateSlotAvailability();
  }

  handleWiFiReconnect();
}

void initializePins() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED_01_RED, OUTPUT);
  pinMode(LED_01_GREEN, OUTPUT);
  pinMode(LED_01_BLUE, OUTPUT);
  pinMode(LED_02_RED, OUTPUT);
  pinMode(LED_02_GREEN, OUTPUT);
  pinMode(LED_02_BLUE, OUTPUT);
  pinMode(LED_03_RED, OUTPUT);
  pinMode(LED_03_GREEN, OUTPUT);
  pinMode(LED_03_BLUE, OUTPUT);
  pinMode(LED_04_RED, OUTPUT);
  pinMode(LED_04_GREEN, OUTPUT);
  pinMode(LED_04_BLUE, OUTPUT);

  turnOffAllLeds();
}

void turnOffAllLeds() {
  digitalWrite(LED_01_RED, HIGH);
  digitalWrite(LED_01_GREEN, HIGH);
  digitalWrite(LED_01_BLUE, HIGH);
  digitalWrite(LED_02_RED, HIGH);
  digitalWrite(LED_02_GREEN, HIGH);
  digitalWrite(LED_02_BLUE, HIGH);
  digitalWrite(LED_03_RED, HIGH);
  digitalWrite(LED_03_GREEN, HIGH);
  digitalWrite(LED_03_BLUE, HIGH);
  digitalWrite(LED_04_RED, HIGH);
  digitalWrite(LED_04_GREEN, HIGH);
  digitalWrite(LED_04_BLUE, HIGH);
}

void initializeSerial() {
  Serial.begin(9600);
}

void initializeDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  displaySearchingWifi();
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    toggleBuiltinLed();
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());
  }
  Serial.println("---------------------");
}

void toggleBuiltinLed() {
  digitalWrite(BUILTIN_LED, HIGH);
  delay(200);
  digitalWrite(BUILTIN_LED, LOW);
  delay(200);
}

void initializeFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Connected to Firebase");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("---------------------");
}

void handleWiFiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
}

void updateSlotAvailability() {
  updateSlot("remaining_slots", showAvailability);
  updateSlot("/slots/slot_01/availability", setLEDColor, LED_01_RED, LED_01_GREEN, LED_01_BLUE);
  updateSlot("/slots/slot_02/availability", setLEDColor, LED_02_RED, LED_02_GREEN, LED_02_BLUE);
  updateSlot("/slots/slot_03/availability", setLEDColor, LED_03_RED, LED_03_GREEN, LED_03_BLUE);
  updateSlot("/slots/slot_04/availability", setLEDColor, LED_04_RED, LED_04_GREEN, LED_04_BLUE);
}

template<typename Function, typename... Args>
void updateSlot(const String& path, Function func, Args... args) {
  if (Firebase.RTDB.getInt(&fbdo, path)) {
    if (fbdo.dataType() == "int") {
      int value = fbdo.intData();
      func(value, args...);
    }
  } else {
    Serial.println(fbdo.errorReason());
  }
}

void setLEDColor(int value, int redPin, int greenPin, int bluePin) {
  switch (value) {
    case 0:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
      break;
    case 1:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, HIGH);
      break;
    case 2:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
      break;
    default:
      Serial.println("Invalid color code: " + String(value));
      break;
  }
}

// OLED Display Functions

void displayWifiConnection(const String& ssid) {
  display.clearDisplay();
  const unsigned char bitmap_wifi[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 
    0x00, 0x7f, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xf8, 
    0x3f, 0xc0, 0x03, 0xfc, 0x7f, 0x03, 0xc0, 0xfe, 0xfc, 0x3f, 0xfc, 0x3f, 0xf8, 0xff, 0xff, 0x1f, 
    0xf1, 0xff, 0xff, 0x8f, 0x07, 0xff, 0xff, 0xe0, 0x0f, 0xe0, 0x07, 0xf0, 0x0f, 0xc1, 0x83, 0xf0, 
    0x0f, 0x0f, 0xf0, 0xf0, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 
    0x00, 0x78, 0x1e, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x07, 0xe0, 0x00, 
    0x00, 0x07, 0xe0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
    0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  display.drawBitmap(0, 20, bitmap_wifi, 32, 32, WHITE);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(40, 30);
  display.print("WiFi Connected");
  display.setCursor(55, 40);
  display.print("- ");
  display.print(ssid);
  display.print(" -");
  display.display();
}

void displaySearchingWifi() {
  display.clearDisplay();
  const unsigned char bitmap_wifi[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 
    0x00, 0x7f, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xf8, 
    0x3f, 0xc0, 0x03, 0xfc, 0x7f, 0x03, 0xc0, 0xfe, 0xfc, 0x3f, 0xfc, 0x3f, 0xf8, 0xff, 0xff, 0x1f, 
    0xf1, 0xff, 0xff, 0x8f, 0x07, 0xff, 0xff, 0xe0, 0x0f, 0xe0, 0x07, 0xf0, 0x0f, 0xc1, 0x83, 0xf0, 
    0x0f, 0x0f, 0xf0, 0xf0, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 
    0x00, 0x78, 0x1e, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x07, 0xe0, 0x00, 
    0x00, 0x07, 0xe0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
    0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  display.drawBitmap(50, 10, bitmap_wifi, 32, 32, WHITE);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(35, 50);
  display.print("Searching...");
  display.display();
}

void showAvailability(int slots) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  if (slots != 0) {
    display.setTextSize(4);
    display.setCursor(56, 10);
    display.println(slots);
    
    display.setTextSize(1);
    display.setCursor(20, 50);
    display.println("Slots Available");
  } else {
    display.setTextSize(1);
    display.setCursor(40, 10);
    display.println("Car Park");

    display.setTextSize(3);
    display.setCursor(30, 30);
    display.println("Full");
  }

  display.display();
}
