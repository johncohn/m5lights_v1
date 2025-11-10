#include "ui.h"
#include "version.h" // Include the auto-generated version file
#include "networking.h" // For forceSyncReset function

// Non-blocking UI timing
static uint32_t lastUIUpdate = 0;

const char* MODE_NAMES[MODE_COUNT] = {"AUTO","OFF"};

// Define brightness levels - LOCAL CONTROL ONLY
BrightnessLevel brightnessLevels[6] = {
  {"LOCAL 6%", 15},    // 6% brightness (~1/16th) - ultra dim
  {"LOCAL 12%", 32},   // 12.5% brightness (1/8th) - very dim
  {"LOCAL 25%", 64},   // 25% brightness (1/4th) - DEFAULT
  {"LOCAL 50%", 128},  // 50% brightness (1/2)
  {"LOCAL 75%", 192},  // 75% brightness (3/4)
  {"LOCAL 100%", 255}  // 100% brightness (full)
};

// External WiFi status variable (defined in networking.cpp)
extern bool wifiConnected;

void initUI(){
  M5.Lcd.setRotation(3);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
}

void loadControls(){
  prefs.begin("npref", false);
  
  // Load global brightness scale (stored separately)
  globalBrightnessScale = prefs.getUChar("globalBright", 64);  // Default 25%
  
  for(int m = 0; m < MODE_COUNT; ++m){
    for(int i = 0; i < 22; ++i){
      char k[8];
      snprintf(k, 8, "%d%dS", m, i); speedVals[m][i]  = prefs.getUChar(k, 5);
      snprintf(k, 8, "%d%dB", m, i); brightVals[m][i] = prefs.getUChar(k, 9);
      snprintf(k, 8, "%d%dX", m, i); ssensVals[m][i]  = prefs.getUChar(k, 5);
      snprintf(k, 8, "%d%dY", m, i); bsensVals[m][i]  = prefs.getUChar(k, 5);
      snprintf(k, 8, "%d%dV", m, i); vsensVals[m][i]  = prefs.getUChar(k, 5);
      snprintf(k, 8, "%d%dD", m, i); decayVals[m][i]  = prefs.getUChar(k, 5);
      snprintf(k, 8, "%d%dT", m, i); timeVals[m][i]   = prefs.getUChar(k, 1);
    }
  }
}

void saveControl(Control c){
  if(c == STYLE) return;
  char k[8]; 
  uint8_t v = 0;
  
  switch(c){
    case SPEED:  v = getSpeed();  snprintf(k, 8, "%d%dS", currentMode, styleIdx); break;
    case BRIGHT: v = getBright(); snprintf(k, 8, "%d%dB", currentMode, styleIdx); break;
    case SSENS:  v = getSS();     snprintf(k, 8, "%d%dX", currentMode, styleIdx); break;
    case BSENS:  v = getBS();     snprintf(k, 8, "%d%dY", currentMode, styleIdx); break;
    case VSENS:  v = getVS();     snprintf(k, 8, "%d%dV", currentMode, styleIdx); break;
    case DECAY:  v = getDe();     snprintf(k, 8, "%d%dD", currentMode, styleIdx); break;
    case TIME:   v = getTi();     snprintf(k, 8, "%d%dT", currentMode, styleIdx); break;
    default: return;
  }
  
  prefs.putUChar(k, v);
  if(c == BRIGHT) {
    // Apply global brightness scaling
    uint8_t scaledBrightness = (map(getBright(), 0, 9, 0, 255) * globalBrightnessScale) / 255;
    FastLED.setBrightness(scaledBrightness);
  }
}

// Function to save global brightness scale to flash
void saveGlobalBrightness(){
  prefs.putUChar("globalBright", globalBrightnessScale);
  if(DEBUG_SERIAL) {
    Serial.printf("Global brightness saved: %d/255 (%.1f%%)\n", 
      globalBrightnessScale, (globalBrightnessScale * 100.0f) / 255.0f);
  }
}

