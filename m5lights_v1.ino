/// @file    m5lights_v1_simple.ino
/// @brief   Ultra-Simple ESP-NOW LED Sync with 12 Patterns + Music Mode
/// @version 3.0.7
/// @date    2024-10-26
/// @author  John Cohn (adapted from Mark Kriegsman)
///
/// @changelog
/// v3.0.0 (2024-10-26) - Clean Ultra-Simple ESP-NOW Implementation
///   - Complete rewrite with simple 4-mode system
///   - Normal, Music, Normal Leader, Music Leader modes
///   - Button: Short press = Normal↔Music, Long press = Become Leader
///   - Direct LED data sync (153 bytes per packet, 50 LEDs)
///   - Zero processing delay on followers
///   - All 12 patterns from v2.6 + working music system
///   - High contrast music mode (2-96 brightness range)

#include <M5StickCPlus2.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

FASTLED_USING_NAMESPACE

// Version info
#define VERSION "3.0.7"

// Hardware config
#define LED_PIN 32
#define NUM_LEDS 200
#define BRIGHTNESS 30
#define COLOR_ORDER GRB
#define CHIPSET WS2811

CRGB leds[NUM_LEDS];

// Ultra-Simple Mode System  
enum NodeMode {
  MODE_NORMAL,        // Standalone normal patterns, no sync
  MODE_MUSIC,         // Standalone music-reactive patterns, no sync  
  MODE_NORMAL_LEADER, // Normal patterns + broadcast LED data
  MODE_MUSIC_LEADER   // Music patterns + broadcast LED data
};

// Simple LED data sync message
struct LEDSync {
  uint8_t startIndex;       // LED start position (0-199)
  uint8_t count;           // Number of LEDs in this packet (1-50)
  uint8_t sequenceNum;     // Packet sequence for ordering
  uint8_t brightness;      // Current brightness (for audio sync)
  uint8_t rgbData[149];    // RGB data (max 49 LEDs = 147 bytes, reduced by 1 for brightness)
};

// Global variables
NodeMode currentMode = MODE_NORMAL;
unsigned long lastModeSwitch = 0;
bool leaderDataActive = false;
unsigned long lastLeaderMessage = 0;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Pattern globals
uint8_t gCurrentPatternNumber = 0;
uint8_t gHue = 0;

// Audio system variables (from working v2.6 implementation)
float soundMin = 1.0f;
float soundMax = 0.0f;
float musicLevel = 0.0f;
float audioLevel = 0.0f;
bool beatDetected = false;
bool prevAbove = false;
uint32_t beatTimes[50];
uint8_t beatCount = 0;
uint32_t lastBpmMillis = 0;
bool audioDetected = true;
uint8_t musicBrightness = BRIGHTNESS;

// Adaptive audio scaling
float noiseFloor = 0.01f;          // Moving average of quiet ambient sound
float peakLevel = 0.1f;            // Moving average of loud sound peaks
float noiseFloorSmooth = 0.998f;   // Slow adaptation for noise floor
float peakLevelSmooth = 0.99f;     // Faster adaptation for peaks

// Pattern control
bool autoAdvancePatterns = true;   // Whether patterns auto-advance
unsigned long lastPatternChange = 0;

// Audio configuration
static constexpr size_t MIC_BUF_LEN = 240;
static constexpr int MIC_SR = 44100;
static constexpr float SMOOTH = 0.995f;
static constexpr uint32_t BPM_WINDOW = 5000;

// Button handling
volatile bool buttonStateChanged = false;
volatile bool buttonCurrentState = false;
volatile unsigned long buttonLastInterrupt = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBroadcast = 0;

// Timing constants
#define LONG_PRESS_TIME_MS 1500
#define BROADCAST_INTERVAL_MS 50
#define LEADER_TIMEOUT_MS 3000

// Pattern function declarations
void rainbow();
void doChase();
void juggle();
void rainbowWithGlitter();
void confetti();
void bpm();
void fire();
void lightningStorm();
void plasmaField();
void meteorShower();
void auroraWaves();
void lavaFlow();

// Pattern list and names
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { 
  rainbow, doChase, juggle, rainbowWithGlitter, confetti, bpm,
  fire, lightningStorm, plasmaField, meteorShower, auroraWaves, lavaFlow
};

