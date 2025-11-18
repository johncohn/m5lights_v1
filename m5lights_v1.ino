/// @file    m5lights_v1_simple.ino
/// @brief   Ultra-Simple ESP-NOW LED Sync with 12 Patterns + Music Mode
/// @version 3.3.1
/// @date    2024-10-26
/// @author  John Cohn (adapted from Mark Kriegsman)
///
/// @changelog
/// v3.3.1 (2024-10-26) - Added Leader Broadcast Debug Logging
///   - Leader now logs broadcast activity (once per second summary)
///   - Shows when broadcasts are skipped due to timing
///   - Logs ESP-NOW send failures if they occur
///   - Helps diagnose leader broadcast issues
/// v3.3.0 (2024-10-26) - Added Leader Conflict Prevention & Enhanced Debug Logging
///   - CRITICAL: Nodes now refuse to become leader if another leader exists
///   - Prevents multiple simultaneous leaders (double-red state)
///   - Extensive debug logging for ESP-NOW packet reception
///   - Timestamped debug output for incomplete frame detection
///   - Leader timeout events now show detailed timing information
///   - Helps diagnose why followers aren't reconnecting to leaders
/// v3.2.1 (2024-10-26) - Fixed Rejoin Logic Stuck State
///   - CRITICAL FIX: Followers now immediately exit rejoin mode when receiving leader data
///   - Prevents stuck state where followers stay green despite leader broadcasting
///   - Increased incomplete frame timeout from 500ms to 2000ms (less aggressive)
///   - Followers can now reconnect to leaders without requiring power cycle
/// v3.2.0 (2024-10-26) - Added Incomplete Frame Detection & Auto-Recovery
///   - Followers now detect when receiving packets but not complete frames
///   - Auto-resync if no complete frame received within 500ms (packet loss)
///   - Prevents "stuck following" state where display shows blue but LEDs are stale
///   - Improved reliability during WiFi interference or signal issues
/// v3.1.9 (2024-10-26) - Fixed Leader/Follower Sync Timing
///   - CRITICAL FIX: Leader now broadcasts BEFORE showing LEDs (was backwards!)
///   - Leader: generate pattern → broadcast → wait LEADER_DELAY_MS → show
///   - Followers: receive all packets → show immediately
///   - Both leader and followers now show LEDs at the same time
///   - Calibrated LEADER_DELAY_MS to 50ms for perfect synchronization
///   - Fixed ESP-NOW callback signature for esp32 v3.3.3 compatibility
/// v3.1.8 (2024-10-26) - Fixed Adaptive Tracking
///   - noiseFloor now always adapts (was only updating when audio < noiseFloor)
///   - Both noiseFloor and peakLevel continuously track audio range
///   - Brightness always scales full 1-25 range based on recent audio levels
///   - Talking quietly or loudly both give full brightness variation
/// v3.1.7 (2024-10-26) - Simplified Direct Audio Mapping
///   - Removed all complex adaptive brightness range logic
///   - Direct mapping: audioLevel → brightness (loud=25, quiet=1)
///   - Fast-adapting noiseFloor/peakLevel keep range appropriate
///   - Immediate voice amplitude response
/// v3.1.6 (2024-10-26) - Much More Aggressive Contrast Mapping
///   - Changed dynamic range threshold from 0.30 to 0.10 for full brightness range
///   - Normal talking now produces full 1-25 contrast (was only getting 14-25)
///   - Dramatic bright/dark pulsing with much less sound variation needed
/// v3.1.5 (2024-10-26) - Ultra-Fast Audio Response
///   - Reduced time constants for immediate contrast when talking/music starts
///   - peakLevelSmooth: 0.90 → 0.5 (50% adaptation per frame)
///   - noiseFloorSmooth: 0.95 → 0.7 (30% adaptation per frame)
///   - Lights now react almost instantly to sound dynamics changes
/// v3.1.4 (2024-10-26) - Extended LED Count to 200
///   - Increased NUM_LEDS from 100 to 200
/// v3.1.3 (2024-10-26) - Fixed: Music Brightness Based on Dynamic Range
///   - Changed adaptive brightness to use dynamic range (beat contrast) not absolute level
///   - Low dynamic range (constant noise): lights stay bright (20-25) with small flicker
///   - High dynamic range (music beats): lights use full range (1-25) for dramatic pulsing
/// v3.1.2 (2024-10-26) - Adaptive Music Brightness & Power Optimization
///   - Adaptive brightness range based on absolute sound levels
///   - Quiet environment: lights stay bright (20-25) with subtle modulation
///   - Loud environment: full dynamic range (1-25) with dramatic pulsing
///   - Reduced to 100 LEDs for M5Stick 5V power stability
///   - Aggressive audio normalization for responsive music mode
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
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>

