#include <Arduino.h>
#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/playalights_claude_v51.ino"
/*
  NeoPixel Controller - Modular Version with OTA
  22 patterns with robust networking, audio, UI, and OTA updates
  Refactored into manageable modules
*/

#include "config.h"
#include "patterns.h" 
#include "networking.h"
#include "audio.h"
#include "ui.h"
#include "ota.h"

// ── Global Variable Definitions ───────────────────────────────────────────────
Mode      currentMode      = AUTO;
FsmState  fsmState         = FOLLOWER;
uint8_t   styleIdx         = 0;
bool      freezeActive     = false;

CRGB      leds[NUM_LEDS];
LGFX_Sprite canvas(&M5.Lcd);
Preferences prefs;

// ── Control Arrays ────────────────────────────────────────────────────────────
uint8_t speedVals[MODE_COUNT][22], brightVals[MODE_COUNT][22],
        ssensVals[MODE_COUNT][22], bsensVals[MODE_COUNT][22],
        vsensVals[MODE_COUNT][22], decayVals[MODE_COUNT][22],
        timeVals[MODE_COUNT][22];

// ── Network Variables ─────────────────────────────────────────────────────────
uint8_t  broadcastAddress[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
uint32_t masterSeq         = 0, chunkMask      = 0, lastRecvMillis = 0;
uint32_t electionStart     = 0, electionEnd    = 0;
uint32_t myToken           = 0, highestTokenSeen = 0, myDelay = 0;
bool     electionBroadcasted = false;
uint32_t lastTokenBroadcast  = 0, lastHeartbeat      = 0, missedFrameCount   = 0;

// ── Audio Variables ───────────────────────────────────────────────────────────
float   soundMin      = 1.0f, soundMax      = 0.0f, musicLevel = 0.0f;
bool    prevAbove     = false;
uint32_t beatTimes[50];
uint8_t  beatCount    = 0;
uint32_t lastBpmMillis= 0;
bool    audioDetected = true;

// ── Setup ─────────────────────────────────────────────────────────────────────
#line 47 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/playalights_claude_v51.ino"
void setup();
#line 79 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/playalights_claude_v51.ino"
void loop();
#line 112 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ota_bootstrap.ino"
void connectWiFi();
#line 159 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ota_bootstrap.ino"
void initOTA();
#line 220 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ota_bootstrap.ino"
void showInfo();
#line 47 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/playalights_claude_v51.ino"
void setup(){
  Serial.begin(115200);
  
  // Initialize M5 hardware
  M5.begin();
  
  // Initialize all modules
  loadControls();
  initUI();
  initAudio();
  initNetworking();
  
  // Initialize OTA (must be after networking for WiFi)
  initOTA();
  
  // Initialize FastLED
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(map(getBright(), 0, 9, 0, 255));

  // Initialize timing
  randomSeed(micros());
  lastRecvMillis     = millis();
  lastTokenBroadcast = millis();
  lastHeartbeat      = millis();
  missedFrameCount   = 0;
  
  Serial.println("NeoPixel Controller initialized - 22 patterns ready!");
  Serial.printf("Ready for OTA updates at: NeoNode-%06X.local\n", myToken);
}

// ── Main Loop ─────────────────────────────────────────────────────────────────
void loop(){
  M5.update();
  
  // Handle OTA updates (highest priority)
  handleOTA();
  
  // Handle user input
  handleButtons();
  
  // Handle networking (AUTO mode) or local patterns (GO/QUIET)
  if(currentMode == AUTO){
    handleNetworking();
    drawUI(); // FIXED: Added missing drawUI call for AUTO mode
  }
  else if(currentMode == GO){
    if(freezeActive) {
      effectWild();
    } else {
      runTimed(effectWild);
    }
    FastLED.show(); 
    drawUI();
  }
  else { // QUIET
    FastLED.setBrightness((map(getBright(), 0, 9, 0, 255) * 3) / 100);
    if(freezeActive) {
      effectWild();
    } else {
      runTimed(effectWild);
    }
    FastLED.show(); 
    drawUI();
  }

  // Update audio processing
  updateBPM();
}
#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ota_bootstrap.ino"
/*
  OTA Bootstrap - Minimal code to enable OTA on new M5StickC Plus2 devices
  Upload this via USB first, then use OTA for full NeoPixel code
  
  Compatible with your main NeoPixel controller OTA system
*/

#include <M5Unified.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>
#include <FastLED.h>

// ── WiFi Configuration (CHANGE THESE TO MATCH YOUR NETWORK) ──────────────────
#define WIFI_SSID     "Barn"     // Your WiFi name
#define WIFI_PASSWORD "3576wifi"    // Your WiFi password

// ── Hardware Configuration ────────────────────────────────────────────────────
#define NEOPIXEL_PIN  27  // M5StickC Plus2 built-in NeoPixel pin
#define NUM_PIXELS    1   // Single built-in NeoPixel

// ── Global Variables ──────────────────────────────────────────────────────────
uint32_t myToken = 0;
unsigned long lastBlink = 0;
bool ledState = false;
CRGB neopixel[NUM_PIXELS];

void setup() {
  Serial.begin(115200);
  M5.begin();
  
  // Initialize NeoPixel
  FastLED.addLeds<WS2812, NEOPIXEL_PIN, GRB>(neopixel, NUM_PIXELS);
  neopixel[0] = CRGB::Blue;  // Blue = booting
  FastLED.show();
  
  // Initialize LCD
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.print("OTA Bootstrap");
  
  // Get unique token from MAC address (same method as main code)
  uint8_t mac_raw[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac_raw);
  myToken = ((uint32_t)mac_raw[3]<<16) | ((uint32_t)mac_raw[4]<<8) | (uint32_t)mac_raw[5];
  
  Serial.printf("Device Token: 0x%06X\n", myToken);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.printf("Token: %06X", myToken);
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize OTA
  initOTA();
  
  Serial.println("Bootstrap ready! Use OTA to upload main code.");
  
  // Show ready status
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.print("Ready for OTA!");
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.printf("IP: %s", WiFi.localIP().toString().c_str());
  
  // NeoPixel green = ready for OTA
  neopixel[0] = CRGB::Green;
  FastLED.show();
}

void loop() {
  M5.update();
  ArduinoOTA.handle();
  
  // Blink built-in NeoPixel to show it's alive
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    ledState = !ledState;
    
    // Pulse between green and dim green
    if (WiFi.status() == WL_CONNECTED) {
      neopixel[0] = ledState ? CRGB::Green : CRGB(0, 64, 0);  // Bright/dim green
    } else {
      neopixel[0] = ledState ? CRGB::Red : CRGB(64, 0, 0);    // Bright/dim red
    }
    FastLED.show();
    
    // Update WiFi status on LCD
    M5.Lcd.fillRect(10, 130, 200, 20, TFT_BLACK);
    M5.Lcd.setCursor(10, 130);
    if (WiFi.status() == WL_CONNECTED) {
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.printf("WiFi: %d dBm", WiFi.RSSI());
    } else {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.print("WiFi: Disconnected");
    }
  }
  
  // Button A: Show info
  if (M5.BtnA.wasClicked()) {
    showInfo();
  }
  
  delay(10);
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.print("Connecting WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Show progress
    M5.Lcd.fillRect(180, 60, 50, 20, TFT_BLACK);
    M5.Lcd.setCursor(180, 60);
    M5.Lcd.printf("%d/30", attempts);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    M5.Lcd.fillRect(10, 60, 220, 20, TFT_BLACK);
    M5.Lcd.setCursor(10, 60);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.print("WiFi Connected!");
    
    // NeoPixel yellow = WiFi connected, setting up OTA
    neopixel[0] = CRGB::Yellow;
    FastLED.show();
  } else {
    Serial.println("WiFi connection failed!");
    
    M5.Lcd.fillRect(10, 60, 220, 20, TFT_BLACK);
    M5.Lcd.setCursor(10, 60);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.print("WiFi Failed!");
    
    // NeoPixel red = WiFi failed
    neopixel[0] = CRGB::Red;
    FastLED.show();
  }
}