const char* patternNames[] = {
  "Rainbow", "Chase", "Juggle", "Rainbow+Glitter", "Confetti", "BPM",
  "Fire", "Lightning", "Plasma", "Meteors", "Aurora", "Lava"
};

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// ESP-NOW callbacks
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW: Send OK");
  } else {
    Serial.println("ESP-NOW: Send FAIL");
  }
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t *incomingData, int len) {
  Serial.print("ESP-NOW: Received ");
  Serial.print(len);
  Serial.println(" bytes");
  
  // Only process if we're a follower (not a leader)
  if (currentMode == MODE_NORMAL_LEADER || currentMode == MODE_MUSIC_LEADER) {
    Serial.println("ESP-NOW: Ignoring (I'm a leader)");
    return;
  }
  
  // Verify message size
  if (len != sizeof(LEDSync)) {
    Serial.print("ESP-NOW: Wrong size, expected ");
    Serial.println(sizeof(LEDSync));
    return;
  }
  
  LEDSync receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  // Update leader activity
  lastLeaderMessage = millis();
  leaderDataActive = true;
  
  // Apply brightness from leader (for audio sync)
  FastLED.setBrightness(receivedData.brightness);
  
  // Apply LED data directly
  for (int i = 0; i < receivedData.count && i < 49; i++) {
    int ledIndex = receivedData.startIndex + i;
    if (ledIndex < NUM_LEDS) {
      int dataIndex = i * 3;
      leds[ledIndex].r = receivedData.rgbData[dataIndex];
      leds[ledIndex].g = receivedData.rgbData[dataIndex + 1];
      leds[ledIndex].b = receivedData.rgbData[dataIndex + 2];
    }
  }
  
  // Show LEDs when we receive the last packet
  if (receivedData.startIndex + receivedData.count >= NUM_LEDS) {
    FastLED.show();
  }
}

// ESP-NOW setup
void setupESPNOW() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceived);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ESP-NOW peer add failed");
  } else {
    Serial.println("ESP-NOW setup complete");
  }
}

// Broadcast LED data (for leader modes)
void broadcastLEDData() {
  const int LEDS_PER_PACKET = 49;  // Reduced by 1 to fit brightness byte
  static uint8_t sequenceNum = 0;
  
  LEDSync message;
  message.sequenceNum = sequenceNum++;
  
  for (int startIdx = 0; startIdx < NUM_LEDS; startIdx += LEDS_PER_PACKET) {
    message.startIndex = startIdx;
    message.count = min(LEDS_PER_PACKET, NUM_LEDS - startIdx);
    message.brightness = FastLED.getBrightness();  // Include current brightness
    
    // Pack RGB data
    for (int i = 0; i < message.count; i++) {
      int ledIdx = startIdx + i;
      int dataIdx = i * 3;
      message.rgbData[dataIdx] = leds[ledIdx].r;
      message.rgbData[dataIdx + 1] = leds[ledIdx].g;
      message.rgbData[dataIdx + 2] = leds[ledIdx].b;
    }
    
    esp_now_send(broadcastAddress, (uint8_t*)&message, sizeof(message));
  }
}

// Audio system (working implementation from v2.6)
void initAudio() {
  M5.Mic.begin(); 
  M5.Mic.setSampleRate(MIC_SR);
  lastBpmMillis = millis();
  Serial.println("Audio initialized");
}