FASTLED_USING_NAMESPACE

// Version info
#define VERSION "3.3.1"

// Hardware config
#define LED_PIN 32
#define NUM_LEDS 200
#define BRIGHTNESS 25  // Adjusted for M5Stick power stability
#define COLOR_ORDER GRB
#define CHIPSET WS2811

// Leader sync delay - adjust to match follower display timing
#define LEADER_DELAY_MS 50  // Delay before leader shows LEDs (ms)

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
unsigned long lastCompleteFrame = 0;  // Last time we received a complete LED frame
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

// Adaptive audio scaling - Ultra-fast response for immediate contrast
float noiseFloor = 0.01f;          // Moving average of quiet ambient sound
float peakLevel = 0.1f;            // Moving average of loud sound peaks
float noiseFloorSmooth = 0.7f;     // Ultra-fast adaptation (30% new value per frame)
float peakLevelSmooth = 0.5f;      // Ultra-fast decay (50% new value per frame)

// Pattern control
bool autoAdvancePatterns = true;   // Whether patterns auto-advance
unsigned long lastPatternChange = 0;

// ESP-NOW rejoin logic
unsigned long lastRejoinScan = 0;
bool rejoinMode = false;
uint8_t rejoinAttempts = 0;

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
#define LEADER_TIMEOUT_MS 8000  // Increased from 3s to 8s for robustness
#define REJOIN_SCAN_INTERVAL_MS 15000  // Scan for leaders every 15 seconds
#define COMPLETE_FRAME_TIMEOUT_MS 2000  // Max time between complete frames before resync

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
  fire, lightningStorm, plasmaField, meteorShower, auroraWaves
};