void initOTA() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA disabled - no WiFi");
    return;
  }
  
  // Create hostname (same format as main code)
  String hostname = "NeoNode-" + String(myToken, HEX);
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword("neopixel123");  // Same password as main code
  
  // OTA callbacks with LCD feedback
  ArduinoOTA.onStart([]() {
    M5.Lcd.fillScreen(TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.print("OTA UPDATE");
    Serial.println("OTA Update Start");
  });
  
  ArduinoOTA.onEnd([]() {
    M5.Lcd.fillScreen(TFT_GREEN);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.print("UPDATE");
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.print("COMPLETE!");
    Serial.println("OTA Update Complete");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress / (total / 100));
    M5.Lcd.fillRect(10, 100, 220, 20, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.printf("Progress: %u%%", percent);
    
    // Progress bar
    int barWidth = 200;
    int barProgress = (percent * barWidth) / 100;
    M5.Lcd.drawRect(10, 120, barWidth, 10, TFT_WHITE);
    M5.Lcd.fillRect(10, 120, barProgress, 10, TFT_WHITE);
    
    Serial.printf("OTA Progress: %u%%\n", percent);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    M5.Lcd.fillScreen(TFT_RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.print("OTA ERROR");
    Serial.printf("OTA Error: %u\n", error);
  });
  
  ArduinoOTA.begin();
  Serial.printf("OTA Ready: %s.local\n", hostname.c_str());
}

void showInfo() {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE);
  
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.print("=== DEVICE INFO ===");
  
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.printf("Token: %06X", myToken);
  
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.printf("Hostname: NeoNode-%06X", myToken);
  
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.printf("IP: %s", WiFi.localIP().toString().c_str());
    
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.printf("SSID: %s", WiFi.SSID().c_str());
    
    M5.Lcd.setCursor(10, 110);
    M5.Lcd.printf("Signal: %d dBm", WiFi.RSSI());
  } else {
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.print("WiFi: Not Connected");
  }
  
  M5.Lcd.setCursor(10, 140);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.print("Press A again for main screen");
  
  delay(500); // Debounce
  while (!M5.BtnA.wasClicked()) {
    M5.update();
    delay(10);
  }
  
  // Return to main screen
  setup();
}