void detectAudioFrame() {
  static int16_t micBuf[MIC_BUF_LEN];
  if (!M5.Mic.record(micBuf, MIC_BUF_LEN)) return;
  
  long sum = 0; 
  for (auto &v : micBuf) sum += abs(v);
  float raw = float(sum) / MIC_BUF_LEN / 32767.0f;
  
  soundMin = min(raw, SMOOTH * soundMin + (1 - SMOOTH) * raw);
  soundMax = max(raw, SMOOTH * soundMax + (1 - SMOOTH) * raw);
  
  // Adaptive sensitivity
  float dynamicRange = soundMax - soundMin;
  const float MIN_DYNAMIC_RANGE = 0.08f;
  const float HIGH_VOLUME_THRESHOLD = 0.7f;
  
  float adaptedMin = soundMin;
  float adaptedMax = soundMax;
  float beatThreshold = 0.6f;
  
  bool highVolumeEnvironment = (soundMin > HIGH_VOLUME_THRESHOLD) || (dynamicRange < MIN_DYNAMIC_RANGE);
  
  if (highVolumeEnvironment) {
    if (dynamicRange < MIN_DYNAMIC_RANGE) {
      float expansion = (MIN_DYNAMIC_RANGE - dynamicRange) * 0.5f;
      adaptedMin = max(0.0f, soundMin - expansion);
      adaptedMax = min(1.0f, soundMax + expansion);
    }
    beatThreshold = 0.35f;
  }
  
  musicLevel = constrain((raw - adaptedMin) / (adaptedMax - adaptedMin + 1e-6f), 0.0f, 1.0f);
  audioLevel = musicLevel;
  
  bool above = (musicLevel > beatThreshold);
  if (above && !prevAbove) {
    uint32_t t = millis();
    if (beatCount < 50) {
      beatTimes[beatCount++] = t;
    } else { 
      memmove(beatTimes, beatTimes + 1, 49 * sizeof(uint32_t)); 
      beatTimes[49] = t; 
    }
    beatDetected = true;
  } else if (!above) {
    beatDetected = false;
  }
  prevAbove = above;
}

void updateBPM() {
  uint32_t now = millis();
  if (now - lastBpmMillis >= BPM_WINDOW) {
    int cnt = 0; 
    uint32_t cutoff = now - BPM_WINDOW;
    for (int i = 0; i < beatCount; i++) {
      if (beatTimes[i] >= cutoff) cnt++;
    }
    
    float bpm = cnt * (60000.0f / float(BPM_WINDOW));
    audioDetected = (cnt >= 4 && bpm >= 30.0f && bpm <= 300.0f);
    lastBpmMillis += BPM_WINDOW; 
    beatCount = 0;
  }
}

void updateAudioLevel() {
  detectAudioFrame();
  updateBPM();
  
  // Adaptive noise floor and peak tracking
  if (audioLevel < noiseFloor || noiseFloor == 0.01f) {
    noiseFloor = noiseFloor * noiseFloorSmooth + audioLevel * (1.0f - noiseFloorSmooth);
  }
  
  if (audioLevel > peakLevel) {
    peakLevel = peakLevel * peakLevelSmooth + audioLevel * (1.0f - peakLevelSmooth);
  }
  
  // Ensure we have a reasonable dynamic range
  float dynamicRange = peakLevel - noiseFloor;
  if (dynamicRange < 0.02f) {
    dynamicRange = 0.02f;  // Minimum range to prevent division by zero
  }
  
  // Map audio level from [noiseFloor, peakLevel] to [1, 96] brightness
  float normalizedLevel = (audioLevel - noiseFloor) / dynamicRange;
  normalizedLevel = constrain(normalizedLevel, 0.0f, 1.0f);
  
  // Full contrast: 1 (barely on) to 96 (full brightness)
  uint8_t minBrightness = 1;
  uint8_t maxBrightness = 96;
  
  musicBrightness = minBrightness + (uint8_t)(normalizedLevel * (maxBrightness - minBrightness));
  FastLED.setBrightness(musicBrightness);
}

// Button interrupt handler
void IRAM_ATTR buttonInterrupt() {
  unsigned long currentTime = millis();
  if (currentTime - buttonLastInterrupt > 50) {
    buttonCurrentState = digitalRead(37);
    buttonStateChanged = true;
    buttonLastInterrupt = currentTime;
  }
}

void setupButtonInterrupt() {
  pinMode(37, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(37), buttonInterrupt, CHANGE);
}

// Mode switching functions
void switchToNormalMode() {
  currentMode = MODE_NORMAL;
  lastModeSwitch = millis();
  leaderDataActive = false;
  Serial.println("*** NORMAL MODE ***");
}

void switchToMusicMode() {
  currentMode = MODE_MUSIC;
  lastModeSwitch = millis();
  leaderDataActive = false;
  Serial.println("*** MUSIC MODE ***");
}

void switchToNormalLeaderMode() {
  currentMode = MODE_NORMAL_LEADER;
  lastModeSwitch = millis();
  leaderDataActive = false;
  Serial.println("*** NORMAL LEADER MODE ***");
}

