#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ui.cpp"
#include "ui.h"

const char* MODE_NAMES[MODE_COUNT] = {"AUTO","GO","QUIET"};

void initUI(){
  M5.Lcd.setRotation(3);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
}

void loadControls(){
  prefs.begin("npref", false);
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
    FastLED.setBrightness(map(getBright(), 0, 9, 0, 255));
  }
}

void handleButtons(){
  uint32_t now = millis();
  
  // Button A: Mode cycling - FIXED the cycle order
  if(M5.BtnA.wasClicked()){
    if(currentMode == AUTO) {
      currentMode = GO;
    } else if(currentMode == GO) {
      currentMode = QUIET;  
    } else { // QUIET -> AUTO
      currentMode = AUTO; 
      fsmState = FOLLOWER; 
      lastRecvMillis = now; 
    }
    FastLED.setBrightness(map(getBright(), 0, 9, 0, 255));
    freezeActive = false;
    Serial.printf("BtnA: Mode → %s (freeze reset)\n", MODE_NAMES[currentMode]);
  }

  // Button B: Freeze/Advance
  if(M5.BtnB.wasClicked()){
    bool canUseButton = false;
    
    if(currentMode == AUTO && fsmState == LEADER) {
      canUseButton = true;
    } else if(currentMode == GO || currentMode == QUIET) {
      canUseButton = true;
    }
    
    if(canUseButton) {
      if(!freezeActive){
        freezeActive = true;
        Serial.printf("BtnB: %s freeze ON\n", 
          currentMode == AUTO ? "AUTO-LEADER" : MODE_NAMES[currentMode]);
      } else {
        styleIdx = (styleIdx + 1) % 22;
        Serial.printf("BtnB: %s advance → pattern %d (%s) [still frozen]\n", 
          currentMode == AUTO ? "AUTO-LEADER" : MODE_NAMES[currentMode],
          styleIdx, STYLE_NAMES[styleIdx]);
      }
    } else {
      Serial.printf("BtnB: ignored (mode=%s, fsm=%s)\n", 
        MODE_NAMES[currentMode], 
        fsmState==FOLLOWER?"FOLLOWER":fsmState==LEADER?"LEADER":"ELECT");
    }
  }
}

void drawUI(){
  int w = M5.Lcd.width(), h = M5.Lcd.height();
  canvas.fillSprite(TFT_BLACK);
  canvas.fillRect(0, 0, w, 40, TFT_BLUE);
  canvas.setTextSize(2); 
  canvas.setTextColor(TFT_WHITE);
  
  String title;
  if(currentMode == AUTO) {
    title = String("Auto");
    if(fsmState == FOLLOWER) title += " - follow";      // lowercase
    else if(fsmState == LEADER) {
      title += " - lead";                               // lowercase
      if(freezeActive) title += " [F]";
    }
    else title += " - Elect";
  } else {
    title = MODE_NAMES[currentMode];
    if(freezeActive) title += " [F]";
  }
  
  canvas.setCursor((w - canvas.textWidth(title)) / 2, 10);
  canvas.print(title);
  canvas.setTextSize(1); 
  canvas.setTextColor(TFT_WHITE);
  String sname = STYLE_NAMES[styleIdx];
  canvas.setCursor((w - canvas.textWidth(sname)) / 2, 42);
  canvas.print(sname);
  
  int barH = 20, by = (h - barH) / 2; 
  uint8_t bri = FastLED.getBrightness();
  for(int x = 0; x < w && x < NUM_LEDS; x++){
    CRGB c = leds[x];
    if(currentMode == AUTO && fsmState == LEADER) c.nscale8_video(bri);
    canvas.drawFastVLine(x, by, barH, canvas.color565(c.r, c.g, c.b));
  }
  
  canvas.pushSprite(0, 0);
  delay(FRAME_DELAY_MS);
}