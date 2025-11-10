#include "ota.h"
#include "networking.h"
#include "esp_task_wdt.h"

// OTA mode state tracking
static bool otaMode = false;
static unsigned long otaModeStartTime = 0;
static const unsigned long OTA_MODE_TIMEOUT = 300000; // 5 minutes
static bool espnowWasActive = false;

void enterOTAMode() {
  if (otaMode) return; // Already in OTA mode
  
  if(DEBUG_SERIAL) Serial.println("[OTA] Entering OTA mode - disabling ESP-NOW");
  
  // Check if ESP-NOW is initialized by trying to get peer count
  esp_now_peer_num_t peerNum;
  esp_err_t result = esp_now_get_peer_num(&peerNum);
  espnowWasActive = (result == ESP_OK && peerNum.total_num > 0);
  
  if(DEBUG_SERIAL) {
    Serial.printf("[OTA] ESP-NOW status: %s (%d peers)\n", 
      espnowWasActive ? "ACTIVE" : "INACTIVE", 
      espnowWasActive ? peerNum.total_num : 0);
  }
  
  // Completely disable ESP-NOW and clear all peers
  if (espnowWasActive) {
    if(DEBUG_SERIAL) Serial.println("[OTA] Deinitializing ESP-NOW...");
    esp_now_deinit();
  }
  
  // Force WiFi into pure STA mode with aggressive settings
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false); // Disable WiFi sleep for stability
  
  // Clear any residual network operations
  delay(100);
  
  otaMode = true;
  otaModeStartTime = millis();
  
  if(DEBUG_SERIAL) Serial.println("[OTA] Mode activated - ESP-NOW disabled, WiFi optimized");
}

void exitOTAMode() {
  if (!otaMode) return; // Not in OTA mode
  
  if(DEBUG_SERIAL) Serial.println("[OTA] Exiting OTA mode - re-enabling systems");
  
  otaMode = false;
  
  // Restore WiFi power management
  WiFi.setSleep(true);
  
  // Wait before reinitializing ESP-NOW to avoid conflicts
  delay(500);
  
  if(DEBUG_SERIAL) Serial.println("[OTA] Mode exited - systems will reinitialize");
}

bool isInOTAMode() {
  return otaMode;
}

// Flag to track if OTA has been initialized
static bool otaInitialized = false;

void initOTA(){
  // Check if already initialized
  if (otaInitialized) {
    return;
  }
  
  // Only initialize OTA if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    if(DEBUG_SERIAL) Serial.println("OTA disabled - no WiFi connection");
    return;
  }
  
  // Create unique hostname using the node's token
  String hostname = "NeoNode-" + String(myToken, HEX);
  ArduinoOTA.setHostname(hostname.c_str());
  
  // Set password to match what Arduino IDE is sending
  ArduinoOTA.setPassword("neopixel123");
  
  // Set port (default 3232 is fine for most cases)
  ArduinoOTA.setPort(3232);
  
  setOTACallbacks();
  ArduinoOTA.begin();
  
  // Mark as initialized
  otaInitialized = true;
  
  if(DEBUG_SERIAL) {
    Serial.printf("OTA initialized: %s.local (IP: %s)\n", 
      hostname.c_str(), WiFi.localIP().toString().c_str());
  }
}

