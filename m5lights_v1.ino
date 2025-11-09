/// @file    m5lights_v1.ino
/// @brief   FastLED demo reel adapted for M5StickC Plus 2
/// @version 2.6.0
/// @date    2024-10-26
/// @author  John Cohn (adapted from Mark Kriegsman)
/// @example m5lights_v1.ino
///
/// @changelog
/// v2.6.0 (2024-10-26) - Expanded Pattern Library from oldplayalights
///   - Added 6 new patterns for total of 12 patterns (doubled from 6)
///   - 🔥 Fire pattern with realistic heat simulation and mirrored flames
///   - ⚡ Lightning Storm with dramatic white strikes on stormy background
///   - 🌌 Plasma Field with multiple overlapping sine waves (perfect for music)
///   - ☄️ Meteor Shower with trailing meteors and varying speeds
///   - 🌈 Aurora Waves with flowing green-blue aurora effects
///   - 🌋 Lava Flow with organic noise-based molten lava colors
///   - All new patterns optimized for music-reactive mode
/// v2.5.1 (2024-10-26) - High Contrast Music Mode Brightness
///   - Increased brightness range from 10-30 to 2-96 for dramatic effect
///   - Music mode now goes from almost off to full brightness
///   - Maximum contrast based on current sound range
/// v2.5.0 (2024-10-26) - Working Microphone Integration from oldplayalights
///   - Fixed music mode crashes by using proper M5.Mic API calls
///   - Integrated working microphone implementation from oldplayalights folder
///   - Proper M5.Mic.begin() and M5.Mic.setSampleRate() initialization
///   - Correct M5.Mic.record() buffer usage with 44100Hz sample rate
///   - Real beat detection with BPM calculation and adaptive thresholds
///   - Removed problematic PDM and analog fallback code
/// v2.3.0 (2024-10-26) - Revolutionary Pattern State Synchronization
///   - COMPLETE REDESIGN: Send pattern parameters instead of LED data (750 bytes -> 20 bytes)
///   - Synchronized pattern execution for perfect real-time LED matching
///   - Faster leader detection (10s -> 5s) and recovery (immediate)
///   - Single-message synchronization with zero chunking delays
///   - Followers execute same patterns locally with leader's timing parameters
/// v2.2.1 (2024-10-26) - Ultra-Low Latency ESP-NOW & Button Fixes
///   - Fixed inconsistent button responses requiring multiple presses
///   - Dramatically reduced ESP-NOW latency with optimized chunking
///   - Increased leader timeout to prevent frequent follower dropout
///   - Removed delays between message chunks for real-time synchronization
///   - Simplified button state machine for more reliable detection
/// v2.2.0 (2024-10-26) - Non-blocking Architecture & Interrupt-based Buttons
///   - Removed ALL blocking delay() calls - converted to non-blocking timing
///   - Implemented interrupt-based button handling for 100% reliable detection
///   - Fixed LED synchronization issues preventing proper follower display
///   - Enhanced ESP-NOW timing without interference from blocking operations
///   - Improved overall responsiveness and real-time performance
/// v2.1.5 (2024-10-26) - Fixed ESP-NOW Synchronization Issues
///   - Reduced leader timeout from 10s to 2s for faster response
///   - Reduced leader check interval from 10s to 1s for better detection
///   - Fixed follower LED control - now properly displays leader patterns only
///   - Added automatic leader re-detection and rejoin capability
///   - Improved button debug output with clearer state information
///   - Enhanced follow mode to completely stop local pattern execution
/// v2.1.4 (2024-10-24) - Robust Button State Machine
///   - Implemented proper button state machine with 4 states (IDLE, PRESSED, LONG_TRIGGERED, COOLDOWN)
///   - Fixed inconsistent mode switching behavior between devices
///   - Added 300ms debouncing and 500ms cooldown to prevent rapid triggers
///   - Much clearer debug output showing exact button states
///   - Reliable short press (pattern/normal) and long press (lead) functionality
/// v2.1.3 (2024-10-24) - Fixed Button Detection Logic
///   - Fixed M5.BtnA.isPressed() detection for long press functionality
///   - Hybrid approach using manual state tracking with M5 button detection
///   - Proper button press/release cycle detection
///   - Long press now works reliably for Lead mode switching
/// v2.1.2 (2024-10-24) - Simplified Button Logic Fix
///   - Completely rewritten button handling using M5's built-in methods
///   - Fixed freezing issues during button press/release
///   - Removed complex progress bar display that caused conflicts
///   - Much more reliable long press detection (2s)
///   - Better debouncing to prevent multiple triggers
/// v2.1.1 (2024-10-24) - Critical ESP-NOW Stability Fix
///   - Fixed ESP-NOW initialization crashes and "Peer interface is invalid" errors
///   - Added comprehensive error handling for WiFi and ESP-NOW setup
///   - Improved ESP-NOW callback safety with null pointer checks
///   - Enhanced broadcast error handling to prevent device lockup
///   - Added MAC address debugging output
///   - Device now continues operation even if ESP-NOW setup fails
/// v2.1.0 (2024-10-24) - Enhanced Button Controls & Visual Feedback
///   - Fixed long press button detection with improved responsiveness
///   - Added color-coded display backgrounds: Green=Normal, Orange=Lead, Blue=Follow
///   - Reduced long press time to 2 seconds for better UX
///   - Added visual progress bar and feedback for long press
///   - Enhanced debug output for button detection
///   - Better text contrast on colored backgrounds
/// v2.0.0 (2024-10-24) - ESP-NOW Synchronization System
///   - Added three-mode operation: Normal, Lead, Follow
///   - ESP-NOW wireless synchronization between multiple devices
///   - Lead mode broadcasts LED states to all followers
///   - Follow mode receives and displays leader's LED patterns
///   - Automatic leader discovery and timeout handling
///   - Button controls: long press -> Lead, short press -> Normal/Next Pattern
///   - Updated display to show current mode and status
///   - 10-second timeout for follow mode, auto-fallback to normal
/// v1.0.0 (2024-10-24) - Initial release
///   - Ported FastLED demo from ESP32-S3 to M5StickC Plus 2
///   - Support for 250 WS2811/WS2812 LEDs on G32 pin
///   - 6 animation patterns with auto-cycling and manual control
///   - M5StickC Plus 2 display integration with pattern info
///   - Button A for manual pattern switching
///   - Auto-cycle every 5 seconds

