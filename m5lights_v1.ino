/// @file    m5lights_v1.ino
/// @brief   FastLED demo reel adapted for M5StickC Plus 2
/// @version 2.0.0
/// @date    2024-10-24
/// @author  John Cohn (adapted from Mark Kriegsman)
/// @example m5lights_v1.ino
///
/// @changelog
/// v2.0.0 (2024-10-24) - ESP-NOW Synchronization System
///   - Added three-mode operation: Normal, Lead, Follow
///   - ESP-NOW wireless synchronization between multiple devices
///   - Lead mode broadcasts LED states to all followers
///   - Follow mode receives and displays leader's LED patterns
///   - Automatic leader discovery and timeout handling
///   - Button controls: long press (3s) -> Lead, short press -> Normal/Next Pattern
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

FASTLED_USING_NAMESPACE

// Version info
#define VERSION "2.0.0"
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
#define NUM_LEDS 250       // Back to original count
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 128
#define FRAMES_PER_SECOND 400

// ESP-NOW Synchronization System
enum NodeMode {
  MODE_NORMAL,    // Standalone mode - runs patterns locally
  MODE_LEAD,      // Lead mode - runs patterns and broadcasts LED states
  MODE_FOLLOW     // Follow mode - receives and displays LED states from leader
};

// ESP-NOW message structure
struct SyncMessage {
  uint8_t messageType;      // 0x01 = LED data, 0x02 = heartbeat
  uint8_t patternNumber;    // Current pattern being displayed
  uint32_t timestamp;       // Message timestamp
  uint16_t ledCount;        // Number of LEDs in this message
  uint16_t ledOffset;       // Starting LED index for this chunk
  uint8_t ledData[240];     // LED color data (RGB, max 80 LEDs per message)
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

// Timing constants
#define LEADER_TIMEOUT_MS 10000      // 10 seconds
#define LEADER_CHECK_INTERVAL_MS 10000  // Check for leader every 10 seconds
#define LONG_PRESS_TIME_MS 3000      // 3 seconds for long press
#define SYNC_SEND_INTERVAL_MS 50     // Send LED data every 50ms in lead mode

// ESP-NOW callback functions
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Called when data is sent via ESP-NOW
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW send failed");
  }
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t *incomingData, int len) {
  if (len != sizeof(SyncMessage)) return;
  
  SyncMessage receivedMessage;
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
  
  // Verify checksum
  uint8_t calculatedChecksum = 0;
  uint8_t* data = (uint8_t*)&receivedMessage;
  for (int i = 0; i < sizeof(SyncMessage) - 1; i++) {
    calculatedChecksum ^= data[i];
  }
  
  if (calculatedChecksum != receivedMessage.checksum) {
    return; // Invalid checksum
  }
  
  lastLeaderMessage = millis();
  leaderDetected = true;
  
  // If we're in normal mode and detect a leader, switch to follow mode
  if (currentMode == MODE_NORMAL) {
    switchToFollowMode();
  }
  
  // If we're in follow mode, apply the received LED data
  if (currentMode == MODE_FOLLOW && receivedMessage.messageType == 0x01) {
    applyReceivedLEDData(receivedMessage);
  }
}

void setupESPNOW() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceived);
  
  // Add broadcast peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
  }
  
  Serial.println("ESP-NOW initialized");
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
  delay(1000);  // Reduced delay

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
SimplePatternList gPatterns = {  rainbow, doChase,juggle, rainbowWithGlitter, confetti,  bpm };

uint8_t gCurrentPatternNumber = 0;  // Index number of which pattern is current
uint8_t gHue = 0;                   // rotating "base color" used by many of the patterns

void loop() {
  M5.update();
  
  // Handle button logic for mode switching
  handleButtonLogic();
  
  // Handle mode-specific behavior
  handleCurrentMode();
  
  // Update the display if needed
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Mode switching and helper functions
void switchToNormalMode() {
  if (currentMode != MODE_NORMAL) {
    currentMode = MODE_NORMAL;
    lastModeSwitch = millis();
    Serial.println("Switched to NORMAL mode");
    updateDisplay();
  }
}

void switchToLeadMode() {
  if (currentMode != MODE_LEAD) {
    currentMode = MODE_LEAD;
    lastModeSwitch = millis();
    Serial.println("Switched to LEAD mode");
    updateDisplay();
  }
}

void switchToFollowMode() {
  if (currentMode != MODE_FOLLOW) {
    currentMode = MODE_FOLLOW;
    lastModeSwitch = millis();
    Serial.println("Switched to FOLLOW mode");
    updateDisplay();
  }
}

void updateDisplay() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  
  // Title and version
  M5.Display.drawString("FastLED Sync", 10, 10);
  M5.Display.drawString("v" + String(VERSION), 10, 20);
  
  // Current mode
  String modeStr = "";
  switch (currentMode) {
    case MODE_NORMAL: modeStr = "NORMAL"; break;
    case MODE_LEAD: modeStr = "LEAD"; break;
    case MODE_FOLLOW: modeStr = "FOLLOW"; break;
  }
  M5.Display.drawString("Mode: " + modeStr, 10, 35);
  
  // Pattern info (for normal and lead modes)
  if (currentMode != MODE_FOLLOW) {
    M5.Display.drawString("Pattern: " + String(gCurrentPatternNumber), 10, 50);
  } else {
    M5.Display.drawString("Following...", 10, 50);
  }
  
  // Additional status
  M5.Display.drawString("G32->LEDs", 10, 65);
}

