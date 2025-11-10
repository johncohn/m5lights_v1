/*
  NeoPixel Controller - Modular Version with OTA
  42 patterns with robust networking, audio, UI, and OTA updates
  Refactored into manageable modules
  
  ** SERIAL DEBUG OUTPUT SIGNIFICANTLY REDUCED **
  Use DEBUG_SERIAL, DEBUG_HEARTBEAT, DEBUG_BPM in config.h to control output
*/

#include "config.h"
#include "patterns.h" 
#include "networking.h"
#include "audio.h"
#include "ui.h"
#include "ota.h"
#include "version.h"

// ── Global Variable Definitions ───────────────────────────────────────────────
Mode      currentMode      = AUTO;
FsmState  fsmState         = FOLLOWER;
uint8_t   styleIdx         = 0;
bool      freezeActive     = false;

CRGB      leds[NUM_LEDS];
LGFX_Sprite canvas(&M5.Lcd);
Preferences prefs;

// ── Global Brightness Control ────────────────────────────────────────────────
uint8_t   globalBrightnessScale = 64;  // 25% of max brightness (64/255) - LOCAL CONTROL

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

// ── OTA Coordination Variables ───────────────────────────────────────────────
bool     otaSuspended        = false;  // Start active - controlled manually via serial commands
uint32_t otaSuspendTimeout   = 0;      // No longer used - kept for compatibility  
uint32_t otaSuspendDuration  = 300000; // No longer used - kept for compatibility
uint32_t bootupQuietTime     = 60000;  // Stay quiet for 60s after boot

// ── Audio Variables ───────────────────────────────────────────────────────────
float   soundMin      = 1.0f, soundMax      = 0.0f, musicLevel = 0.0f;
bool    prevAbove     = false;
uint32_t beatTimes[50];
uint8_t  beatCount    = 0;
uint32_t lastBpmMillis= 0;
bool    audioDetected = true;

// ── Watchdog Variables ────────────────────────────────────────────────────────
uint32_t lastWatchdogUpdate = 0;
const uint32_t WATCHDOG_TIMEOUT = 30000;  // 30 seconds

// ── System State Variables ────────────────────────────────────────────────────
uint32_t lastSystemCheck = 0;
const uint32_t SYSTEM_CHECK_INTERVAL = 5000; // Check system health every 5 seconds

// ── Non-blocking OFF mode timing ──────────────────────────────────────────────
static uint32_t lastOFFModeUpdate = 0;
const uint32_t OFF_MODE_UPDATE_INTERVAL = 200; // Update OFF mode every 200ms

void feedWatchdog() {
  lastWatchdogUpdate = millis();
}

void checkWatchdog() {
  uint32_t now = millis();
  if (now - lastWatchdogUpdate > WATCHDOG_TIMEOUT) {
    if(DEBUG_SERIAL) Serial.println("WATCHDOG: System appears hung - restarting...");
    ESP.restart();
  }
}