#include <M5StickCPlus2.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

FASTLED_USING_NAMESPACE

// Version info
#define VERSION "2.3.3"
#define BUILD_DATE "2025-10-24"

// Animation speeds
#define FORWARD 0
#define BACKWARD 1
#define SLOW 250
#define MEDIUM 50
#define FAST 5

// FastLED "100-lines-of-code" demo reel, showing just a few
// of the kinds of animation patterns you can quickly and easily
// compose using FastLED.
//
// This example also shows one easy way to define multiple
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014
// Adapted for M5StickC Plus 2 by John Cohn


#define DATA_PIN 32        // G32 on Grove connector (white wire)
//#define CLK_PIN   4
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 200       // Reduced count for lower power consumption
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 30      // Reduced brightness for lower current draw
#define FRAMES_PER_SECOND 400

// ESP-NOW Synchronization System
enum NodeMode {
  MODE_NORMAL,    // Standalone mode - runs patterns locally
  MODE_LEAD,      // Lead mode - runs patterns and broadcasts LED states
  MODE_FOLLOW,    // Follow mode - receives and displays LED states from leader
  MODE_MUSIC      // Music mode - patterns react to microphone input
};

// NEW: Pattern state synchronization message (much smaller!)
struct PatternSyncMessage {
  uint8_t messageType;      // 0x01 = pattern state, 0x02 = heartbeat
  uint8_t patternNumber;    // Current pattern being displayed
  uint32_t patternStartTime; // When this pattern started (for timing sync)
  uint32_t currentTime;     // Leader's current millis() for sync
  uint8_t globalHue;        // Current gHue value for color sync
  uint32_t randomSeed;      // Random seed for synchronized randomness
  uint16_t frameNumber;     // Frame counter for smooth sync
  uint8_t reserved[8];      // Reserved for future pattern-specific params
  uint8_t checksum;         // Simple checksum for data integrity
};

// Global synchronization variables
NodeMode currentMode = MODE_NORMAL;
unsigned long lastLeaderMessage = 0;
unsigned long lastModeSwitch = 0;
unsigned long lastLeaderCheck = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool leaderDetected = false;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast to all

// Pattern and animation globals (moved here for ESP-NOW callback access)
uint8_t gCurrentPatternNumber = 0;  // Index number of which pattern is current  
uint8_t gHue = 0;                   // rotating "base color" used by many of the patterns

// Music mode variables - using working oldplayalights implementation
float soundMin = 1.0f;
float soundMax = 0.0f;
float musicLevel = 0.0f;
float audioLevel = 0.0f;             // Current audio level (0.0 to 1.0) - for compatibility
bool beatDetected = false;           // True when beat detected
bool prevAbove = false;
uint32_t beatTimes[50];
uint8_t beatCount = 0;
uint32_t lastBpmMillis = 0;
bool audioDetected = true;
uint8_t musicBrightness = BRIGHTNESS; // Dynamic brightness based on audio

// Audio configuration (matching working implementation)
static constexpr size_t MIC_BUF_LEN = 240;
static constexpr int MIC_SR = 44100;
static constexpr float SMOOTH = 0.995f;
static constexpr uint32_t BPM_WINDOW = 5000;

// NEW: Follower synchronization variables
unsigned long leaderPatternStartTime = 0;
unsigned long timeOffset = 0; // Difference between leader and follower clocks
uint32_t syncedRandomSeed = 0;
uint16_t lastFrameNumber = 0;
bool patternSyncActive = false;

// Interrupt-based button handling  
volatile bool buttonStateChanged = false;
volatile bool buttonCurrentState = false;
volatile unsigned long buttonLastInterrupt = 0;

// Non-blocking timing variables
unsigned long lastPatternUpdate = 0;
unsigned long lastBroadcast = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastDisplayUpdate = 0;

// Timing constants
#define LEADER_TIMEOUT_MS 5000       // 5 seconds to prevent frequent dropout
#define LEADER_CHECK_INTERVAL_MS 50     // Check for leader every 50ms (much faster detection)
#define LONG_PRESS_TIME_MS 1500      // 1.5 seconds for long press (faster response)
#define SYNC_SEND_INTERVAL_MS 50     // Send pattern state every 50ms (20 FPS)
#define LEADER_HEARTBEAT_INTERVAL_MS 50   // Send heartbeat every 50ms for real-time sync