void switchToMusicLeaderMode() {
  currentMode = MODE_MUSIC_LEADER;
  lastModeSwitch = millis();
  leaderDataActive = false;
  Serial.println("*** MUSIC LEADER MODE ***");
}

// Button handling
void handleButtons() {
  static enum { BTN_IDLE, BTN_PRESSED, BTN_LONG_TRIGGERED, BTN_COOLDOWN } buttonState = BTN_IDLE;
  static unsigned long buttonPressTime = 0;
  static unsigned long lastAction = 0;
  
  unsigned long now = millis();
  bool currentPressed = !digitalRead(37);
  
  if (now - lastAction < 50) return;
  
  switch (buttonState) {
    case BTN_IDLE:
      if (currentPressed) {
        buttonState = BTN_PRESSED;
        buttonPressTime = now;
        lastAction = now;
      }
      break;
      
    case BTN_PRESSED:
      if (!currentPressed) {
        // Short press: Toggle Normal ↔ Music (preserve leader state)
        Serial.print("Short press from mode: ");
        Serial.println(currentMode);
        if (currentMode == MODE_NORMAL) {
          Serial.println("Switching NORMAL -> MUSIC");
          switchToMusicMode();
        } else if (currentMode == MODE_MUSIC) {
          Serial.println("Switching MUSIC -> NORMAL");
          switchToNormalMode();
        } else if (currentMode == MODE_NORMAL_LEADER) {
          Serial.println("Switching NORMAL_LEADER -> MUSIC_LEADER");
          switchToMusicLeaderMode();
        } else if (currentMode == MODE_MUSIC_LEADER) {
          Serial.println("Switching MUSIC_LEADER -> NORMAL_LEADER");
          switchToNormalLeaderMode();
        }
        buttonState = BTN_COOLDOWN;
        lastAction = now;
        
      } else if (now - buttonPressTime >= LONG_PRESS_TIME_MS) {
        // Long press: Toggle leader status of current mode
        Serial.print("Long press from mode: ");
        Serial.println(currentMode);
        if (currentMode == MODE_NORMAL) {
          Serial.println("Switching NORMAL -> NORMAL_LEADER");
          switchToNormalLeaderMode();
        } else if (currentMode == MODE_MUSIC) {
          Serial.println("Switching MUSIC -> MUSIC_LEADER");
          switchToMusicLeaderMode();
        } else if (currentMode == MODE_NORMAL_LEADER) {
          Serial.println("Switching NORMAL_LEADER -> NORMAL");
          switchToNormalMode();
        } else if (currentMode == MODE_MUSIC_LEADER) {
          Serial.println("Switching MUSIC_LEADER -> MUSIC");
          switchToMusicMode();
        }
        buttonState = BTN_LONG_TRIGGERED;
        lastAction = now;
      }
      break;
      
    case BTN_LONG_TRIGGERED:
      if (!currentPressed) {
        buttonState = BTN_COOLDOWN;
        lastAction = now;
      }
      break;
      
    case BTN_COOLDOWN:
      if (!currentPressed && now - lastAction > 300) {
        buttonState = BTN_IDLE;
      }
      break;
  }
}

// B Button handling for pattern control
void handlePatternButtons() {
  // Only allow pattern control when not following (not in blue mode)
  if (leaderDataActive && (currentMode == MODE_NORMAL || currentMode == MODE_MUSIC)) {
    return; // In follower mode (blue), disable pattern control
  }
  
  static unsigned long bBtnPressTime = 0;
  static bool bBtnWasPressed = false;
  static bool bLongHandled = false;
  
  bool bBtnPressed = M5.BtnB.isPressed();
  
  if (bBtnPressed && !bBtnWasPressed) {
    // B button just pressed
    bBtnPressTime = millis();
    bLongHandled = false;
  } else if (!bBtnPressed && bBtnWasPressed) {
    // B button just released
    unsigned long pressDuration = millis() - bBtnPressTime;
    
    if (!bLongHandled && pressDuration < 1000) {
      // Short press: advance to next pattern
      nextPattern();
      lastPatternChange = millis(); // Reset auto-advance timer
      Serial.print("Manual pattern change to: ");
      Serial.println(gCurrentPatternNumber);
    }
  } else if (bBtnPressed && !bLongHandled && (millis() - bBtnPressTime >= 1000)) {
    // Long press: toggle auto-advance
    autoAdvancePatterns = !autoAdvancePatterns;
    bLongHandled = true;
    Serial.print("Auto-advance patterns: ");
    Serial.println(autoAdvancePatterns ? "ON" : "OFF");
  }
  
  bBtnWasPressed = bBtnPressed;
}