void checkSystemHealth() {
  uint32_t now = millis();
  if (now - lastSystemCheck < SYSTEM_CHECK_INTERVAL) return;
  lastSystemCheck = now;
  
  // Check ESP-NOW peer status
  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if(DEBUG_SERIAL) Serial.println("HEALTH: ESP-NOW broadcast peer missing - re-adding");
    esp_now_peer_info_t peer={};
    memcpy(peer.peer_addr, broadcastAddress, 6);
    peer.channel = 0; 
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  
  // v56 style OTA doesn't need complex reinitialization logic
  
  // Check if we're stuck in a bad state
  static FsmState lastFsmState = FOLLOWER;
  static uint32_t stateChangeTime = now;
  static int stuckCounter = 0;
  
  if (fsmState != lastFsmState) {
    lastFsmState = fsmState;
    stateChangeTime = now;
    stuckCounter = 0;
  } else {
    // Check if we've been in the same state too long
    if (now - stateChangeTime > 60000) { // 1 minute in same state
      stuckCounter++;
      if (stuckCounter > 3) {
        if(DEBUG_SERIAL) Serial.printf("HEALTH: Stuck in %s state - forcing election\n", 
          fsmState == LEADER ? "LEADER" : fsmState == FOLLOWER ? "FOLLOWER" : "ELECT");
        
        // Force a new election to unstick the system
        fsmState = ELECT;
        electionStart = now;
        electionEnd = now + ELECTION_TIMEOUT;
        highestTokenSeen = myToken;
        myDelay = random(0, ELECTION_JITTER);
        electionBroadcasted = false;
        missedFrameCount = 0;
        chunkMask = 0;
        stuckCounter = 0;
      }
    }
  }
  
  // Check for memory issues
  if (ESP.getFreeHeap() < 10000) { // Less than 10KB free
    if(DEBUG_SERIAL) Serial.printf("HEALTH: Low memory - %d bytes free\n", ESP.getFreeHeap());
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup(){
  Serial.begin(115200);
  
  // Initialize watchdog
  feedWatchdog();
  
  // Show startup info
  if(DEBUG_SERIAL) {
    Serial.println("=====================================");
    Serial.printf("NeoPixel Controller v%s\n", FIRMWARE_VERSION);
    Serial.printf("Build: %s %s\n", BUILD_DATE, BUILD_TIME);
    Serial.println("=====================================");
  }
  
  // Initialize M5 hardware
  M5.begin();
  feedWatchdog();
  
  // Initialize all modules
  loadControls();
  feedWatchdog();
  initUI();
  feedWatchdog();
  initAudio();
  feedWatchdog();
  initNetworking();
  feedWatchdog();
  
  // Initialize OTA (must be after networking for WiFi)
  initOTA();
  feedWatchdog();
  
  // Initialize FastLED
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  // Start with default local brightness (25%)
  FastLED.setBrightness(globalBrightnessScale);
  feedWatchdog();

  // Initialize timing and state
  randomSeed(micros());
  lastRecvMillis     = millis();
  lastTokenBroadcast = millis();
  lastHeartbeat      = millis();
  lastSystemCheck    = millis();
  missedFrameCount   = 0;
  
  // Ensure we start in a clean state
  fsmState = FOLLOWER;
  currentMode = AUTO;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  // OTA Coordination: ESP-NOW suspension is now controlled manually via serial commands
  // No automatic boot quiet mode - use "SUSPEND_ESPNOW" and "RESUME_ESPNOW" commands
  
  if(DEBUG_SERIAL) {
    Serial.printf("NeoPixel Controller v%s initialized - 42 patterns ready!\n", FIRMWARE_VERSION);
    Serial.printf("Ready for OTA updates at: NeoNode-%06X.local\n", myToken);
    Serial.println("Watchdog timer enabled (30s timeout)");
    Serial.printf("Local brightness: %d/255 (%.1f%%) - each node controls its own\n", 
      globalBrightnessScale, (globalBrightnessScale * 100.0f) / 255.0f);
    Serial.println("Button A: Short press = local brightness cycle, Long press = OFF/ON");
    Serial.println("Button B: Pattern freeze/advance (AUTO-LEADER mode only)");
    Serial.println("Button C: Manual sync reset (force resynchronization)");
    Serial.println("ESP-NOW mesh network active - WiFi optional for OTA");
  }
  feedWatchdog();
}

// ── Serial Command Handler ────────────────────────────────────────────────────
void handleSerialCommands(){
  static String commandBuffer = "";
  
  // Read any available serial data
  while(Serial.available()) {
    char c = Serial.read();
    
    // Debug: Flash blue briefly for ANY serial input
    static uint32_t lastFlash = 0;
    if(millis() - lastFlash > 100) { // Limit flash rate
      fill_solid(leds, 5, CRGB::Blue); // Flash first 5 LEDs blue
      FastLED.show();
      lastFlash = millis();
    }
    
    if(c == '\n' || c == '\r') {
      // Command complete - process it
      commandBuffer.trim();
      
      // No serial commands currently implemented
      if(commandBuffer.length() > 0) {
        Serial.println("[SERIAL] No commands available");
      }
      
      commandBuffer = ""; // Clear buffer
    } else {
      commandBuffer += c; // Add character to buffer
      
      // Prevent buffer overflow
      if(commandBuffer.length() > 50) {
        commandBuffer = "";
        Serial.println("ERROR: Command too long");
      }
    }
  }
}

// ── Main Loop ─────────────────────────────────────────────────────────────────
void loop(){
  // Feed watchdog at start of every loop
  feedWatchdog();
  
  M5.update();
  
  // Handle serial commands for OTA coordination
  handleSerialCommands();
  
  // Initialize OTA if WiFi becomes available (retry periodically)
  initOTA();
  
  // Handle OTA updates (highest priority, but only if WiFi connected)
  handleOTA();
  
  // Handle user input
  handleButtons();
  
  if(currentMode == OFF) {
    // OFF mode - minimal processing for battery savings
    uint32_t now = millis();
    if (now - lastOFFModeUpdate >= OFF_MODE_UPDATE_INTERVAL) {
      if (shouldUpdateUI()) drawUI();   // Show OFF status on dimmed display
      lastOFFModeUpdate = now;
    }
    return;     // Skip all LED/networking processing
  }
  
  // AUTO mode - full functionality
  handleNetworking(); // This handles WiFi transitions gracefully
  if (shouldUpdateUI()) drawUI();  // Non-blocking UI updates
  updateBPM();
  
  // Periodic system health checks
  checkSystemHealth();
  
  // Check watchdog (will restart if hung)
  checkWatchdog();
}