// ESP-NOW callback functions
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Called when data is sent via ESP-NOW
  static unsigned long lastSendLog = 0;
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW send failed");
  } else if (millis() - lastSendLog > 2000) {
    Serial.println("ESP-NOW message sent successfully");
    lastSendLog = millis();
  }
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t *incomingData, int len) {
  // Add debug output to verify callback is triggered
  static unsigned long lastReceiveLog = 0;
  if (millis() - lastReceiveLog > 1000) {
    Serial.print("ESP-NOW received: len=");
    Serial.print(len);
    Serial.print(", expected=");
    Serial.println(sizeof(PatternSyncMessage));
    lastReceiveLog = millis();
  }
  
  // Safety checks
  if (!recv_info || !incomingData) {
    Serial.println("ESP-NOW: NULL data received");
    return;
  }
  
  if (len != sizeof(PatternSyncMessage)) {
    Serial.print("ESP-NOW: Wrong message size. Got ");
    Serial.print(len);
    Serial.print(", expected ");
    Serial.println(sizeof(PatternSyncMessage));
    return;
  }
  
  PatternSyncMessage receivedMessage;
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
  
  // Debug: Show message contents
  Serial.print("MSG: type=");
  Serial.print(receivedMessage.messageType);
  Serial.print(", pattern=");
  Serial.print(receivedMessage.patternNumber);
  
  // Verify checksum - calculate over all bytes except checksum field
  uint8_t calculatedChecksum = 0;
  uint8_t* data = (uint8_t*)&receivedMessage;
  for (int i = 0; i < sizeof(PatternSyncMessage) - 1; i++) {
    calculatedChecksum ^= data[i];
  }
  
  Serial.print(", checksum: calc=");
  Serial.print(calculatedChecksum);
  Serial.print(", recv=");
  Serial.println(receivedMessage.checksum);
  
  if (calculatedChecksum != receivedMessage.checksum) {
    Serial.println("CHECKSUM FAILED!");
    return; // Invalid checksum
  }
  
  Serial.println("Checksum OK - updating leader detection");
  lastLeaderMessage = millis();
  leaderDetected = true;
  
  // If we're in normal mode and detect a leader, switch to follow mode
  if (currentMode == MODE_NORMAL) {
    Serial.print("In NORMAL mode, switching to FOLLOW. Current mode=");
    Serial.println(currentMode);
    switchToFollowMode();
    Serial.print("After switch, current mode=");
    Serial.println(currentMode);
  } else {
    Serial.print("Not in NORMAL mode. Current mode=");
    Serial.println(currentMode);
  }
  
  // Handle pattern synchronization in follow mode
  if (currentMode == MODE_FOLLOW) {
    Serial.println("Processing message in FOLLOW mode");
    if (receivedMessage.messageType == 0x01) {
      // Pattern state message - synchronize our pattern execution
      Serial.print("Applying pattern sync: pattern=");
      Serial.print(receivedMessage.patternNumber);
      Serial.print(", frame=");
      Serial.println(receivedMessage.frameNumber);
      // Update synchronization variables from leader
      if (gCurrentPatternNumber != receivedMessage.patternNumber) {
        Serial.print("PATTERN CHANGE from leader: ");
        Serial.print(gCurrentPatternNumber);
        Serial.print(" -> ");
        Serial.println(receivedMessage.patternNumber);
        gCurrentPatternNumber = receivedMessage.patternNumber;
      }
      gHue = receivedMessage.globalHue;
      leaderPatternStartTime = receivedMessage.patternStartTime;
      timeOffset = millis() - receivedMessage.currentTime;
      
      // Sync random seed for consistent randomness
      if (syncedRandomSeed != receivedMessage.randomSeed) {
        syncedRandomSeed = receivedMessage.randomSeed;
        random16_set_seed(syncedRandomSeed);
      }
      
      // Enable pattern synchronization
      patternSyncActive = true;
      lastFrameNumber = receivedMessage.frameNumber;
    } else if (receivedMessage.messageType == 0x02) {
      // Heartbeat message - keeps us in follow mode
      Serial.print("Heartbeat from leader, pattern=");
      Serial.println(receivedMessage.patternNumber);
      // Update pattern from heartbeat too in case we missed a pattern message
      if (gCurrentPatternNumber != receivedMessage.patternNumber) {
        Serial.print("PATTERN CHANGE from heartbeat: ");
        Serial.print(gCurrentPatternNumber);
        Serial.print(" -> ");
        Serial.println(receivedMessage.patternNumber);
        gCurrentPatternNumber = receivedMessage.patternNumber;
      }
    }
  } else {
    Serial.println("NOT in follow mode, ignoring message processing");
  }
}

void setupESPNOW() {
  // Set device as a Wi-Fi Station and disconnect from any AP
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Force specific WiFi channel for ESP-NOW compatibility
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  
  // Non-blocking delay replacement
  unsigned long wifiSetupStart = millis();
  while (millis() - wifiSetupStart < 100) {
    yield(); // Allow other tasks
  }
  
  // Print MAC address and channel for debugging
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  wifi_second_chan_t secondChan;
  uint8_t channel;
  esp_wifi_get_channel(&channel, &secondChan);
  Serial.print("WiFi Channel: ");
  Serial.println(channel);
  
  // Initialize ESP-NOW
  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.print("Error initializing ESP-NOW: ");
    Serial.println(initResult);
    Serial.println("Continuing without ESP-NOW...");
    return;
  }
  
  // Register callbacks
  esp_err_t sendResult = esp_now_register_send_cb(onDataSent);
  esp_err_t recvResult = esp_now_register_recv_cb(onDataReceived);
  
  if (sendResult != ESP_OK) {
    Serial.print("Send callback registration failed: ");
    Serial.println(sendResult);
  }
  
  if (recvResult != ESP_OK) {
    Serial.print("Receive callback registration failed: ");
    Serial.println(recvResult);
  }
  
  // Add broadcast peer with better error handling
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1; // Match the WiFi channel we set
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  
  esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
  if (addPeerResult != ESP_OK) {
    Serial.print("Failed to add broadcast peer: ");
    Serial.println(addPeerResult);
    
    // Try to remove existing peer first, then re-add
    esp_now_del_peer(broadcastAddress);
    addPeerResult = esp_now_add_peer(&peerInfo);
    if (addPeerResult == ESP_OK) {
      Serial.println("Broadcast peer added after removal");
    } else {
      Serial.print("Still failed after removal: ");
      Serial.println(addPeerResult);
    }
  } else {
    Serial.println("Broadcast peer added successfully");
  }
  
  Serial.println("ESP-NOW setup completed");
}

// Button interrupt handler
void IRAM_ATTR buttonInterrupt() {
  unsigned long currentTime = millis();
  // Debounce the interrupt (ignore if within 50ms of last interrupt)
  if (currentTime - buttonLastInterrupt > 50) {
    buttonCurrentState = digitalRead(37); // M5StickC Plus 2 button A pin
    buttonStateChanged = true;
    buttonLastInterrupt = currentTime;
  }
}

void setupButtonInterrupt() {
  // Configure button pin for interrupt
  pinMode(37, INPUT_PULLUP); // M5StickC Plus 2 button A
  attachInterrupt(digitalPinToInterrupt(37), buttonInterrupt, CHANGE);
  Serial.println("Button interrupt setup completed");
}