// Display update
void updateDisplay() {
  uint16_t backgroundColor;
  uint16_t textColor = WHITE;
  
  // Check if we're following (receiving leader data)
  if (leaderDataActive && (currentMode == MODE_NORMAL || currentMode == MODE_MUSIC)) {
    backgroundColor = BLUE;  // Blue background when following
    textColor = WHITE;
  } else {
    switch (currentMode) {
      case MODE_NORMAL:
        backgroundColor = GREEN;
        textColor = BLACK;
        break;
      case MODE_MUSIC:
        backgroundColor = PURPLE;
        textColor = WHITE;
        break;
      case MODE_NORMAL_LEADER:
        backgroundColor = ORANGE;
        textColor = BLACK;
        break;
      case MODE_MUSIC_LEADER:
        backgroundColor = RED;
        textColor = WHITE;
        break;
      default:
        backgroundColor = BLACK;
        break;
    }
  }
  
  M5.Display.fillScreen(backgroundColor);
  M5.Display.setTextColor(textColor);
  M5.Display.setTextSize(1);
  
  M5.Display.drawString("Simple Sync", 10, 10);
  M5.Display.drawString("v" + String(VERSION), 10, 20);
  
  String modeStr = "";
  switch (currentMode) {
    case MODE_NORMAL: modeStr = "NORMAL"; break;
    case MODE_MUSIC: modeStr = "MUSIC"; break;
    case MODE_NORMAL_LEADER: modeStr = "NORM LEAD"; break;
    case MODE_MUSIC_LEADER: modeStr = "MUSIC LEAD"; break;
  }
  M5.Display.drawString("Mode: " + modeStr, 10, 35);
  
  // Pattern info
  if (leaderDataActive && (currentMode == MODE_NORMAL || currentMode == MODE_MUSIC)) {
    M5.Display.drawString("Following...", 10, 50);
  } else if (currentMode == MODE_MUSIC || currentMode == MODE_MUSIC_LEADER) {
    String patternDisplay = String(gCurrentPatternNumber) + ": " + String(patternNames[gCurrentPatternNumber]);
    M5.Display.drawString(patternDisplay, 10, 50);
    M5.Display.drawString("Audio: " + String((int)(audioLevel * 100)) + "%", 10, 65);
    M5.Display.drawString("Beat: " + String(beatDetected ? "YES" : "NO"), 10, 80);
  } else {
    String patternDisplay = String(gCurrentPatternNumber) + ": " + String(patternNames[gCurrentPatternNumber]);
    M5.Display.drawString(patternDisplay, 10, 50);
  }
}

void nextPattern() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

// Check for leader timeout
void checkLeaderTimeout() {
  if (leaderDataActive && (millis() - lastLeaderMessage > LEADER_TIMEOUT_MS)) {
    leaderDataActive = false;
    Serial.println("Leader timeout");
  }
}

void setup() {
  // Initialize M5StickC Plus 2
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  
  Serial.begin(115200);
  delay(1000);
  
  setupButtonInterrupt();
  initAudio();
  setupESPNOW();
  
  // Initialize FastLED
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  
  randomSeed(micros());
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  // Initialize pattern timing
  lastPatternChange = millis();
  
  updateDisplay();
  
  Serial.println("Simple LED Sync v" + String(VERSION) + " ready!");
  Serial.println("Short press: Normal ↔ Music");
  Serial.println("Long press: Become Leader");
}

