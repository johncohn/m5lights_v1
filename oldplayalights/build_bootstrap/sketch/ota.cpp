#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ota.cpp"
#include "ota.h"

void initOTA(){
  // Only initialize OTA if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA disabled - no WiFi connection");
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
  
  Serial.printf("OTA initialized: %s.local (IP: %s)\n", 
    hostname.c_str(), WiFi.localIP().toString().c_str());
}

void setOTACallbacks(){
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    Serial.println("OTA Start updating " + type);
    
    // Show OTA status on LED strip - solid blue
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
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
    canvas.pushSprite(0, 0);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Complete!");
    
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
    canvas.print("Rebooting...");
    canvas.pushSprite(0, 0);
    
    delay(2000); // Show success for 2 seconds before reboot
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Update progress every 500ms to avoid spam
    if (now - lastUpdate > 500) {
      lastUpdate = now;
      
      unsigned int percent = (progress / (total / 100));
      Serial.printf("OTA Progress: %u%% (%u/%u bytes)\n", percent, progress, total);
      
      // Progress bar on LED strip
      int ledsOn = (percent * NUM_LEDS) / 100;
      fill_solid(leds, ledsOn, CRGB::Blue);
      fill_solid(leds + ledsOn, NUM_LEDS - ledsOn, CRGB::Black);
      FastLED.show();
      
      // Progress on LCD
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
      
      canvas.pushSprite(0, 0);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    String errorMsg;
    
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
    canvas.setCursor(10, 70);
    canvas.print("Will retry...");
    canvas.pushSprite(0, 0);
    
    delay(5000); // Show error for 5 seconds then continue normal operation
  });
}

void handleOTA(){
  // Only handle OTA if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }
}