void setup() {
  // Initialize M5StickC Plus 2
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  updateDisplay();
  
  Serial.begin(115200);
  // Non-blocking initialization wait
  unsigned long serialStart = millis();
  while (millis() - serialStart < 1000) {
    yield(); // Allow other processes
  }

  // Setup interrupt-based button handling
  setupButtonInterrupt();
  
  // Initialize audio system
  initAudio();
  
  // Initialize ESP-NOW
  setupESPNOW();

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  Serial.println("M5 Lights v" + String(VERSION) + " - Normal Mode Ready");
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
SimplePatternList gPatterns = { 
  rainbow, doChase, juggle, rainbowWithGlitter, confetti, bpm,
  fire, lightningStorm, plasmaField, meteorShower, auroraWaves, lavaFlow
};

// Moved to top for ESP-NOW callback access

void loop() {
  M5.update();
  
  // Handle interrupt-based button logic for mode switching
  handleButtonLogic();
  
  // Handle mode-specific behavior
  handleCurrentMode();
  
  // Update the display if needed (non-blocking)
  if (millis() - lastDisplayUpdate > 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  // Small yield to allow other processes
  yield();
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Mode switching and helper functions
void switchToNormalMode() {
  if (currentMode != MODE_NORMAL) {
    currentMode = MODE_NORMAL;
    lastModeSwitch = millis();
    leaderDetected = false;
    patternSyncActive = false;
    Serial.println("*** SWITCHED TO NORMAL MODE ***");
    updateDisplay();
  }
}

void switchToLeadMode() {
  if (currentMode != MODE_LEAD) {
    currentMode = MODE_LEAD;
    lastModeSwitch = millis();
    leaderDetected = false;
    patternSyncActive = false;
    Serial.println("*** SWITCHED TO LEAD MODE ***");
    updateDisplay();
  }
}

void switchToFollowMode() {
  if (currentMode != MODE_FOLLOW) {
    currentMode = MODE_FOLLOW;
    lastModeSwitch = millis();
    Serial.println("*** SWITCHED TO FOLLOW MODE ***");
    updateDisplay();
  }
}

void switchToMusicMode() {
  if (currentMode != MODE_MUSIC) {
    currentMode = MODE_MUSIC;
    lastModeSwitch = millis();
    leaderDetected = false;
    patternSyncActive = false;
    Serial.println("*** SWITCHED TO MUSIC MODE ***");
    updateDisplay();
  }
}

void updateDisplay() {
  // Set background color based on mode
  uint16_t backgroundColor;
  uint16_t textColor = WHITE;
  
  switch (currentMode) {
    case MODE_NORMAL:
      backgroundColor = GREEN;
      textColor = BLACK;
      break;
    case MODE_LEAD:
      backgroundColor = ORANGE;
      textColor = BLACK;
      break;
    case MODE_FOLLOW:
      backgroundColor = BLUE;
      textColor = BLACK;
      break;
    case MODE_MUSIC:
      backgroundColor = PURPLE;
      textColor = WHITE;
      break;
    default:
      backgroundColor = BLACK;
      textColor = WHITE;
      break;
  }
  
  M5.Display.fillScreen(backgroundColor);
  M5.Display.setTextColor(textColor);
  M5.Display.setTextSize(1);
  
  // Title and version
  M5.Display.drawString("FastLED Sync", 10, 10);
  M5.Display.drawString("v" + String(VERSION), 10, 20);
  
  // Current mode with emphasis
  String modeStr = "";
  switch (currentMode) {
    case MODE_NORMAL: modeStr = "NORMAL"; break;
    case MODE_LEAD: modeStr = "LEAD"; break;
    case MODE_FOLLOW: modeStr = "FOLLOW"; break;
    case MODE_MUSIC: modeStr = "MUSIC"; break;
  }
  M5.Display.drawString("Mode: " + modeStr, 10, 35);
  
  // Pattern info (for normal and lead modes)
  if (currentMode == MODE_FOLLOW) {
    M5.Display.drawString("Following...", 10, 50);
  } else if (currentMode == MODE_MUSIC) {
    M5.Display.drawString("Audio: " + String((int)(audioLevel * 100)) + "%", 10, 50);
    M5.Display.drawString("Beats: " + String(beatCount), 10, 65);
    M5.Display.drawString("Beat: " + String(beatDetected ? "YES" : "NO"), 10, 80);
  } else {
    M5.Display.drawString("Pattern: " + String(gCurrentPatternNumber), 10, 50);
  }
  
  // Additional status
  M5.Display.drawString("G32->LEDs", 10, 65);
}

void handleButtonLogic() {
  // Simplified button state machine
  static enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_LONG_TRIGGERED,
    BTN_COOLDOWN
  } buttonState = BTN_IDLE;
  
  static unsigned long buttonPressTime = 0;
  static unsigned long lastAction = 0;
  
  unsigned long now = millis();
  bool currentPressed = !digitalRead(37); // Read button directly (inverted due to pull-up)
  
  // Debounce check
  if (now - lastAction < 50) {
    return;
  }
  
  switch (buttonState) {
    case BTN_IDLE:
      if (currentPressed) {
        buttonState = BTN_PRESSED;
        buttonPressTime = now;
        Serial.println("=== Button pressed ===");
        lastAction = now;
      }
      break;
      
    case BTN_PRESSED:
      if (!currentPressed) {
        // Button released - short press
        unsigned long duration = now - buttonPressTime;
        Serial.print("=== Short press (");
        Serial.print(duration);
        Serial.print("ms) ===");
        
        if (currentMode == MODE_NORMAL) {
          switchToMusicMode();
          Serial.println(" -> MUSIC mode");
        } else if (currentMode == MODE_MUSIC) {
          nextPattern();
          switchToNormalMode();
          Serial.print(" -> NORMAL mode, Pattern: ");
          Serial.println(gCurrentPatternNumber);
        } else {
          switchToNormalMode();
          Serial.println(" -> NORMAL mode");
        }
        updateDisplay();
        
        buttonState = BTN_COOLDOWN;
        lastAction = now;
        
      } else if (now - buttonPressTime >= LONG_PRESS_TIME_MS) {
        // Long press triggered
        Serial.println("=== Long press -> LEAD mode ===");
        switchToLeadMode();
        updateDisplay();
        
        buttonState = BTN_LONG_TRIGGERED;
        lastAction = now;
      }
      break;
      
    case BTN_LONG_TRIGGERED:
      if (!currentPressed) {
        Serial.println("=== Long press released ===");
        buttonState = BTN_COOLDOWN;
        lastAction = now;
      }
      break;
      
    case BTN_COOLDOWN:
      if (!currentPressed && now - lastAction > 300) {
        buttonState = BTN_IDLE;
        Serial.println("=== Button ready ===");
      }
      break;
  }
}

void handleCurrentMode() {
  unsigned long currentTime = millis();
  
  switch (currentMode) {
    case MODE_NORMAL:
      handleNormalMode(currentTime);
      break;
      
    case MODE_LEAD:
      handleLeadMode(currentTime);
      break;
      
    case MODE_FOLLOW:
      handleFollowMode(currentTime);
      break;
      
    case MODE_MUSIC:
      handleMusicMode(currentTime);
      break;
  }
}

void handleNormalMode(unsigned long currentTime) {
  // Run patterns locally
  runLocalPatterns();
  
  // Frequently check for leader (every 500ms now)
  if (currentTime - lastLeaderCheck > LEADER_CHECK_INTERVAL_MS) {
    lastLeaderCheck = currentTime;
    // Reset leader detection flag to allow fresh detection
    if (!leaderDetected) {
      Serial.print("Normal mode: Checking for leader... Last msg: ");
      Serial.print(currentTime - lastLeaderMessage);
      Serial.println("ms ago");
      
      // Send a test heartbeat to verify ESP-NOW is working
      static unsigned long lastTestBroadcast = 0;
      if (currentTime - lastTestBroadcast > 5000) { // Every 5 seconds
        Serial.println("Sending test heartbeat...");
        broadcastHeartbeat();
        lastTestBroadcast = currentTime;
      }
    }
  }
}

void handleLeadMode(unsigned long currentTime) {
  // Run patterns locally (non-blocking)
  runLocalPatterns();
  
  // Broadcast pattern state frequently (using global timing variable)
  if (currentTime - lastBroadcast > SYNC_SEND_INTERVAL_MS) {
    broadcastPatternState();
    lastBroadcast = currentTime;
  }
  
  // Send periodic heartbeat to maintain connection (using global timing variable)
  if (currentTime - lastHeartbeat > LEADER_HEARTBEAT_INTERVAL_MS) {
    broadcastHeartbeat();
    lastHeartbeat = currentTime;
  }
}

void handleFollowMode(unsigned long currentTime) {
  // Check for leader timeout
  if (currentTime - lastLeaderMessage > LEADER_TIMEOUT_MS) {
    Serial.println("Leader timeout - switching to normal mode");
    switchToNormalMode();
    leaderDetected = false;
    patternSyncActive = false;
    return;
  }
  
  // NEW: In follow mode, run the SAME patterns as leader with synchronized timing
  if (patternSyncActive) {
    runSynchronizedPatterns();
  } else {
    // Fallback to showing something while waiting for sync
    static unsigned long lastFallbackUpdate = 0;
    if (currentTime - lastFallbackUpdate > 100) {
      fill_solid(leds, NUM_LEDS, CRGB::Blue); // Blue while waiting for sync
      FastLED.show();
      lastFallbackUpdate = currentTime;
    }
  }
}

void handleMusicMode(unsigned long currentTime) {
  // Note: ESP-NOW stays active to avoid crash issues
  // Just run patterns without network sync in music mode
  
  // Sample microphone and update audio level
  updateAudioLevel();
  
  // Run patterns with music-reactive brightness
  runMusicReactivePatterns();
  
  // Update display periodically
  static unsigned long lastDisplayUpdate = 0;
  if (currentTime - lastDisplayUpdate > 200) { // Update every 200ms
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
}

void initAudio() {
  M5.Mic.begin(); 
  M5.Mic.setSampleRate(MIC_SR);
  lastBpmMillis = millis();
  Serial.println("Audio initialized with M5.Mic");
}

void detectAudioFrame() {
  static int16_t micBuf[MIC_BUF_LEN];
  if (!M5.Mic.record(micBuf, MIC_BUF_LEN)) return;
  
  long sum = 0; 
  for (auto &v : micBuf) sum += abs(v);
  float raw = float(sum) / MIC_BUF_LEN / 32767.0f;
  
  soundMin = min(raw, SMOOTH * soundMin + (1 - SMOOTH) * raw);
  soundMax = max(raw, SMOOTH * soundMax + (1 - SMOOTH) * raw);
  
  // Adaptive sensitivity for high volume environments
  float dynamicRange = soundMax - soundMin;
  const float MIN_DYNAMIC_RANGE = 0.08f;
  const float HIGH_VOLUME_THRESHOLD = 0.7f;
  
  float adaptedMin = soundMin;
  float adaptedMax = soundMax;
  float beatThreshold = 0.6f;
  
  // Detect high volume saturation scenario
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
  audioLevel = musicLevel; // For compatibility with existing code
  
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
  // Call the working audio detection functions
  detectAudioFrame();
  updateBPM();
  
  // Calculate music-reactive brightness with maximum contrast
  // From almost off (2) to full brightness (96) for dramatic effect
  uint8_t minBrightness = 2;   // Almost off but still visible
  uint8_t maxBrightness = 96;  // Full brightness for maximum contrast
  
  musicBrightness = minBrightness + (uint8_t)(audioLevel * (maxBrightness - minBrightness));
  
  // Apply the brightness to FastLED
  FastLED.setBrightness(musicBrightness);
  
  // Debug output (rate limited)
  static unsigned long lastAudioDebug = 0;
  if (millis() - lastAudioDebug > 1000 && Serial) { // Every 1000ms
    lastAudioDebug = millis();
    Serial.printf("Audio: level=%.3f, beats=%d, brightness=%d\n", 
      audioLevel, beatCount, musicBrightness);
  }
}

void runMusicReactivePatterns() {
  unsigned long currentTime = millis();
  
  // Non-blocking frame rate limiting
  if (currentTime - lastPatternUpdate < 20) {
    return; // Too soon for next frame (50 FPS)
  }
  lastPatternUpdate = currentTime;
  
  // Run the current pattern with music-reactive modifications
  if (gCurrentPatternNumber < ARRAY_SIZE(gPatterns)) {
    gPatterns[gCurrentPatternNumber]();
  }
  
  // Apply beat-responsive effects
  if (beatDetected) {
    // Flash effect on beat - temporarily boost brightness
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeLightBy(50); // Brighten by reducing fade
    }
  }
  
  // Send the music-reactive pattern to LED strip
  FastLED.show();
  
  // Auto-change patterns on strong beats (optional)
  static unsigned long lastAutoChange = 0;
  if (beatDetected && audioLevel > 0.7 && (currentTime - lastAutoChange > 10000)) {
    nextPattern();
    lastAutoChange = currentTime;
  }
  
  // Update hue based on audio level for color shifts
  gHue = (uint8_t)(audioLevel * 255);
}

void runLocalPatterns() {
  unsigned long currentTime = millis();
  
  // Non-blocking frame rate limiting - faster for real-time sync
  if (currentTime - lastPatternUpdate < 20) {
    return; // Too soon for next frame (50 FPS)
  }
  lastPatternUpdate = currentTime;
  
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // do some periodic updates (non-blocking)
  static unsigned long lastHueUpdate = 0;
  if (currentTime - lastHueUpdate > 20) {
    gHue++;
    lastHueUpdate = currentTime;
  }
  
  static unsigned long lastPatternChange = 0;
  if (currentTime - lastPatternChange > 5000) { // 5 seconds
    nextPattern();
    lastPatternChange = currentTime;
  }
}

void broadcastPatternState() {
  // NEW: Broadcast pattern synchronization state (single tiny message!)
  static uint16_t frameCounter = 0;
  static unsigned long patternStartTime = millis();
  static uint8_t lastPattern = 255; // Invalid pattern to force reset
  
  // Reset timing when pattern changes
  if (gCurrentPatternNumber != lastPattern) {
    patternStartTime = millis();
    frameCounter = 0;
    lastPattern = gCurrentPatternNumber;
    Serial.print("Broadcasting new pattern: ");
    Serial.println(gCurrentPatternNumber);
  }
  
  PatternSyncMessage message;
  message.messageType = 0x01; // Pattern state
  message.patternNumber = gCurrentPatternNumber;
  message.patternStartTime = patternStartTime;
  message.currentTime = millis();
  message.globalHue = gHue;
  message.randomSeed = frameCounter * 12345 + gCurrentPatternNumber; // Predictable but varied seed
  message.frameNumber = frameCounter++;
  
  // Clear reserved bytes
  memset(message.reserved, 0, sizeof(message.reserved));
  
  // Calculate checksum over all bytes except checksum field
  message.checksum = 0;
  uint8_t* data = (uint8_t*)&message;
  for (int i = 0; i < sizeof(PatternSyncMessage) - 1; i++) {
    message.checksum ^= data[i];
  }
  
  // Send single message - only ~30 bytes!
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&message, sizeof(message));
  
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 100) { // Send updates every 100ms for real-time sync
    Serial.print("Leading pattern ");
    Serial.print(gCurrentPatternNumber);
    Serial.print(", frame ");
    Serial.print(frameCounter);
    Serial.print(", send result: ");
    Serial.print(result == ESP_OK ? "OK" : "FAIL");
    Serial.print(", msg size: ");
    Serial.println(sizeof(message));
    lastLog = millis();
  }
}

