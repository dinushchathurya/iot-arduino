#if defined(ESP32)
  #include <WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DATABASE_URL ""
#define API_KEY ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define BUILTIN_LED 2
#define ONE_PIR_TRIG 17
#define ONE_PIR_ECHO 16
#define TWO_PIR_TRIG 26
#define TWO_PIR_ECHO 27
#define THREE_PIR_TRIG 12
#define THREE_PIR_ECHO 14
#define FOUR_PIR_TRIG 23
#define FOUR_PIR_ECHO 22

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

int remainingSlot = 4;
int preRemainingSlot = 4;
int slotVal1 = 0;
int slotVal2 = 0;
int slotVal3 = 0;
int slotVal4 = 0;

void setup() {
  initializePins();
  Serial.begin(9600);

  connectToWiFi();
  initializeFirebase();
  if (Firebase.ready() && signupOK) {
    setFirebaseDefault();
  }
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 3000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    updateAllSlots();
  }

  handleWiFiReconnect();
}

void initializePins() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(ONE_PIR_TRIG, OUTPUT);
  pinMode(ONE_PIR_ECHO, INPUT);
  pinMode(TWO_PIR_TRIG, OUTPUT);
  pinMode(TWO_PIR_ECHO, INPUT);
  pinMode(THREE_PIR_TRIG, OUTPUT);
  pinMode(THREE_PIR_ECHO, INPUT);
  pinMode(FOUR_PIR_TRIG, OUTPUT);
  pinMode(FOUR_PIR_ECHO, INPUT);
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    toggleLed();
    Serial.print(".");
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());
  }
  Serial.println("---------------------");
}

void toggleLed() {
  digitalWrite(BUILTIN_LED, HIGH);
  delay(200);
  digitalWrite(BUILTIN_LED, LOW);
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

void updateAllSlots() {
  updateSlot(ONE_PIR_TRIG, ONE_PIR_ECHO, "/slots/slot_01/availability", 1);
  updateSlot(TWO_PIR_TRIG, TWO_PIR_ECHO, "/slots/slot_02/availability", 2);
  updateSlot(THREE_PIR_TRIG, THREE_PIR_ECHO, "/slots/slot_03/availability", 3);
  updateSlot(FOUR_PIR_TRIG, FOUR_PIR_ECHO, "/slots/slot_04/availability", 4);
}

void updateSlot(int trigPin, int echoPin, const String &slot, int slotNum) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long cm = (duration / 2) / 29.1;
  int minimumDistance = 10; // set activation distance to 10cm

  int slotState = (cm < minimumDistance) ? 1 : 0;
  if (Firebase.RTDB.setInt(&fbdo, slot, slotState)) {
    updateRemainingSlots(slotState, slotNum);
  } else {
    Serial.println("FAILED to update slot: " + slot);
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void updateRemainingSlots(int state, int slotNum) {
  int *slotVal = nullptr;
  switch (slotNum) {
    case 1: slotVal = &slotVal1; break;
    case 2: slotVal = &slotVal2; break;
    case 3: slotVal = &slotVal3; break;
    case 4: slotVal = &slotVal4; break;
  }

  if (slotVal && *slotVal != state) {
    remainingSlot += (state == 0) ? 1 : -1;
    *slotVal = state;
  }

  if (preRemainingSlot != remainingSlot) {
    if (Firebase.RTDB.setInt(&fbdo, "/remaining_slots", remainingSlot)) {
      Serial.println("Remaining Slots Set to: " + String(remainingSlot));
      preRemainingSlot = remainingSlot;
    } else {
      Serial.println("FAILED to update remaining slots");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void setFirebaseDefault() {
  if (Firebase.RTDB.setInt(&fbdo, "/remaining_slots", 4)) {
    Serial.println("Remaining Slot count set to default");
  } else {
    Serial.println("FAILED to set default remaining slots");
    Serial.println("REASON: " + fbdo.errorReason());
  }

  for (int x = 1; x <= 4; x++) {
    if (Firebase.RTDB.setInt(&fbdo, "/slots/slot_0" + String(x) + "/availability", 0)) {
      Serial.println("Slot 0" + String(x) + " Data set to default");
    } else {
      Serial.println("FAILED to set default slot 0" + String(x));
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