void handleButtons(){
  uint32_t now = millis();
  
  // Long press = OFF mode
  if(M5.BtnA.pressedFor(2000)) {
    if(currentMode != OFF) {
      currentMode = OFF;
      globalBrightnessScale = 0;
      
      // Turn off LEDs
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      
      // Dim display for battery savings
      M5.Lcd.setBrightness(20);
      
      if(DEBUG_SERIAL) Serial.println("Mode: OFF (short press to wake)");
      
      // Wait for button release to avoid immediate wake
      while(M5.BtnA.isPressed()) {
        M5.update();
        delay(50);
      }
    }
  }
  
  // Short press = brightness cycle (or wake from OFF)
  else if(M5.BtnA.wasClicked()) {
    if(currentMode == OFF) {
      // Wake up from OFF mode
      currentMode = AUTO;
      
      // Restore display brightness
      M5.Lcd.setBrightness(255);
      
      // Start at default brightness (25%)
      static uint8_t currentLevel = 2; // Start at 25% (index 2)
      globalBrightnessScale = brightnessLevels[currentLevel].scale;
      
      if(DEBUG_SERIAL) {
        Serial.printf("WAKE: %s - forcing sync reset\n", brightnessLevels[currentLevel].name);
      }
      
      // CRITICAL: Force sync reset when waking up
      // Node may have missed leader elections and pattern changes while sleeping
      forceSyncReset();
      
    } else {
      // Cycle brightness in AUTO mode
      static uint8_t currentLevel = 2; // Start at 25% (index 2)
      currentLevel = (currentLevel + 1) % 6; // Cycle through 6 brightness levels
      
      globalBrightnessScale = brightnessLevels[currentLevel].scale;
      
      if(DEBUG_SERIAL) Serial.printf("Brightness: %s\n", brightnessLevels[currentLevel].name);
    }
    
    saveGlobalBrightness();
    freezeActive = false; // Reset freeze when changing brightness
  }

  // Button C double-click = ESP-NOW suspend/resume toggle (safe, avoids long press conflicts)
  static uint32_t lastCClick = 0;
  static uint8_t cClickCount = 0;
  
  if(M5.BtnC.wasClicked() && currentMode == AUTO) {
    uint32_t now = millis();
    
    if(now - lastCClick < 500) { // Within 500ms = double click
      cClickCount++;
      if(cClickCount == 2) {
        // Double-click detected - force sync reset
        if(DEBUG_SERIAL) Serial.println("BUTTON C DOUBLE-CLICK: Force sync reset");
        forceSyncReset();
        cClickCount = 0; // Reset
        return;
      }
    } else {
      cClickCount = 1; // First click
    }
    lastCClick = now;
  }
  
  // Reset click count after timeout
  if(millis() - lastCClick > 500 && cClickCount == 1) {
    cClickCount = 0; // Single click timeout, process as sync reset below
  }
  
  // Button B: Short press = Pattern control (only works in AUTO mode when leader)
  else if(M5.BtnB.wasClicked() && currentMode == AUTO && fsmState == LEADER) {
    if(!freezeActive) {
      freezeActive = true;
      if(DEBUG_SERIAL) Serial.println("Pattern freeze ON");
    } else {
      styleIdx = (styleIdx + 1) % 42;
      if(DEBUG_SERIAL) {
        Serial.printf("Pattern advance → %s [frozen]\n", STYLE_NAMES[styleIdx]);
      }
    }
  }
  
  // Button C: Single click = Manual sync reset (only if not part of double-click)
  if(cClickCount == 0 && millis() - lastCClick > 500 && millis() - lastCClick < 1000 && currentMode == AUTO) {
    if(DEBUG_SERIAL) Serial.println("MANUAL SYNC RESET requested via Button C single click");
    
    // Show reset indication on screen briefly
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_YELLOW);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_BLACK);
    canvas.setCursor(10, 10);
    canvas.print("SYNC RESET");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print("Forcing resync...");
    canvas.pushSprite(0, 0);
    
    // Force sync reset
    forceSyncReset();
    
    delay(1000); // Show message for 1 second
    lastCClick = 0; // Clear to prevent repeat
  }
}