void broadcastHeartbeat() {
  // Send a simple heartbeat message to maintain connection
  PatternSyncMessage heartbeat;
  heartbeat.messageType = 0x02; // Heartbeat
  heartbeat.patternNumber = gCurrentPatternNumber;
  heartbeat.patternStartTime = 0;
  heartbeat.currentTime = millis();
  heartbeat.globalHue = gHue;
  heartbeat.randomSeed = 0;
  heartbeat.frameNumber = 0;
  memset(heartbeat.reserved, 0, sizeof(heartbeat.reserved));
  
  // Calculate checksum over all bytes except checksum field
  heartbeat.checksum = 0;
  uint8_t* data = (uint8_t*)&heartbeat;
  for (int i = 0; i < sizeof(PatternSyncMessage) - 1; i++) {
    heartbeat.checksum ^= data[i];
  }
  
  // Send heartbeat
  esp_now_send(broadcastAddress, (uint8_t*)&heartbeat, sizeof(heartbeat));
}

void applySyncedPatternState(const PatternSyncMessage& syncMsg) {
  // NEW: Apply synchronized pattern state from leader
  // Calculate time offset for synchronization
  unsigned long now = millis();
  timeOffset = now - syncMsg.currentTime;
  
  // Switch pattern if needed
  if (gCurrentPatternNumber != syncMsg.patternNumber) {
    gCurrentPatternNumber = syncMsg.patternNumber;
    Serial.print("Switched to leader's pattern: ");
    Serial.println(gCurrentPatternNumber);
  }
  
  // Synchronize timing and state
  leaderPatternStartTime = syncMsg.patternStartTime;
  gHue = syncMsg.globalHue;
  
  // Synchronize random seed for patterns that use randomness
  if (syncMsg.randomSeed != 0) {
    syncedRandomSeed = syncMsg.randomSeed;
    randomSeed(syncedRandomSeed);
  }
  
  lastFrameNumber = syncMsg.frameNumber;
  patternSyncActive = true;
}