void handleButtonLogic() {
  // Button A logic for mode switching
  if (M5.BtnA.isPressed() && !buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();
  }
  
  if (!M5.BtnA.isPressed() && buttonPressed) {
    buttonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStart;
    
    if (pressDuration >= LONG_PRESS_TIME_MS) {
      // Long press - switch to lead mode
      switchToLeadMode();
    } else {
      // Short press - switch to normal mode or next pattern
      if (currentMode == MODE_NORMAL) {
        nextPattern();
      } else {
        switchToNormalMode();
      }
    }
    updateDisplay();
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
  }
}

void handleNormalMode(unsigned long currentTime) {
  // Run patterns locally
  runLocalPatterns();
  
  // Periodically check for leader
  if (currentTime - lastLeaderCheck > LEADER_CHECK_INTERVAL_MS) {
    lastLeaderCheck = currentTime;
    // The ESP-NOW receive callback will automatically switch to follow mode if leader detected
  }
}

void handleLeadMode(unsigned long currentTime) {
  // Run patterns locally
  runLocalPatterns();
  
  // Broadcast LED states
  static unsigned long lastBroadcast = 0;
  if (currentTime - lastBroadcast > SYNC_SEND_INTERVAL_MS) {
    broadcastLEDState();
    lastBroadcast = currentTime;
  }
}

void handleFollowMode(unsigned long currentTime) {
  // Check for leader timeout
  if (currentTime - lastLeaderMessage > LEADER_TIMEOUT_MS) {
    Serial.println("Leader timeout - switching to normal mode");
    switchToNormalMode();
    leaderDetected = false;
    return;
  }
  
  // In follow mode, LEDs are updated by the receive callback
  // Just need to update the display
  FastLED.show();
}

void runLocalPatterns() {
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }  // slowly cycle the "base color" through the rainbow
  
  EVERY_N_SECONDS(5) {
    nextPattern();
  }  // change patterns periodically
}

void broadcastLEDState() {
  // Send LED data in chunks due to ESP-NOW message size limits
  const int ledsPerMessage = 80; // 240 bytes / 3 bytes per LED
  int totalMessages = (NUM_LEDS + ledsPerMessage - 1) / ledsPerMessage;
  
  for (int msg = 0; msg < totalMessages; msg++) {
    SyncMessage message;
    message.messageType = 0x01; // LED data
    message.patternNumber = gCurrentPatternNumber;
    message.timestamp = millis();
    message.ledOffset = msg * ledsPerMessage;
    message.ledCount = min(ledsPerMessage, NUM_LEDS - message.ledOffset);
    
    // Copy LED data
    for (int i = 0; i < message.ledCount; i++) {
      int ledIndex = message.ledOffset + i;
      message.ledData[i * 3] = leds[ledIndex].r;
      message.ledData[i * 3 + 1] = leds[ledIndex].g;
      message.ledData[i * 3 + 2] = leds[ledIndex].b;
    }
    
    // Calculate checksum
    message.checksum = 0;
    uint8_t* data = (uint8_t*)&message;
    for (int i = 0; i < sizeof(SyncMessage) - 1; i++) {
      message.checksum ^= data[i];
    }
    
    // Send the message
    esp_now_send(broadcastAddress, (uint8_t*)&message, sizeof(message));
    
    // Small delay between messages
    delay(5);
  }
}

void applyReceivedLEDData(const SyncMessage& message) {
  // Apply received LED data to our local LED array
  for (int i = 0; i < message.ledCount; i++) {
    int ledIndex = message.ledOffset + i;
    if (ledIndex < NUM_LEDS) {
      leds[ledIndex].r = message.ledData[i * 3];
      leds[ledIndex].g = message.ledData[i * 3 + 1];
      leds[ledIndex].b = message.ledData[i * 3 + 2];
    }
  }
  
  // Update pattern number for display
  gCurrentPatternNumber = message.patternNumber;
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
    delay(speed);
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
    delay(speed);
    allColor(CRGB::Black);
    delay(speed);
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
    delay(speed);
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
      delay(speed);
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

      delay(speed);

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        leds[i + q] = CRGB::Black;  //turn every third pixel off
      }
    }
  }
}

void doChase(){
   theaterChaseRainbow(1,MEDIUM);
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

      delay(speed);

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
    delay(speed);
    for (int s = 0; s < simultaneous; s++) {
      leds[flashes[s]] = CRGB::Black;
    }
    delay(speed);
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
    delay(speed);
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

    delay(speed);
  }
}

void doStripes() {
  for (int i = 0; i < 3; i++) {
    CRGB c1 = randomColor();
    CRGB c2 = randomColor();
    stripes(c1, c2, 5);
    delay(2000);
    stripes(c2, c1, 5);
    delay(2000);
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