void drawUI(){
  int w = M5.Lcd.width(), h = M5.Lcd.height();
  canvas.fillSprite(TFT_BLACK);
  
  // Set banner color and title based on mode and state
  uint16_t bannerColor = TFT_BLUE;  // Default
  String title;
  
  if(currentMode == OFF) {
    title = "SLEEPING";
    bannerColor = TFT_BLACK; // Dark banner for sleep mode
  } else {
    // Find current brightness level name - but show LOCAL/WIFI status
    String brightnessName;
    for(int i = 0; i < 6; i++) {
      if(globalBrightnessScale == brightnessLevels[i].scale) {
        brightnessName = String(brightnessLevels[i].name);
        break;
      }
    }
    
    // Modify title to show WiFi status without blocking
    if(wifiConnected) {
      title = brightnessName; // Just show brightness when WiFi connected
    } else {
      title = brightnessName; // Keep showing brightness in local mode too
    }
    
    // Set banner color based on networking state
    if(fsmState == LEADER) {
      bannerColor = TFT_ORANGE;     // Leader = Orange
      if(freezeActive) title += " [F]";
    } else if(fsmState == FOLLOWER) {
      bannerColor = TFT_GREEN;      // Follower = Green  
    } else {
      bannerColor = TFT_PURPLE;     // Election = Purple
    }
  }
  
  canvas.fillRect(0, 0, w, 40, bannerColor);
  canvas.setTextSize(2); 
  canvas.setTextColor(TFT_WHITE);
  
  canvas.setCursor((w - canvas.textWidth(title)) / 2, 10);
  canvas.print(title);
  canvas.setTextSize(1); 
  canvas.setTextColor(TFT_WHITE);
  
  // Pattern name display logic
  String sname;
  if(currentMode == OFF) {
    sname = "Press A to wake";
  } else if(currentMode == AUTO && fsmState == FOLLOWER) {
    sname = "Following Leader";  // Don't show pattern name for followers
  } else {
    sname = STYLE_NAMES[styleIdx];
  }
  
  canvas.setCursor((w - canvas.textWidth(sname)) / 2, 42);
  canvas.print(sname);
  
  // Show version in bottom left corner (larger font for readability)
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_DARKGREY);
  String version = "v" + String(FIRMWARE_VERSION);
  canvas.setCursor(2, h - 12);
  canvas.print(version);
  
  // Show connection status in bottom right (larger font for readability)
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_DARKGREY);
  
  if(wifiConnected) {
    // Show IP when WiFi connected
    String ip = WiFi.localIP().toString();
    int lastDot = ip.lastIndexOf('.');
    String shortIP = ip.substring(lastDot);
    canvas.setCursor(w - canvas.textWidth(shortIP) - 2, h - 12);
    canvas.print(shortIP);
  } else {
    // Show LOCAL when no WiFi
    String localText = "LOCAL";
    canvas.setCursor(w - canvas.textWidth(localText) - 2, h - 12);
    canvas.print(localText);
  }
  
  // LED preview bar (only show in AUTO mode)
  if(currentMode == AUTO) {
    int barH = 20, by = (h - barH) / 2; 
    uint8_t bri = FastLED.getBrightness();
    for(int x = 0; x < w && x < NUM_LEDS; x++){
      CRGB c = leds[x];
      if(fsmState == LEADER) c.nscale8_video(bri);
      canvas.drawFastVLine(x, by, barH, canvas.color565(c.r, c.g, c.b));
    }
  }
  
  canvas.pushSprite(0, 0);
  
  // Non-blocking frame rate limiting
  lastUIUpdate = millis();
}

bool shouldUpdateUI() {
  return (millis() - lastUIUpdate >= FRAME_DELAY_MS);
}