const char* patternNames[] = {
  "Rainbow", "Chase", "Juggle", "Rainbow+Glitter", "Confetti", "BPM",
  "Fire", "Lightning", "Plasma", "Meteors", "Aurora"
};

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// ESP-NOW callbacks
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW: Send OK");
  } else {
    Serial.println("ESP-NOW: Send FAIL");
  }
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t *incomingData, int len) {
  unsigned long now = millis();
  Serial.print("[");
  Serial.print(now);
  Serial.print("ms] ESP-NOW RX: ");
  Serial.print(len);
  Serial.print(" bytes");

  // Only process if we're a follower (not a leader)
  if (currentMode == MODE_NORMAL_LEADER || currentMode == MODE_MUSIC_LEADER) {
    Serial.println(" - IGNORED (I'm a leader)");
    return;
  }
  Serial.println();

  // Verify message size
  if (len != sizeof(LEDSync)) {
    Serial.print("ESP-NOW: WRONG SIZE, expected ");
    Serial.print(sizeof(LEDSync));
    Serial.print(", got ");
    Serial.println(len);
    return;
  }

  LEDSync receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  Serial.print("  Packet: seq=");
  Serial.print(receivedData.sequenceNum);
  Serial.print(", start=");
  Serial.print(receivedData.startIndex);
  Serial.print(", count=");
  Serial.println(receivedData.count);

  // Update leader activity
  bool wasActive = leaderDataActive;
  lastLeaderMessage = millis();
  leaderDataActive = true;

  if (!wasActive) {
    Serial.println("  >>> LEADER DETECTED - now following <<<");
  }

  // Exit rejoin mode immediately when receiving valid leader data
  if (rejoinMode) {
    rejoinMode = false;
    rejoinAttempts = 0;
    Serial.println("  >>> EXITING REJOIN MODE <<<");
  }

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
    lastCompleteFrame = millis();  // Mark successful complete frame reception
    Serial.println("  ✓ COMPLETE FRAME - LEDs updated");
  } else {
    Serial.println("  ... waiting for more packets");
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
  static unsigned long lastBroadcastLog = 0;
  static uint32_t broadcastCount = 0;
  unsigned long now = millis();

  const int LEDS_PER_PACKET = 49;  // Reduced by 1 to fit brightness byte
  static uint8_t sequenceNum = 0;

  LEDSync message;
  message.sequenceNum = sequenceNum++;

  broadcastCount++;

  // Log every 20th broadcast (once per second at 50ms intervals)
  if (now - lastBroadcastLog > 1000) {
    Serial.print("[LEADER TX] Broadcasting frame #");
    Serial.print(broadcastCount);
    Serial.print(", seq=");
    Serial.println(message.sequenceNum);
    lastBroadcastLog = now;
  }

  int successCount = 0;
  int failCount = 0;

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

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
      successCount++;
    } else {
      failCount++;
    }
  }

  // Log if any packets failed
  if (failCount > 0) {
    Serial.print("!!! BROADCAST FAILED: ");
    Serial.print(failCount);
    Serial.print(" packets failed, ");
    Serial.print(successCount);
    Serial.println(" succeeded");
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
  
  // Always adapt BOTH noiseFloor and peakLevel to track current audio range
  // This ensures brightness always scales from min to max based on recent audio
  noiseFloor = noiseFloor * noiseFloorSmooth + audioLevel * (1.0f - noiseFloorSmooth);
  peakLevel = peakLevel * peakLevelSmooth + audioLevel * (1.0f - peakLevelSmooth);

  // Simple direct mapping: loud voice = bright (25), quiet voice = dark (1)
  // Fast-adapting noiseFloor and peakLevel keep the range appropriate
  float range = peakLevel - noiseFloor;
  if (range < 0.01f) range = 0.01f;  // Prevent division by zero

  float normalizedLevel = (audioLevel - noiseFloor) / range;
  normalizedLevel = constrain(normalizedLevel, 0.0f, 1.0f);

  // Direct 1-25 mapping - no adaptive complexity
  musicBrightness = 1 + (uint8_t)(normalizedLevel * 24);
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

        // LEADER CONFLICT PREVENTION: Check if another leader already exists
        if (currentMode == MODE_NORMAL || currentMode == MODE_MUSIC) {
          if (leaderDataActive || (now - lastLeaderMessage < LEADER_TIMEOUT_MS)) {
            Serial.println("*** BLOCKED: Another leader is already active! ***");
            Serial.print("leaderDataActive: ");
            Serial.print(leaderDataActive);
            Serial.print(", Time since last leader msg: ");
            Serial.println(now - lastLeaderMessage);
            buttonState = BTN_LONG_TRIGGERED;
            lastAction = now;
            break;  // Prevent becoming leader
          }
        }

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

// Enhanced leader timeout with rejoin logic
void checkLeaderTimeout() {
  unsigned long now = millis();

  // Check for incomplete frame reception: receiving packets but not complete frames
  if (leaderDataActive && lastCompleteFrame > 0 &&
      (now - lastCompleteFrame > COMPLETE_FRAME_TIMEOUT_MS) &&
      (now - lastLeaderMessage < LEADER_TIMEOUT_MS)) {
    // We're receiving ESP-NOW data but haven't gotten a complete frame recently
    // This indicates packet loss - force resync
    Serial.println("\n!!! WARNING: INCOMPLETE FRAMES DETECTED !!!");
    Serial.print("  Time since last complete frame: ");
    Serial.print(now - lastCompleteFrame);
    Serial.println("ms");
    Serial.print("  Time since last leader msg: ");
    Serial.print(now - lastLeaderMessage);
    Serial.println("ms");
    Serial.println("  >>> FORCING RESYNC <<<\n");
    leaderDataActive = false;
    rejoinMode = true;
    rejoinAttempts = 0;
    lastRejoinScan = now;
    lastCompleteFrame = 0;  // Reset for next sync attempt
  }

  if (leaderDataActive && (now - lastLeaderMessage > LEADER_TIMEOUT_MS)) {
    Serial.println("\n!!! LEADER TIMEOUT !!!");
    Serial.print("  Time since last leader msg: ");
    Serial.print(now - lastLeaderMessage);
    Serial.println("ms");
    Serial.println("  >>> ENTERING REJOIN MODE <<<\n");
    leaderDataActive = false;
    rejoinMode = true;
    rejoinAttempts = 0;
    lastRejoinScan = now;
  }
  
  // Active rejoin scanning for dropped followers
  if (rejoinMode && (now - lastRejoinScan > REJOIN_SCAN_INTERVAL_MS)) {
    if (rejoinAttempts < 5) {  // Try 5 times before giving up
      Serial.print("Rejoin attempt ");
      Serial.println(rejoinAttempts + 1);
      
      // Reset ESP-NOW to try reconnecting
      esp_now_deinit();
      delay(100);
      setupESPNOW();
      
      rejoinAttempts++;
      lastRejoinScan = now;
      
      if (rejoinAttempts >= 5) {
        rejoinMode = false;
        Serial.println("Rejoin attempts exhausted - staying standalone");
      }
    }
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
  
  // Initialize watchdog timer (30 second timeout)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  
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
  // Reset watchdog timer
  esp_task_wdt_reset();
  
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

    // If leader: broadcast FIRST, then wait for followers to receive/process, then show
    if (currentMode == MODE_MUSIC_LEADER) {
      static unsigned long lastSkipLog = 0;
      if (currentTime - lastBroadcast > BROADCAST_INTERVAL_MS) {
        broadcastLEDData();
        lastBroadcast = currentTime;
        delay(LEADER_DELAY_MS);  // Wait for followers to receive and process
      } else if (currentTime - lastSkipLog > 5000) {
        Serial.print("[LEADER] Skipping broadcast (too soon): ");
        Serial.print(currentTime - lastBroadcast);
        Serial.println("ms since last");
        lastSkipLog = currentTime;
      }
    }

    FastLED.show();
  } else {
    // Normal mode - just run pattern
    FastLED.setBrightness(BRIGHTNESS);
    gPatterns[gCurrentPatternNumber]();

    // If leader: broadcast FIRST, then wait for followers to receive/process, then show
    if (currentMode == MODE_NORMAL_LEADER) {
      static unsigned long lastSkipLog = 0;
      if (currentTime - lastBroadcast > BROADCAST_INTERVAL_MS) {
        broadcastLEDData();
        lastBroadcast = currentTime;
        delay(LEADER_DELAY_MS);  // Wait for followers to receive and process
      } else if (currentTime - lastSkipLog > 5000) {
        Serial.print("[LEADER] Skipping broadcast (too soon): ");
        Serial.print(currentTime - lastBroadcast);
        Serial.println("ms since last");
        lastSkipLog = currentTime;
      }
    }

    FastLED.show();
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
  // Ultra-dynamic lava with faster, more organic movement
  uint32_t time = millis();
  
  // Faster, more chaotic flow speeds for organic movement
  uint16_t mainFlow = time / 15;      // Faster primary flow (was 25)
  uint16_t bubbleFlow = time / 7;     // Much faster bubbling (was 12)
  uint16_t waveFlow = time / 35;      // Faster waves (was 65)
  uint16_t slowChurn = time / 90;     // Faster deep churning (was 180)
  uint16_t fastTurbulence = time / 4; // New: very fast surface turbulence
  
  // Dynamic color evolution with more variation
  uint8_t colorShift = (time / 60) & 255;  // Faster color cycling (was 100)
  uint8_t colorVariation = (time / 40) & 255;  // Additional color complexity
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // Create highly complex flowing heat patterns with more layers
    uint8_t bubble = inoise8(i * 35 + bubbleFlow, bubbleFlow / 3);     // Tighter, faster bubbles
    uint8_t wave = inoise8(i * 55, waveFlow);                          // More frequent waves
    uint8_t flow = inoise8(i * 85, mainFlow);                          // Tighter main flow
    uint8_t churn = inoise8(i * 150, slowChurn);                       // More active churning
    uint8_t turbulence = inoise8(i * 25 + fastTurbulence, fastTurbulence / 2); // Surface chaos
    
    // Multiple directional flows for organic movement
    uint16_t flowPos1 = i + (mainFlow / 6);  // Faster primary flow
    uint16_t flowPos2 = i - (bubbleFlow / 10);  // Counter-flow
    uint8_t directionalHeat1 = inoise8(flowPos1 * 45, time / 25);
    uint8_t directionalHeat2 = inoise8(flowPos2 * 65, time / 35);
    
    // Complex heat mixing with more dynamic interaction
    uint16_t totalHeat = (bubble * 4 + wave * 3 + flow * 5 + churn * 3 + 
                         turbulence * 6 + directionalHeat1 * 4 + directionalHeat2 * 2) / 10;
    totalHeat = constrain(totalHeat, 0, 255);
    
    // Enhanced dynamic color temperature with spatial variation
    uint8_t tempShift = sin8(colorShift + i * 12) / 6;  // More color variation
    uint8_t spatialVar = sin8(colorVariation + i * 20) / 4;  // Additional spatial variation
    
    // Richer lava color palette with more reds, oranges, and yellows
    CRGB color;
    uint8_t adjustedHeat = qadd8(qadd8(totalHeat, tempShift), spatialVar);
    
    if (adjustedHeat < 50) {
      // Deep dark red to rich red
      uint8_t red = 20 + (adjustedHeat * 4);
      color = CRGB(red, adjustedHeat / 8, 0);
    } else if (adjustedHeat < 100) {
      // Rich reds to warm orange-reds
      uint8_t progress = adjustedHeat - 50;
      uint8_t red = 220 + (progress / 2);
      uint8_t green = progress * 2;
      color = CRGB(red, green, 0);
    } else if (adjustedHeat < 150) {
      // Warm oranges to bright oranges
      uint8_t progress = adjustedHeat - 100;
      uint8_t red = 255;
      uint8_t green = 100 + (progress * 2);
      uint8_t blue = progress / 8;
      color = CRGB(red, green, blue);
    } else if (adjustedHeat < 200) {
      // Bright orange to golden yellow
      uint8_t progress = adjustedHeat - 150;
      uint8_t red = 255;
      uint8_t green = 200 + progress;
      uint8_t blue = progress / 3;
      color = CRGB(red, green, blue);
    } else {
      // Golden yellow to bright white-yellow
      uint8_t progress = adjustedHeat - 200;
      uint8_t red = 255;
      uint8_t green = 255;
      uint8_t blue = 50 + (progress * 3);
      color = CRGB(red, green, blue);
    }
    
    // Enhanced flickering with multiple frequency layers
    uint8_t flicker1 = sin8(time / 5 + i * 20) / 20;   // Fast flicker
    uint8_t flicker2 = sin8(time / 12 + i * 8) / 32;   // Medium flicker
    uint8_t brightness = 220 + flicker1 + flicker2;
    color.nscale8(brightness);
    
    leds[i] = color;
  }
}