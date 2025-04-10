#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WIFI_SSID "TechnoPark"
#define WIFI_PASSWORD "techno2020"
#define API_KEY "AIzaSyAh4d3P2tQ9TT_o3-pUmzlATZurp9p8UKE"
#define DATABASE_URL "https://testified-c8b21-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "kerimbekbekgan@gmail.com"
#define USER_PASSWORD "B123321123"

#define SS_PIN 5  
#define RST_PIN 27
#define BUZZER_PIN 13 

MFRC522 mfrc522(SS_PIN, RST_PIN);
FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

bool adminCardDetected = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();      
  mfrc522.PCD_Init(); 

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  static unsigned long lastRFIDReadTime = millis();
  if (millis() - lastRFIDReadTime > 500) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      handleCardDetected();
    }
    lastRFIDReadTime = millis();
  }
}

void beep(unsigned long duration) {
  // static unsigned long startBeepTime = 0;
  // if (!isBeeping) {
  //   digitalWrite(BUZZER_PIN, HIGH);
  //   startBeepTime = millis();
  //   isBeeping = true;
  // }
  // if (millis() - startBeepTime > duration) {
  //   digitalWrite(BUZZER_PIN, LOW);
  //   isBeeping = false;
  // }
}

void displayInfo(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(message);
  display.display();
}

void handleCardDetected() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    uid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uid.toUpperCase();

  String message = "Card UID: " + uid;
  if (uid == "B5453ADE") {
    message += "\nAdmin card detected.\nThe next card will be added or reset.";
    beep(300);
    adminCardDetected = true;
  } else {
    message += "\nProcessing card...";
  }
  displayInfo(message);
  processCard(uid);
}

void processCard(String uid) {
  String path = "/RFID/" + uid;
  fbData.clear();
  if (Firebase.RTDB.getInt(&fbData, path)) {
    int currentValue = fbData.intData();
    String message = "Card Value: ";
    if (currentValue == 0 && adminCardDetected) {
      Firebase.RTDB.setInt(&fbData, path, 10);
      message += "Reset to 10";
      beep(200);
    } else if (currentValue > 0) {
      currentValue--;
      Firebase.RTDB.setInt(&fbData, path, currentValue);
      message += String(currentValue);
      beep(100);
    }
    adminCardDetected = false;
    displayInfo(message);
  } else {
    if (adminCardDetected) {
      Firebase.RTDB.setInt(&fbData, path, 10);
      displayInfo("New card added with value 10.");
      beep(200);
    } else {
      displayInfo("Card not found in database.");
    }
    adminCardDetected = false;
  }
}