void loop() {
  M5.update();
  handleButtons();
  handlePatternButtons();
  
  unsigned long currentTime = millis();
  
  // Check for leader timeout
  checkLeaderTimeout();
  
  // If we're following a leader, don't run our own patterns
  if (leaderDataActive && (currentMode == MODE_NORMAL || currentMode == MODE_MUSIC)) {
    // Just update display and return - LEDs controlled by leader
    if (currentTime - lastDisplayUpdate > 200) {
      updateDisplay();
      lastDisplayUpdate = currentTime;
    }
    return;
  }
  
  // Run patterns based on mode
  if (currentMode == MODE_MUSIC || currentMode == MODE_MUSIC_LEADER) {
    // Music mode - update audio and run pattern
    updateAudioLevel();
    gPatterns[gCurrentPatternNumber]();
    FastLED.show();
  } else {
    // Normal mode - just run pattern
    FastLED.setBrightness(BRIGHTNESS);
    gPatterns[gCurrentPatternNumber]();
    FastLED.show();
  }
  
  // Broadcast LED data if we're a leader
  if ((currentMode == MODE_NORMAL_LEADER || currentMode == MODE_MUSIC_LEADER) && 
      (currentTime - lastBroadcast > BROADCAST_INTERVAL_MS)) {
    broadcastLEDData();
    lastBroadcast = currentTime;
  }
  
  // Update display periodically
  if (currentTime - lastDisplayUpdate > 200) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Auto-advance patterns every 15 seconds in all modes (if enabled)
  if (autoAdvancePatterns && currentTime - lastPatternChange > 15000) {
    nextPattern();
    lastPatternChange = currentTime;
  }
  
  // Increment hue for patterns
  EVERY_N_MILLISECONDS(20) { gHue++; }
  
  yield();
}

// ===== PATTERN IMPLEMENTATIONS =====
// All 12 patterns from v2.6

void rainbow() {
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {
  rainbow();
  if (random8() < 80) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

void juggle() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void bpm() {
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void doChase() {
  static uint32_t last = 0;
  static uint16_t pos = 0;
  static uint8_t hue = 0;
  uint32_t now = millis();
  if (now - last < 100) return;
  last = now;
  pos = (pos + 1) % NUM_LEDS;
  fadeToBlackBy(leds, NUM_LEDS, 50);
  for (int i = 0; i < NUM_LEDS; i += 20) {
    for (int t = 0; t < 10; t++) {
      int idx = (pos + i + NUM_LEDS - t) % NUM_LEDS;
      leds[idx] |= CHSV(hue + i, 255, map(t, 0, 9, 255, 50));
    }
  }
  hue++;
}

void fire() {
  static uint8_t heat[NUM_LEDS/2];
  int half = NUM_LEDS/2;
  uint8_t cooling = 55;
  uint8_t sparking = 120;
  
  for (int i = 0; i < half; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / half) + 2));
  }
  
  for (int k = half - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  if (random8() < sparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  for (int j = 0; j < half; j++) {
    CRGB color = ColorFromPalette(HeatColors_p, scale8(heat[j], 240));
    leds[half + j] = color;
    leds[half - 1 - j] = color;
  }
}

void lightningStorm() {
  static unsigned long lastStrike = 0;
  unsigned long currentTime = millis();
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(220);
    leds[i] += CRGB(0, 0, 15);
  }
  
  if (currentTime - lastStrike > random16(100, 2000)) {
    lastStrike = currentTime;
    int strikePos = random16(NUM_LEDS - 30);
    int strikeLength = random8(10, 25);
    
    for (int i = strikePos; i < strikePos + strikeLength && i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 255, 255);
    }
    
    if (random8() < 150) {
      int secondaryPos = strikePos + random8(-5, 5);
      int secondaryLen = random8(5, 12);
      for (int i = secondaryPos; i < secondaryPos + secondaryLen && i >= 0 && i < NUM_LEDS; i++) {
        leds[i] = CRGB(200, 200, 255);
      }
    }
  }
}

void plasmaField() {
  static uint16_t plasmaTime = 0;
  static uint8_t plasmaHue = 0;
  
  plasmaTime += 2;
  plasmaHue += 1;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t wave1 = sin8(plasmaTime/4 + i * 8);
    uint8_t wave2 = sin8(plasmaTime/3 + i * 6 + 85);
    uint8_t wave3 = sin8(plasmaTime/5 + i * 4 + 170);
    uint8_t wave4 = sin8(plasmaTime/7 + i * 12);
    
    uint8_t combined = (wave1/4 + wave2/3 + wave3/3 + wave4/6);
    uint8_t hue = plasmaHue + combined/2;
    uint8_t brightness = combined + sin8(plasmaTime/8 + i)/4;
    
    leds[i] = CHSV(hue, 240, brightness);
  }
}