void runSynchronizedPatterns() {
  // NEW: Run patterns synchronized with leader timing
  unsigned long currentTime = millis();
  
  // Non-blocking frame rate limiting - faster for real-time sync
  if (currentTime - lastPatternUpdate < 20) {
    return; // Too soon for next frame (50 FPS)
  }
  lastPatternUpdate = currentTime;
  
  // Run the EXACT same pattern as leader with NO local pattern switching
  if (gCurrentPatternNumber < ARRAY_SIZE(gPatterns)) {
    gPatterns[gCurrentPatternNumber]();
  }

  // Send the synchronized pattern to LED strip
  FastLED.show();
  
  // gHue is already synchronized from leader, no need to update locally
  // Pattern number changes ONLY come from leader messages
}

void nextPattern() {

  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
  Serial.print("Next pattern:");
  Serial.println(gCurrentPatternNumber);
}

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) {  //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// NEW PATTERNS ADDED FROM OLDPLAYALIGHTS 🔥

void fire() {
  // Realistic fire simulation using heat array
  static uint8_t heat[NUM_LEDS/2];
  int half = NUM_LEDS/2;
  uint8_t cooling = 55;  // Adjustable cooling rate
  uint8_t sparking = 120; // Adjustable spark rate
  
  // Step 1: Cool down every cell a little
  for(int i = 0; i < half; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / half) + 2));
  }
  
  // Step 2: Heat from each cell drifts up and diffuses
  for(int k = half - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // Step 3: Randomly ignite new sparks near bottom
  if(random8() < sparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // Step 4: Convert heat to LED colors (mirrored for full strip)
  for(int j = 0; j < half; j++) {
    CRGB color = ColorFromPalette(HeatColors_p, scale8(heat[j], 240));
    leds[half + j] = color;
    leds[half - 1 - j] = color;  // Mirror effect
  }
}

void lightningStorm() {
  // Dark stormy background with lightning strikes
  static unsigned long lastStrike = 0;
  unsigned long currentTime = millis();
  
  // Fade to dark blue stormy background
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(220); // Fade existing
    leds[i] += CRGB(0, 0, 15); // Add dark blue atmosphere
  }
  
  // Random lightning strikes
  if(currentTime - lastStrike > random16(100, 2000)) {
    lastStrike = currentTime;
    
    // Lightning strike position and length
    int strikePos = random16(NUM_LEDS - 30);
    int strikeLength = random8(10, 25);
    
    // Bright white lightning
    for(int i = strikePos; i < strikePos + strikeLength && i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 255, 255); // Bright white
    }
    
    // Secondary weaker strikes
    if(random8() < 150) {
      int secondaryPos = strikePos + random8(-5, 5);
      int secondaryLen = random8(5, 12);
      for(int i = secondaryPos; i < secondaryPos + secondaryLen && i >= 0 && i < NUM_LEDS; i++) {
        leds[i] = CRGB(200, 200, 255); // Bluish lightning
      }
    }
  }
}