void setOTACallbacks(){
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    if(DEBUG_SERIAL) Serial.println("OTA Start updating " + type);
    
    // ESP-NOW suspension is now controlled manually via serial commands
    // Use deploy script: echo "SUSPEND_ESPNOW" > /dev/ttyACM0 before OTA
    
    // Stop our own ESP-NOW (others controlled via serial command)
    esp_now_deinit();
    WiFi.mode(WIFI_STA); // Ensure we stay in STA mode for OTA
    
    // Turn off all LEDs to reduce power consumption during upload
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    
    // Show OTA status on LED strip - solid blue
    for(int i = 0; i < NUM_LEDS; i += 10) {
      leds[i] = CRGB::Blue;
    }
    FastLED.show();
    
    // Show OTA status on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_BLUE);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("OTA UPDATE");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print("Updating " + type + "...");
    canvas.setCursor(10, 65);
    canvas.print("ESP-NOW disabled");
    canvas.pushSprite(0, 0);
  });

  ArduinoOTA.onEnd([]() {
    if(DEBUG_SERIAL) Serial.println("[OTA] Update completed successfully!");
    
    // Success indication - solid green
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    
    // Show success on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_GREEN);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("OTA SUCCESS");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print("Rebooting in 3s...");
    canvas.setCursor(10, 65);
    canvas.print("Systems will restore");
    canvas.pushSprite(0, 0);
    
    // ESP-NOW resumption is now controlled manually via serial commands  
    // Use deploy script: echo "RESUME_ESPNOW" > /dev/ttyACM0 after OTA
    
    delay(3000); // Show success for 3 seconds before reboot
    if(DEBUG_SERIAL) Serial.println("[OTA] Rebooting now...");
    // ESP will automatically restart after OTA completion
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Update progress every 2000ms to minimize interference with upload
    if (now - lastUpdate > 2000) {
      lastUpdate = now;
      
      unsigned int percent = (progress / (total / 100));
      if(DEBUG_SERIAL) {
        Serial.printf("OTA Progress: %u%% (%u/%u bytes)\n", percent, progress, total);
      }
      
      // NO LED updates during upload - keep them dark for maximum stability
      
      // Update LCD progress - simplified and less frequent
      canvas.fillSprite(TFT_BLACK);
      canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_BLUE);
      canvas.setTextSize(2);
      canvas.setTextColor(TFT_WHITE);
      canvas.setCursor(10, 10);
      canvas.print("OTA UPDATE");
      canvas.setTextSize(1);
      canvas.setCursor(10, 50);
      canvas.printf("Progress: %u%%", percent);
      
      // Progress bar on LCD
      int barWidth = M5.Lcd.width() - 20;
      int barProgress = (percent * barWidth) / 100;
      canvas.drawRect(10, 70, barWidth, 10, TFT_WHITE);
      canvas.fillRect(10, 70, barProgress, 10, TFT_WHITE);
      
      // Show status
      canvas.setCursor(10, 85);
      canvas.print("LEDs: OFF for stability");
      canvas.setCursor(10, 100);
      canvas.print("ESP-NOW: disabled");
      
      canvas.pushSprite(0, 0);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    String errorMsg = "Unknown error";
    
    if(DEBUG_SERIAL) {
      Serial.printf("[OTA] Error[%u]: ", error);
      
      if (error == OTA_AUTH_ERROR) {
        errorMsg = "Auth Failed";
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        errorMsg = "Begin Failed";
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        errorMsg = "Connect Failed";
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        errorMsg = "Receive Failed";
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        errorMsg = "End Failed";
        Serial.println("End Failed");
      }
    }
    
    // Error indication - solid red
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    
    // Show error on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_RED);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("OTA ERROR");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print(errorMsg);
    canvas.setCursor(10, 65);
    canvas.print("Restart in 3s");
    canvas.setCursor(10, 80);
    canvas.print("Will exit OTA mode");
    canvas.pushSprite(0, 0);
    
    // ESP-NOW resumption is now controlled manually via serial commands
    // Use deploy script: echo "RESUME_ESPNOW" > /dev/ttyACM0 after OTA failure
    
    delay(3000); // Show error for 3 seconds
    
    // Exit OTA mode before restart to ensure clean recovery
    exitOTAMode();
    delay(500);
    
    if(DEBUG_SERIAL) Serial.println("[OTA] Restarting after error...");
    ESP.restart();
  });
}

bool isESPNowActive(){
  // Check if ESP-NOW is currently active by checking peer count
  esp_now_peer_num_t peerNum;
  esp_err_t result = esp_now_get_peer_num(&peerNum);
  return (result == ESP_OK && peerNum.total_num > 0);
}

void handleOTA(){
  // Only handle OTA if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }
}