void meteorShower() {
  struct Meteor {
    int16_t pos;
    uint8_t hue;
    uint8_t size;
    int8_t speed;
  };
  static Meteor meteors[6];
  static bool initialized = false;
  static unsigned long lastUpdate = 0;
  
  if (!initialized) {
    for (int i = 0; i < 6; i++) {
      meteors[i] = {(int16_t)(-random8(20)), (uint8_t)random8(), (uint8_t)(3 + random8(4)), (int8_t)(1 + random8(2))};
    }
    initialized = true;
  }
  
  fadeToBlackBy(leds, NUM_LEDS, 60);
  
  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();
    
    for (int m = 0; m < 6; m++) {
      Meteor &meteor = meteors[m];
      
      for (int t = 0; t < meteor.size; t++) {
        int16_t trailPos = meteor.pos - t;
        if (trailPos >= 0 && trailPos < NUM_LEDS) {
          uint8_t brightness = map(t, 0, meteor.size-1, 255, 50);
          leds[trailPos] += CHSV(meteor.hue, 200, brightness);
        }
      }
      
      meteor.pos += meteor.speed;
      
      if (meteor.pos >= NUM_LEDS + meteor.size) {
        meteor.pos = -meteor.size;
        meteor.hue = random8();
        meteor.size = 3 + random8(4);
        meteor.speed = 1 + random8(2);
      }
    }
  }
}

void auroraWaves() {
  static uint16_t wave1_pos = 0, wave2_pos = 0, wave3_pos = 0;
  static uint8_t auroraHue = 96;
  
  wave1_pos += 2;
  wave2_pos += 3;
  wave3_pos += 1;
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t wave1 = sin8(wave1_pos + i * 4);
    uint8_t wave2 = sin8(wave2_pos + i * 6 + 85);
    uint8_t wave3 = sin8(wave3_pos + i * 2 + 170);
    
    uint8_t hue1 = auroraHue + sin8(i * 8)/8;
    uint8_t hue2 = auroraHue + 40;
    uint8_t hue3 = auroraHue + 80;
    
    if (wave1 > 100) {
      leds[i] += CHSV(hue1, 255, (wave1-100)*2);
    }
    if (wave2 > 120) {
      leds[i] += CHSV(hue2, 240, (wave2-120)*2);
    }
    if (wave3 > 140) {
      leds[i] += CHSV(hue3, 200, (wave3-140)*3);
    }
  }
  
  if (random8() < 2) {
    auroraHue += random8(5) - 2;
    auroraHue = constrain(auroraHue, 80, 140);
  }
}

void lavaFlow() {
  // Multiple moving heat layers for flowing lava effect
  uint32_t time = millis();
  uint16_t flow1 = time / 30;   // Fast flow
  uint16_t flow2 = time / 80;   // Medium flow  
  uint16_t flow3 = time / 150;  // Slow flow
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // Create flowing noise layers with different speeds and scales
    uint8_t heat1 = inoise8(i * 30, flow1);        // Fast small bubbles
    uint8_t heat2 = inoise8(i * 80, flow2);        // Medium streams
    uint8_t heat3 = inoise8(i * 150, flow3);       // Large slow flows
    
    // Combine heat sources with weighted mixing
    uint16_t totalHeat = (heat1 * 2 + heat2 * 3 + heat3 * 1) / 3;
    totalHeat = constrain(totalHeat, 0, 255);
    
    // Create flowing lava color mapping
    CRGB color;
    if (totalHeat < 80) {
      // Cool/dark lava - deep red to black
      color = CRGB(totalHeat, 0, 0);
    } else if (totalHeat < 160) {
      // Warm lava - red to orange
      uint8_t progress = totalHeat - 80;
      color = CRGB(80 + progress * 2, progress, 0);
    } else if (totalHeat < 200) {
      // Hot lava - orange to yellow  
      uint8_t progress = totalHeat - 160;
      color = CRGB(255, 160 + progress * 2, progress / 2);
    } else {
      // Molten white-hot lava
      uint8_t progress = totalHeat - 200;
      color = CRGB(255, 255, progress * 4);
    }
    
    leds[i] = color;
  }
}