void plasmaField() {
  // Multiple overlapping plasma waves - perfect for music!
  static uint16_t plasmaTime = 0;
  static uint8_t plasmaHue = 0;
  
  plasmaTime += 2;
  plasmaHue += 1;
  
  for(int i = 0; i < NUM_LEDS; i++) {
    // Multiple layered sine waves
    uint8_t wave1 = sin8(plasmaTime/4 + i * 8);
    uint8_t wave2 = sin8(plasmaTime/3 + i * 6 + 85);
    uint8_t wave3 = sin8(plasmaTime/5 + i * 4 + 170);
    uint8_t wave4 = sin8(plasmaTime/7 + i * 12);
    
    // Combine waves
    uint8_t combined = (wave1/4 + wave2/3 + wave3/3 + wave4/6);
    uint8_t hue = plasmaHue + combined/2;
    uint8_t brightness = combined + sin8(plasmaTime/8 + i)/4;
    
    leds[i] = CHSV(hue, 240, brightness);
  }
}

void meteorShower() {
  // Multiple meteors with trails
  struct Meteor {
    int16_t pos;
    uint8_t hue;
    uint8_t size;
    int8_t speed;
  };
  static Meteor meteors[6];
  static bool initialized = false;
  static unsigned long lastUpdate = 0;
  
  if(!initialized) {
    for(int i = 0; i < 6; i++) {
      meteors[i] = {(int16_t)(-random8(20)), (uint8_t)random8(), (uint8_t)(3 + random8(4)), (int8_t)(1 + random8(2))};
    }
    initialized = true;
  }
  
  // Fade trail
  fadeToBlackBy(leds, NUM_LEDS, 60);
  
  // Update meteors
  if(millis() - lastUpdate > 50) {
    lastUpdate = millis();
    
    for(int m = 0; m < 6; m++) {
      Meteor &meteor = meteors[m];
      
      // Draw meteor trail
      for(int t = 0; t < meteor.size; t++) {
        int16_t trailPos = meteor.pos - t;
        if(trailPos >= 0 && trailPos < NUM_LEDS) {
          uint8_t brightness = map(t, 0, meteor.size-1, 255, 50);
          leds[trailPos] += CHSV(meteor.hue, 200, brightness);
        }
      }
      
      // Move meteor
      meteor.pos += meteor.speed;
      
      // Reset if off screen
      if(meteor.pos >= NUM_LEDS + meteor.size) {
        meteor.pos = -meteor.size;
        meteor.hue = random8();
        meteor.size = 3 + random8(4);
        meteor.speed = 1 + random8(2);
      }
    }
  }
}

void auroraWaves() {
  // Flowing aurora waves - smooth and musical
  static uint16_t wave1_pos = 0, wave2_pos = 0, wave3_pos = 0;
  static uint8_t auroraHue = 96; // Start with green
  
  wave1_pos += 2;
  wave2_pos += 3;
  wave3_pos += 1;
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  for(int i = 0; i < NUM_LEDS; i++) {
    // Three overlapping aurora waves
    uint8_t wave1 = sin8(wave1_pos + i * 4);
    uint8_t wave2 = sin8(wave2_pos + i * 6 + 85);
    uint8_t wave3 = sin8(wave3_pos + i * 2 + 170);
    
    // Aurora colors (green to blue)
    uint8_t hue1 = auroraHue + sin8(i * 8)/8;
    uint8_t hue2 = auroraHue + 40;
    uint8_t hue3 = auroraHue + 80;
    
    // Only show bright parts of waves
    if(wave1 > 100) {
      leds[i] += CHSV(hue1, 255, (wave1-100)*2);
    }
    if(wave2 > 120) {
      leds[i] += CHSV(hue2, 240, (wave2-120)*2);
    }
    if(wave3 > 140) {
      leds[i] += CHSV(hue3, 200, (wave3-140)*3);
    }
  }
  
  // Slowly shift aurora color
  if(random8() < 2) {
    auroraHue += random8(5) - 2;
    auroraHue = constrain(auroraHue, 80, 140);
  }
}

void lavaFlow() {
  // Flowing molten lava effect
  for(int i = 0; i < NUM_LEDS; i++) {
    // Use noise for organic flow
    uint8_t heat = inoise8(i * 40, millis() / 60);
    
    // Create lava colors: black -> red -> orange -> yellow
    CRGB color;
    if(heat < 128) {
      // Black to red gradient
      color = CRGB(heat * 2, 0, 0);
    } else {
      // Red to orange to yellow
      uint8_t excess = heat - 128;
      color = CRGB(255, excess * 2, excess / 4);
    }
    
    leds[i] = color;
  }
}

// Changes all LEDS to given color
void allColor(CRGB c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = c;
  }
  FastLED.show();
}

void allRandom() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = randomColor();
  }
  FastLED.show();
}
void doDissolve() {
  disolve(15, 100, MEDIUM);
}

// Random disolve colors
void disolve(int simultaneous, int cycles, int speed) {
  for (int i = 0; i < cycles; i++) {
    for (int j = 0; j < simultaneous; j++) {
      int idx = random(NUM_LEDS);
      leds[idx] = CRGB::Black;
    }
    FastLED.show();
    // delay(speed); // Non-blocking - removed for real-time sync
  }

  allColor(CRGB::Black);
}

// Flashes given color
// If c==NULL, random color flash
void flash(CRGB c, int count, int speed) {
  for (int i = 0; i < count; i++) {
    if (c) {
      allColor(c);
    } else {
      allColor(randomColor());
    }
    // delay(speed); // Non-blocking - removed for real-time sync
    allColor(CRGB::Black);
    // delay(speed); // Non-blocking - removed for real-time sync
  }
}

// Wipes color from end to end
void colorWipe(CRGB c, int speed, int direction) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (direction == FORWARD) {
      leds[i] = c;
    } else {
      leds[NUM_LEDS - 1 - i] = c;
    }
    FastLED.show();
    // delay(speed); // Non-blocking - removed for real-time sync
  }
}

// Rainbow colors that slowly cycle across LEDs
void rainbow(int cycles, int speed) {  // TODO direction
  if (cycles == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = Wheel(((i * 256 / NUM_LEDS)) & 255);
    }
    FastLED.show();
  } else {
    for (int j = 0; j < 256 * cycles; j++) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      }
      FastLED.show();
      // delay(speed); // Non-blocking - removed for real-time sync
    }
  }
}

// Theater-style crawling lights
void theaterChase(CRGB c, int cycles, int speed) {  // TODO direction

  for (int j = 0; j < cycles; j++) {
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        int pos = i + q;
        leds[pos] = c;  //turn every third pixel on
      }
      FastLED.show();

      // delay(speed); // Non-blocking - removed for real-time sync

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        leds[i + q] = CRGB::Black;  //turn every third pixel off
      }
    }
  }
}

void doChase(){
   // Non-blocking chase pattern
   static uint8_t chasePosition = 0;
   static unsigned long lastChaseUpdate = 0;
   
   unsigned long now = millis();
   if (now - lastChaseUpdate > MEDIUM) {
     // Clear all LEDs
     for (int i = 0; i < NUM_LEDS; i++) {
       leds[i] = CRGB::Black;
     }
     
     // Set every third LED starting from chasePosition
     for (int i = chasePosition; i < NUM_LEDS; i += 3) {
       leds[i] = Wheel((i + chasePosition * 10) % 255);
     }
     
     chasePosition = (chasePosition + 1) % 3;
     lastChaseUpdate = now;
   }
}

// Theater-style crawling lights with rainbow effect
void theaterChaseRainbow(int cycles, int speed) {  // TODO direction, duration
  for (int j = 0; j < 256 * cycles; j++) {         // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        int pos = i + q;
        leds[pos] = Wheel((i + j) % 255);  //turn every third pixel on
      }
      FastLED.show();

      // delay(speed); // Non-blocking - removed for real-time sync

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        leds[i + q] = CRGB::Black;  //turn every third pixel off
      }
    }
  }
}

void doLightning(){
  lightning(NULL,15,50,MEDIUM);
  lightning(CRGB::White,20,50,MEDIUM);

}
// Random flashes of lightning
void lightning(CRGB c, int simultaneous, int cycles, int speed) {
  int flashes[simultaneous];

  for (int i = 0; i < cycles; i++) {
    for (int j = 0; j < simultaneous; j++) {
      int idx = random(NUM_LEDS);
      flashes[j] = idx;
      leds[idx] = c ? c : randomColor();
    }
    FastLED.show();
    // delay(speed); // Non-blocking - removed for real-time sync
    for (int s = 0; s < simultaneous; s++) {
      leds[flashes[s]] = CRGB::Black;
    }
    // delay(speed); // Non-blocking - removed for real-time sync
  }
}

void doCylon(){
   for(int i=0; i<2; i++){
    cylon(randomColor(), 10,FAST);
  }

}

// Sliding bar across LEDs
void cylon(CRGB c, int width, int speed) {
  // First slide the leds in one direction
  for (int i = 0; i <= NUM_LEDS - width; i++) {
    for (int j = 0; j < width; j++) {
      leds[i + j] = c;
    }

    FastLED.show();

    // now that we've shown the leds, reset to black for next loop
    for (int j = 0; j < 5; j++) {
      leds[i + j] = CRGB::Black;
    }
    // delay(speed); // Non-blocking - removed for real-time sync
  }

  // Now go in the other direction.
  for (int i = NUM_LEDS - width; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      leds[i + j] = c;
    }
    FastLED.show();
    for (int j = 0; j < width; j++) {
      leds[i + j] = CRGB::Black;
    }

    // delay(speed); // Non-blocking - removed for real-time sync
  }
}

void doStripes() {
  for (int i = 0; i < 3; i++) {
    CRGB c1 = randomColor();
    CRGB c2 = randomColor();
    stripes(c1, c2, 5);
    // delay(2000); // Non-blocking - removed for real-time sync
    stripes(c2, c1, 5);
    // delay(2000); // Non-blocking - removed for real-time sync
  }
}
// Display alternating stripes
void stripes(CRGB c1, CRGB c2, int width) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % (width * 2) < width) {
      leds[i] = c1;
    } else {
      leds[i] = c2;
    }
  }
  FastLED.show();
}

// Theater-style crawling of stripes
void stripesChase(CRGB c1, CRGB c2, int width, int cycles, int speed) {  // TODO direction
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
CRGB Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

CRGB randomColor() {
  return Wheel(random(256));
}