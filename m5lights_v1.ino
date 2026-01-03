/// @file    m5lights_v1_simple.ino
/// @brief   ESP-NOW LED Sync with Larry's 4 Beat-Reactive Patterns
/// @version 4.0.0
/// @date    2025-12-26
/// @author  John Cohn (Larry patterns adapted from larry_test_m5stack)
///
/// @changelog
/// v4.0.0 (2025-12-26) - MAJOR UPDATE: Larry's Beat-Reactive Patterns
///   - REMOVED all 14 old patterns (rainbow, confetti, bpm, fire, etc.)
///   - ADDED 4 Larry patterns from larry_test_m5stack:
///     * Solid Color - Random vibrant colors, beat triggers new color
///     * Rainbow - Smooth color wheel, BPM-synced rotation speed
///     * Sine Wave Chase - Color waves with dramatic dark gaps, BPM-synced motion
///     * Wavy Flag - Red/white/blue patriotic pattern, BPM-synced waves
///   - ADDED gamma correction system for rich, saturated colors
///   - ADDED fixed-point math utilities (sine/cosine tables, HSV conversion)
///   - ADDED beat-reactive helpers (getBeatSpeed, getBeatBrightnessPulse)
///   - Beat reactivity: patterns sync speed to BPM and pulse brightness on beats
///   - Patterns use 1536 hue units (vs FastLED's 256) for smoother gradients
///   - Critical signed char fix for proper dark gaps in sine waves
///   - Pattern state resets when switching for fresh random parameters
///   - All ESP-NOW sync, music detection, and UI features preserved
/// v3.8.2 (2024-11-24) - Longer Pattern Duration
///   - Changed auto-advance from 15 seconds to 30 seconds (2x longer)
///   - Patterns now stay visible longer for better enjoyment
/// v3.8.1 (2024-11-24) - Extended Cross-Fade Time
///   - Changed fade duration from 1 second to 3 seconds
///   - Longer, more gradual transitions between patterns
/// v3.8.0 (2024-11-24) - Smooth Pattern Cross-Fading
///   - Added smooth cross-fade between patterns
///   - Renders both old and new patterns during transition
///   - Blends them together for professional-looking changes
///   - Works for both manual and auto pattern changes
///   - Minimal performance impact and memory overhead
/// v3.7.0 (2024-11-24) - Improved Beat Detection + Beat-Reactive Patterns (Max's Request!)
///   - MAJOR IMPROVEMENT: Interval-based BPM calculation for stability
///   - Tracks time between beats instead of just counting them
///   - Uses median filtering to find consistent beat interval
///   - Much more stable BPM that locks onto the actual tempo
///   - Added beat-reactive mode (toggle with long press on B button)
///   - Patterns respond to beats when enabled: sparkle, glitter, lightning, BPM, aurora
///   - Moved Rainbow+Glitter pattern away from Rainbow in the list
/// v3.6.2 (2024-11-24) - Smooth BPM Display (Max's Request!)
///   - Added smoothed BPM value to screen display
///   - BPM varies smoothly instead of jumping every 5 seconds
///   - Shows current tempo in music mode
/// v3.6.1 (2024-11-24) - Faster Decay (Max's Request!)
///   - Changed decay time from 0.5s to 0.25s (2x faster falloff)
///   - Even more responsive to rapid beat changes
/// v3.6.0 (2024-11-24) - Beat Threshold and Power Curve (Max's Request!)
///   - Added BRIGHTNESS_THRESHOLD to require more pronounced beats for brightness boost
///   - Applied power curve (exponent 2.0) to make quieter sounds have less impact
///   - Only audio above threshold triggers brightness increase
///   - Prevents lights from being too reactive to background/quiet sounds
///   - More dramatic response to actual beats and loud music
/// v3.5.1 (2024-11-24) - Faster Exponential Decay
///   - Changed decay time from 2.0s to 0.5s (much faster response)
///   - Changed from linear to exponential/Gaussian decay (more natural)
///   - Exponential decay: fast falloff initially, slows as it approaches target
///   - Creates more organic, musical feel
/// v3.5.0 (2024-11-24) - Smooth Brightness Decay Envelope (Max's Request!)
///   - MAJOR IMPROVEMENT: Added smooth decay to music-reactive brightness
///   - Fast attack: Brightness instantly jumps up on peaks
///   - Slow decay: Smoothly falls off over 2 seconds (configurable)
///   - New peaks interrupt decay if they're brighter
///   - Eliminates jarring brightness jumps - much smoother visual experience
///   - Perfect for music visualization - "breathing" effect
///   - Thanks to Max for the excellent feedback!
/// v3.4.2 (2024-11-24) - Ultra-Sticky Beat Detection (Even More Sensitive)
///   - ULTRA-LOW thresholds: 2 beats to enter, 1 beat to stay (was 3/2)
///   - Increased timeout to 20 seconds (was 15) - maximum stickiness
///   - Fixes persistent flickering - now requires minimal beat activity to stay YES
///   - Handles even quiet/subtle rhythmic music
/// v3.4.1 (2024-11-24) - Sticky Beat Detection with Hysteresis
///   - FIXED flickering beat detection - now much more stable
///   - Lowered detection threshold: 3 beats to enter (was 4) - more sensitive
///   - Added hysteresis: only 2 beats needed to stay in music mode - sticky
///   - Added 15 second timeout - stays in music mode even during quiet passages
///   - Beat indicator now stays "YES" throughout songs instead of flickering
///   - Better user experience for music detection
/// v3.4.0 (2024-11-24) - Pattern Overhaul: Full-Strand Patterns Only
///   - REMOVED chase/blob patterns: doChase, juggle, meteorShower (feedback: not visually appealing)
///   - ADDED 6 new full-strand patterns: Sparkle, ColorWaves, Pride, Ocean, Twinkle, Palette
///   - All patterns now fill entire LED strand for better visual impact
///   - Total patterns: 14 (was 11, removed 3, added 6)
///   - Better for public display - entire strand always lit
/// v3.3.4 (2024-11-24) - Auto-Restart on Stuck State (CRITICAL ROBUSTNESS FIX!)
///   - Node now automatically restarts if stuck receiving packets but not complete frames
///   - Detects two stuck states: (1) was working but stopped, (2) never got complete frame
///   - Timeout increased from 2s to 5s to avoid false positives
///   - ESP.restart() provides clean recovery instead of attempted resync
///   - Fixes sporadic 10+ minute frozen follower issue
///   - More robust than power cycling - happens automatically
/// v3.3.3 (2024-11-24) - Fixed ESP-NOW Callback Signature
///   - Changed onDataSent() parameter from wifi_tx_info_t to uint8_t* mac_addr
///   - Required for compilation with current ESP32 library version
/// v3.3.2 (2024-10-26) - Fixed ESP-NOW Packet Loss (CRITICAL FIX!)
///   - Added 500μs delay between packet sends to prevent buffer overflow
///   - Fixes "2 packets failed, 3 succeeded" broadcast failures
///   - Followers now receive all packets and display complete frames
///   - This was the root cause of followers not syncing!
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
#define VERSION "4.0.0"

// Hardware config
#define LED_PIN 32
#define NUM_LEDS 200
#define BRIGHTNESS 25  // Adjusted for M5Stick power stability
#define COLOR_ORDER GRB
#define CHIPSET WS2811

// Leader sync delay - adjust to match follower display timing
#define LEADER_DELAY_MS 10  // Delay before leader shows LEDs (ms) - reduced for smoother animation

// Fluffy Mode E1.31/sACN Configuration
#define FLUFFY_SSID "GMA-WIFI_Access_Point"
#define FLUFFY_PASSWORD "3576wifi"
#define E131_PORT 5568
#define E131_UNIVERSE 30
#define E131_START_CHANNEL 1
#define E131_MULTICAST_BASE "239.255.0."

CRGB leds[NUM_LEDS];
CRGB ledsNext[NUM_LEDS];  // Second buffer for cross-fading

// ===== GAMMA CORRECTION SYSTEM =====
// 8-bit gamma correction table for WS2812B LEDs (from original Larry code)
// Extends black range (0-41 -> pure black) for dramatic dark gaps
// Creates rich, saturated colors by applying exponential brightness curve
const byte gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2,
    3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  7,  7,  7,
    8,  8,  8,  9,  9,  9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14,
   14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22,
   23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 33,
   33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45,
   46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
   60, 61, 62, 63, 64, 65, 66, 67, 68, 70, 71, 72, 73, 74, 75, 76,
   77, 78, 79, 81, 82, 83, 84, 85, 86, 88, 89, 90, 91, 92, 94, 95,
   96, 97, 99,100,101,102,104,105,106,108,109,110,112,113,114,116,
  117,119,120,121,123,124,126,127,129,130,132,133,135,136,138,139,
  141,142,144,145,147,149,150,152,153,155,157,158,160,162,163,165,
  167,168,170,172,174,175,177,179,181,182,184,186,188,190,192,193,
  195,197,199,201,203,205,207,209,211,213,215,217,219,221,223,225
};

// Apply gamma correction to a single RGB value
CRGB applyGamma(CRGB color) {
  return CRGB(
    gammaTable[color.r],
    gammaTable[color.g],
    gammaTable[color.b]
  );
}

// ===== FIXED-POINT MATH UTILITIES =====
// Sine lookup table for fixed-point math (0-180 degrees)
// CRITICAL: Must use signed char on ESP32 for proper dark gaps!
// ESP32 treats 'char' as unsigned by default, which breaks sine wave troughs
const signed char sineTable[181] = {
  0,1,2,3,5,6,7,8,9,10,11,12,13,15,16,17,
  18,19,20,21,22,23,24,25,27,28,29,30,31,32,33,34,
  35,36,37,38,39,40,42,43,44,45,46,47,48,49,50,51,
  52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
  67,68,69,70,71,72,73,74,75,76,77,77,78,79,80,81,
  82,83,83,84,85,86,87,88,88,89,90,91,92,92,93,94,
  95,95,96,97,97,98,99,100,100,101,102,102,103,104,104,105,
  105,106,107,107,108,108,109,110,110,111,111,112,112,113,113,114,
  114,115,115,116,116,117,117,117,118,118,119,119,120,120,120,121,
  121,121,122,122,122,123,123,123,123,124,124,124,124,125,125,125,
  125,125,126,126,126,126,126,126,126,127,127,127,127,127,127,127,
  127,127,127,127,127
};

// Fixed-point sine function (angle in 720 units per cycle, 0-719)
// Returns signed value -127 to +127
// Uses 720-unit cycle (2× resolution of 360 degrees) for smoother animations
signed char fixSin(int angle) {
  angle %= 720;
  if (angle < 0) angle += 720;
  return (angle <= 360) ?
     sineTable[(angle <= 180) ?
       angle          :
      (360 - angle)] :
    -sineTable[(angle <= 540) ?
      (angle - 360)   :
      (720 - angle)];
}

// Fixed-point cosine function (angle in 720 units per cycle, 0-719)
// Returns signed value -127 to +127
signed char fixCos(int angle) {
  angle %= 720;
  if (angle < 0) angle += 720;
  return (angle <= 360) ?
    ((angle <= 180) ?  sineTable[180 - angle]  :
                      -sineTable[angle - 180]) :
    ((angle <= 540) ? -sineTable[540 - angle]  :
                       sineTable[angle - 540]);
}

// HSV to RGB conversion with custom hue range (0-1535)
// The Larry patterns use 1536 hue units instead of FastLED's 256
void hsvToRgb(int h, byte s, byte v, byte *r, byte *g, byte *b) {
  h %= 1536;
  if (h < 0) h += 1536;

  byte sextant = h >> 8;
  byte frac = h & 255;
  byte vs = (v * s) >> 8;
  byte p = v - vs;
  byte q = v - ((vs * frac) >> 8);
  byte t = v - ((vs * (255 - frac)) >> 8);

  switch (sextant) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

// Cross-fade variables
bool isFading = false;
float fadeAmount = 0.0f;  // 0.0 = current pattern, 1.0 = next pattern
uint8_t fadeFromPattern = 0;
uint8_t fadeToPattern = 0;
unsigned long fadeStartTime = 0;
#define FADE_DURATION_MS 3000  // 3 second fade

// Ultra-Simple Mode System  
enum NodeMode {
  MODE_NORMAL,        // Standalone normal patterns, no sync
  MODE_MUSIC,         // Standalone music-reactive patterns, no sync
  MODE_NORMAL_LEADER, // Normal patterns + broadcast LED data
  MODE_MUSIC_LEADER,  // Music patterns + broadcast LED data
  MODE_FLUFFY         // E1.31/sACN WiFi receiver mode
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

// Fluffy Mode variables
WiFiUDP e131UDP;
IPAddress multicastIP;
bool fluffyWiFiConnected = false;
unsigned long lastFluffyWiFiCheck = 0;
const unsigned long FLUFFY_WIFI_CHECK_INTERVAL = 30000;  // 30s
NodeMode previousNonLeaderMode = MODE_NORMAL;  // Track for leader exit

// Pattern globals
uint8_t gCurrentPatternNumber = 0;
uint8_t gPreviousPatternNumber = 255;  // Track pattern changes for state reset
bool g_patternShouldReset = false;     // Signal to patterns that they should reinitialize
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
uint32_t beatIntervals[50];         // NEW: Track intervals between beats
uint8_t intervalCount = 0;          // NEW: Number of intervals stored
uint32_t lastBeatTime = 0;          // NEW: Timestamp of last detected beat
uint32_t lastBpmMillis = 0;
bool audioDetected = true;
uint8_t musicBrightness = BRIGHTNESS;
unsigned long lastMusicDetectedTime = 0;  // Timestamp of last music detection for sticky behavior
float currentBPM = 0.0f;                   // Smoothed BPM value for display
bool beatReactive = false;                 // NEW: Whether patterns should respond to beats

// Brightness decay envelope for smoother audio response
float brightnessEnvelope = BRIGHTNESS;  // Current decaying brightness level
unsigned long lastBrightnessUpdate = 0;  // For calculating decay time delta
unsigned long lastBeatDetectedTime = 0;  // Track when last beat occurred
#define BRIGHTNESS_DECAY_SECONDS 1.0f    // Time constant for exponential decay (63% falloff) - slower = less jarring
#define BRIGHTNESS_THRESHOLD 0.35f       // Audio must exceed this level to boost brightness (0.0-1.0)
#define BRIGHTNESS_POWER_CURVE 2.0f      // Power curve exponent (1.0=linear, 2.0=square, 3.0=cube)
#define BRIGHTNESS_MIN 18                // Minimum brightness during active music
#define BRIGHTNESS_MAX 50                // Maximum brightness - increased for better dynamic range
#define BRIGHTNESS_IDLE 40               // Brightness when no beats detected
#define NO_BEAT_TIMEOUT 3000             // Restore full brightness after 3 seconds of silence

// Speed envelope for dramatic beat-reactive speed changes
float speedEnvelope = 1.0f;              // Current speed multiplier (1.0 = normal, 3.5 = boosted)
unsigned long lastSpeedUpdate = 0;       // For calculating decay time delta
#define SPEED_BOOST_MULTIPLIER 3.5f      // How much to boost speed on beat (3.5x = very dramatic!)

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
#define LEADER_TIMEOUT_MS 1500  // Fast re-sync: detect missing leader within 1.5s
#define REJOIN_SCAN_INTERVAL_MS 15000  // Scan for leaders every 15 seconds
#define COMPLETE_FRAME_TIMEOUT_MS 5000  // Max time between complete frames before restart

// ===== LARRY PATTERN DECLARATIONS =====
// Four beat-reactive patterns from larry_test_m5stack
void solidColor();      // Random vibrant solid colors (beat triggers new color)
void rainbowLarry();    // Smooth rotating color wheel (BPM-synced speed)
void sineWaveChase();   // Color waves with dramatic dark gaps (BPM-synced motion)
void wavyFlag();        // Animated red/white/blue patriotic pattern (BPM-synced waves)

// Pattern list and names
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {
  solidColor,
  rainbowLarry,
  sineWaveChase,
  wavyFlag
};

const char* patternNames[] = {
  "Solid",
  "Rainbow",
  "SineChase",
  "WavyFlag"
};

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// ===== BEAT-REACTIVE HELPER FUNCTIONS =====
// Calculate speed multiplier from beat interval (0.5x to 3.0x)
// Faster BPM = higher speed multiplier for pattern animations
float getBeatSpeed() {
  uint32_t interval = getMedianInterval();
  if (interval == 0) return 1.0f;  // No beat detected, use default speed

  // Calculate BPM from interval: BPM = 60000 / intervalMs
  // Then scale to 0.5-3.0 range:
  //   60 BPM (1000ms) -> 1.0x (normal speed)
  //   120 BPM (500ms) -> 2.0x (double speed)
  //   180 BPM (333ms) -> 3.0x (triple speed)
  //   30 BPM (2000ms) -> 0.5x (half speed)
  float speed = 1000.0f / (float)interval;
  return constrain(speed, 0.5f, 3.0f);
}

// Adjust animation increment based on beat speed
// Example: getBeatAdjustedInc(2) returns 2 at 60BPM, 4 at 120BPM, 1 at 30BPM
int getBeatAdjustedInc(int baseInc) {
  return (int)(baseInc * getBeatSpeed());
}

// Get SUBTLE beat-reactive speed multiplier for patterns (0.8x to 1.2x)
// More subtle than before for smoother, more predictable motion
float getBeatSpeedMultiplier() {
  if (!audioDetected) return 1.0f;  // No music, use base speed

  uint32_t interval = getMedianInterval();
  if (interval == 0) return 1.0f;  // No beat detected

  // Calculate speed from BPM, but keep it subtle (0.8x to 1.2x range)
  // 60 BPM = 1.0x, 120 BPM = 1.2x, 30 BPM = 0.8x
  float speed = 1000.0f / (float)interval;  // BPM-based speed
  speed = 0.8f + (speed - 0.5f) * 0.2f;  // Map to 0.8-1.2 range
  return constrain(speed, 0.8f, 1.2f);
}

// Get speed multiplier from envelope for dramatic beat-reactive speed changes
// Returns 1.0x (normal) to 3.5x (boosted) with smooth decay
float getSpeedMultiplier() {
  if (!audioDetected) return 1.0f;  // No music, use normal speed
  return speedEnvelope;  // Use the globally tracked speed envelope
}

// Get beat-reactive brightness scale for music mode patterns (0.1 to 1.0)
// Applied BEFORE gamma correction to preserve dark gaps
// Only affects music mode - normal mode always returns 1.0
float getMusicBeatBrightnessScale() {
  if (currentMode != MODE_MUSIC && currentMode != MODE_MUSIC_LEADER) {
    return 1.0f;  // Normal mode - no scaling
  }

  // Always use brightnessEnvelope - it handles idle brightness restoration
  // Map brightnessEnvelope (18-50) to dramatic range (0.1-1.0)
  // Idle/no beats: 40 → ~0.7, Min during beats: 18 → 0.1, Max on beats: 50 → 1.0
  float normalized = (brightnessEnvelope - (float)BRIGHTNESS_MIN) / (float)(BRIGHTNESS_MAX - BRIGHTNESS_MIN);
  float scale = 0.1f + normalized * 0.9f;
  return constrain(scale, 0.1f, 1.0f);
}

// ESP-NOW callbacks
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Commented out to reduce serial spam - only report failures
  // if (status == ESP_NOW_SEND_SUCCESS) {
  //   Serial.println("ESP-NOW: Send OK");
  // } else {
  if (status != ESP_NOW_SEND_SUCCESS) {
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

// ===== FLUFFY MODE FUNCTIONS =====

void enterFluffyMode() {
  Serial.println("*** ENTERING FLUFFY MODE ***");

  // Deinitialize ESP-NOW
  esp_now_deinit();
  delay(100);

  // Connect to WiFi
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(FLUFFY_SSID, FLUFFY_PASSWORD);

  Serial.print("Connecting to WiFi: ");
  Serial.println(FLUFFY_SSID);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  fluffyWiFiConnected = (WiFi.status() == WL_CONNECTED);

  if (fluffyWiFiConnected) {
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());

    // Build multicast IP for Universe 30
    String multicastAddr = String(E131_MULTICAST_BASE) + String(E131_UNIVERSE);
    multicastIP.fromString(multicastAddr);

    Serial.print("E1.31/sACN Universe ");
    Serial.print(E131_UNIVERSE);
    Serial.print(" - Channels ");
    Serial.print(E131_START_CHANNEL);
    Serial.print("-");
    Serial.println(E131_START_CHANNEL + 299);
    Serial.print("Multicast: ");
    Serial.println(multicastIP);

    // Join multicast group
    if (e131UDP.beginMulticast(multicastIP, E131_PORT)) {
      Serial.print("Joined Universe ");
      Serial.print(E131_UNIVERSE);
      Serial.print(" multicast: ");
      Serial.println(multicastIP);
    } else {
      Serial.println("ERROR: Failed to join multicast group!");
    }

    Serial.print("E1.31 listening on port ");
    Serial.println(E131_PORT);
  } else {
    Serial.println("WiFi connection failed!");
  }

  // Clear all LEDs to black
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void exitFluffyMode() {
  Serial.println("*** EXITING FLUFFY MODE ***");

  // Stop E1.31
  e131UDP.stop();

  // Disconnect WiFi
  WiFi.disconnect();
  delay(100);

  fluffyWiFiConnected = false;

  // Reinitialize ESP-NOW
  setupESPNOW();

  // Clear LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void checkFluffyWiFi() {
  unsigned long now = millis();
  if (now - lastFluffyWiFiCheck < FLUFFY_WIFI_CHECK_INTERVAL) return;
  lastFluffyWiFiCheck = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    fluffyWiFiConnected = false;
    WiFi.begin(FLUFFY_SSID, FLUFFY_PASSWORD);
  } else {
    if (!fluffyWiFiConnected) {
      Serial.println("WiFi reconnected!");
    }
    fluffyWiFiConnected = true;
  }
}

void processE131() {
  int packetSize = e131UDP.parsePacket();
  if (packetSize < 126) return;  // Minimum E1.31 packet size

  uint8_t buffer[638];  // E1.31 packet: 126 bytes header + 512 DMX channels
  int len = e131UDP.read(buffer, packetSize);

  // Validate minimum packet size
  if (len < 126) return;

  // Extract universe from framing layer (bytes 113-114, big-endian)
  uint16_t universe = (buffer[113] << 8) | buffer[114];

  // Check if it's our universe
  if (universe != E131_UNIVERSE) return;

  // DMX data starts at byte 126 (after all headers)
  uint8_t* dmxData = buffer + 126;
  int dmxChannels = len - 126;

  // Check we have enough channels
  int startIndex = E131_START_CHANNEL - 1;  // Convert to 0-based
  if (dmxChannels < startIndex + 300) return;

  // Map 300 channels to 100 RGB LEDs
  for (int i = 0; i < 100; i++) {
    int channelIndex = startIndex + (i * 3);
    leds[i].r = dmxData[channelIndex];
    leds[i].g = dmxData[channelIndex + 1];
    leds[i].b = dmxData[channelIndex + 2];
  }

  // Clear remaining 100 LEDs to black
  for (int i = 100; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  FastLED.show();
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

    // Small delay between packets to prevent ESP-NOW buffer overflow
    delayMicroseconds(500);  // 0.5ms between packets
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
  
  // Adaptive sensitivity with aggressive AGC for noisy environments
  float dynamicRange = soundMax - soundMin;
  const float MIN_DYNAMIC_RANGE = 0.15f;  // Increased from 0.08 for more aggressive AGC
  const float HIGH_VOLUME_THRESHOLD = 0.5f;  // Lower threshold to trigger AGC earlier

  float adaptedMin = soundMin;
  float adaptedMax = soundMax;
  float beatThreshold = 0.6f;

  bool highVolumeEnvironment = (soundMin > HIGH_VOLUME_THRESHOLD) || (dynamicRange < MIN_DYNAMIC_RANGE);

  if (highVolumeEnvironment) {
    if (dynamicRange < MIN_DYNAMIC_RANGE) {
      // More aggressive expansion for better dynamic range in noisy environments
      float expansion = (MIN_DYNAMIC_RANGE - dynamicRange) * 1.0f;  // Increased from 0.5 to 1.0
      adaptedMin = max(0.0f, soundMin - expansion);
      adaptedMax = min(1.0f, soundMax + expansion);
    }
    beatThreshold = 0.35f;  // Lower threshold for beat detection in noisy environments
  }
  
  musicLevel = constrain((raw - adaptedMin) / (adaptedMax - adaptedMin + 1e-6f), 0.0f, 1.0f);
  audioLevel = musicLevel;
  
  bool above = (musicLevel > beatThreshold);
  if (above && !prevAbove) {
    uint32_t t = millis();

    // Track beat times (for legacy BPM calculation)
    if (beatCount < 50) {
      beatTimes[beatCount++] = t;
    } else {
      memmove(beatTimes, beatTimes + 1, 49 * sizeof(uint32_t));
      beatTimes[49] = t;
    }

    // NEW: Track beat intervals for improved BPM calculation
    if (lastBeatTime > 0) {
      uint32_t interval = t - lastBeatTime;
      // Only track reasonable intervals (150ms to 2000ms = 30-400 BPM)
      if (interval >= 150 && interval <= 2000) {
        if (intervalCount < 50) {
          beatIntervals[intervalCount++] = interval;
        } else {
          memmove(beatIntervals, beatIntervals + 1, 49 * sizeof(uint32_t));
          beatIntervals[49] = interval;
        }
      }
    }
    lastBeatTime = t;
    lastBeatDetectedTime = t;  // Track for brightness restoration

    beatDetected = true;
  } else if (!above) {
    beatDetected = false;
  }
  prevAbove = above;
}

// Helper function to find median interval (for stable BPM calculation)
uint32_t getMedianInterval() {
  if (intervalCount == 0) return 0;

  // Copy intervals to temp array for sorting
  uint32_t temp[50];
  memcpy(temp, beatIntervals, intervalCount * sizeof(uint32_t));

  // Simple bubble sort (small array, not performance critical)
  for (int i = 0; i < intervalCount - 1; i++) {
    for (int j = 0; j < intervalCount - i - 1; j++) {
      if (temp[j] > temp[j + 1]) {
        uint32_t swap = temp[j];
        temp[j] = temp[j + 1];
        temp[j + 1] = swap;
      }
    }
  }

  // Return median
  if (intervalCount % 2 == 0) {
    return (temp[intervalCount/2 - 1] + temp[intervalCount/2]) / 2;
  } else {
    return temp[intervalCount/2];
  }
}

void updateBPM() {
  uint32_t now = millis();
  if (now - lastBpmMillis >= BPM_WINDOW) {
    // Count beats in window (for music detection)
    int cnt = 0;
    uint32_t cutoff = now - BPM_WINDOW;
    for (int i = 0; i < beatCount; i++) {
      if (beatTimes[i] >= cutoff) cnt++;
    }

    // NEW: INTERVAL-BASED BPM CALCULATION
    // Instead of just counting beats, find the most common interval
    // This locks onto the actual tempo instead of fluctuating
    float bpm = 0.0f;
    if (intervalCount >= 3) {  // Need at least 3 intervals for median
      uint32_t medianInterval = getMedianInterval();
      if (medianInterval > 0) {
        bpm = 60000.0f / float(medianInterval);  // Convert interval to BPM
      }
    } else if (cnt >= 2) {
      // Fallback to count-based if not enough intervals yet
      bpm = cnt * (60000.0f / float(BPM_WINDOW));
    }

    // VERY AGGRESSIVE SMOOTHING - 90% old + 10% new for rock-solid display
    // This prevents BPM from jumping around on the display
    if (currentBPM == 0.0f || currentBPM < 10.0f) {
      currentBPM = bpm;  // First reading or reset, no smoothing
    } else if (bpm > 0.0f) {
      currentBPM = currentBPM * 0.9f + bpm * 0.1f;
    }

    // DEBUG: Print beat info to help diagnose detection issues
    Serial.print("BPM Check: beats=");
    Serial.print(cnt);
    Serial.print(", intervals=");
    Serial.print(intervalCount);
    Serial.print(", rawBPM=");
    Serial.print(bpm);
    Serial.print(", smoothed=");
    Serial.print(currentBPM);
    Serial.print(", audioDetected=");
    Serial.println(audioDetected ? "YES" : "NO");

    // ULTRA-STICKY HYSTERESIS DETECTION - very sensitive and stable
    // Enter music mode: 2+ beats (very low threshold - highly sensitive)
    // Stay in music mode: 1+ beat OR within 20 second timeout (ultra sticky)
    // Exit music mode: 0 beats AND timeout expired (strong momentum)

    bool beatsDetected = (cnt >= 2 && currentBPM >= 30.0f && currentBPM <= 300.0f);  // Enter threshold
    bool sustainBeats = (cnt >= 1 && currentBPM >= 30.0f && currentBPM <= 300.0f);   // Stay threshold

    if (beatsDetected || sustainBeats) {
      lastMusicDetectedTime = now;  // Update timestamp on any beat activity
      audioDetected = true;
    } else {
      // Only exit music mode if no beats for 20 seconds (ultra-sticky timeout)
      if (now - lastMusicDetectedTime > 20000) {
        audioDetected = false;
        currentBPM = 0.0f;  // Reset BPM when exiting music mode
      }
      // Otherwise stay in music mode (strong momentum)
    }

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

  // THRESHOLD AND POWER CURVE - pronounced beats boost brightness dramatically
  // Quieter sounds below threshold stay at base brightness
  // Above threshold, apply power curve for dramatic response
  float targetBrightness;
  if (normalizedLevel < BRIGHTNESS_THRESHOLD) {
    targetBrightness = (float)BRIGHTNESS_MIN;  // Minimum brightness for quiet sounds
  } else {
    // Scale from threshold to 1.0 into 0.0 to 1.0 range
    float scaledLevel = (normalizedLevel - BRIGHTNESS_THRESHOLD) / (1.0f - BRIGHTNESS_THRESHOLD);
    // Apply power curve for dramatic response
    float curved = pow(scaledLevel, BRIGHTNESS_POWER_CURVE);
    // Map to WIDE brightness range (30-200) for VERY visible pulsing!
    targetBrightness = (float)BRIGHTNESS_MIN + (curved * (float)(BRIGHTNESS_MAX - BRIGHTNESS_MIN));
  }

  // SMOOTH DECAY ENVELOPE - requested by Max!
  // Fast attack (instant response to peaks), exponential decay (0.5s time constant)
  unsigned long now = millis();
  float timeDelta = (now - lastBrightnessUpdate) / 1000.0f;  // Convert to seconds
  lastBrightnessUpdate = now;

  if (targetBrightness > brightnessEnvelope) {
    // ATTACK: New peak is higher - instantly jump to it
    brightnessEnvelope = targetBrightness;
  } else {
    // GAUSSIAN/EXPONENTIAL DECAY: Natural smooth falloff
    // Uses time constant tau (BRIGHTNESS_DECAY_SECONDS)
    // Formula: envelope = target + (envelope - target) * exp(-timeDelta / tau)
    float tau = BRIGHTNESS_DECAY_SECONDS;
    float decayFactor = exp(-timeDelta / tau);
    brightnessEnvelope = targetBrightness + (brightnessEnvelope - targetBrightness) * decayFactor;

    // Don't go below minimum brightness
    if (brightnessEnvelope < (float)BRIGHTNESS_MIN) {
      brightnessEnvelope = (float)BRIGHTNESS_MIN;
    }
  }

  // Check if no beats detected for a while - restore to idle brightness
  // Also check if current audio level is low (just background noise, not music)
  bool noBeatsTimeout = (now - lastBeatDetectedTime) > NO_BEAT_TIMEOUT;
  bool lowAudioLevel = (normalizedLevel < BRIGHTNESS_THRESHOLD);

  if (noBeatsTimeout || (lowAudioLevel && brightnessEnvelope < (float)BRIGHTNESS_IDLE * 0.7f)) {
    // No beats for a while OR very low audio - restore to idle brightness for visibility
    brightnessEnvelope = (float)BRIGHTNESS_IDLE;
  }

  // SPEED ENVELOPE - dramatic speed boost on beat with smooth decay
  // Same attack/decay behavior as brightness for consistent feel
  float targetSpeed = beatDetected ? SPEED_BOOST_MULTIPLIER : 1.0f;

  float speedTimeDelta = (now - lastSpeedUpdate) / 1000.0f;
  lastSpeedUpdate = now;

  if (targetSpeed > speedEnvelope) {
    // ATTACK: Instantly boost speed on beat
    speedEnvelope = targetSpeed;
  } else {
    // DECAY: Smooth falloff back to normal speed (same tau as brightness)
    float tau = BRIGHTNESS_DECAY_SECONDS;
    float decayFactor = exp(-speedTimeDelta / tau);
    speedEnvelope = targetSpeed + (speedEnvelope - targetSpeed) * decayFactor;

    // Don't go below 1.0x
    if (speedEnvelope < 1.0f) {
      speedEnvelope = 1.0f;
    }
  }

  musicBrightness = (uint8_t)brightnessEnvelope;
  // NOTE: We don't set FastLED.setBrightness() here anymore!
  // Instead, patterns apply brightness scaling BEFORE gamma in music mode
  // This keeps global brightness constant and preserves dark gaps
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
  brightnessEnvelope = (float)BRIGHTNESS_IDLE;  // Start at idle brightness for visibility
  lastBeatDetectedTime = millis();  // Reset beat timer
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
  brightnessEnvelope = (float)BRIGHTNESS_IDLE;  // Start at idle brightness for visibility
  lastBeatDetectedTime = millis();  // Reset beat timer
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
        // Short press: Cycle modes (Normal → Music → Fluffy → Normal, or Leader toggle)
        Serial.print("Short press from mode: ");
        Serial.println(currentMode);
        if (currentMode == MODE_NORMAL) {
          Serial.println("Switching NORMAL -> MUSIC");
          switchToMusicMode();
        } else if (currentMode == MODE_MUSIC) {
          Serial.println("Switching MUSIC -> FLUFFY");
          previousNonLeaderMode = MODE_MUSIC;
          currentMode = MODE_FLUFFY;
          enterFluffyMode();
        } else if (currentMode == MODE_FLUFFY) {
          Serial.println("Switching FLUFFY -> NORMAL");
          exitFluffyMode();
          currentMode = MODE_NORMAL;
          previousNonLeaderMode = MODE_NORMAL;
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
    // Long press: toggle beat-reactive mode
    beatReactive = !beatReactive;
    bLongHandled = true;
    Serial.print("Beat-reactive mode: ");
    Serial.println(beatReactive ? "ON" : "OFF");
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
      case MODE_FLUFFY:
        backgroundColor = WHITE;
        textColor = BLACK;
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
    case MODE_FLUFFY:
      modeStr = fluffyWiFiConnected ? "FLUFFY (WiFi)" : "FLUFFY (No WiFi)";
      break;
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
    M5.Display.drawString("BPM: " + String((int)currentBPM), 10, 95);
    M5.Display.drawString("BeatFX: " + String(beatReactive ? "ON" : "OFF"), 10, 110);
  } else {
    String patternDisplay = String(gCurrentPatternNumber) + ": " + String(patternNames[gCurrentPatternNumber]);
    M5.Display.drawString(patternDisplay, 10, 50);
  }
}

void nextPattern() {
  // Start cross-fade to next pattern
  fadeFromPattern = gCurrentPatternNumber;
  fadeToPattern = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
  isFading = true;
  fadeAmount = 0.0f;
  fadeStartTime = millis();

  // Signal new pattern to initialize (BEFORE fade starts so it's ready)
  g_patternShouldReset = true;

  Serial.print("Fading from pattern ");
  Serial.print(fadeFromPattern);
  Serial.print(" to ");
  Serial.println(fadeToPattern);

  // Track pattern change for state reset
  gPreviousPatternNumber = gCurrentPatternNumber;
}

// Enhanced leader timeout with rejoin logic
void checkLeaderTimeout() {
  unsigned long now = millis();

  // Check for stuck state: receiving packets but not complete frames
  // Case 1: We've received frames before but haven't gotten one recently
  // Case 2: We've been receiving packets for a while but NEVER got a complete frame
  bool stuckState = false;

  if (leaderDataActive && (now - lastLeaderMessage < LEADER_TIMEOUT_MS)) {
    // We're actively receiving leader packets
    if (lastCompleteFrame > 0 && (now - lastCompleteFrame > COMPLETE_FRAME_TIMEOUT_MS)) {
      // Case 1: Had complete frames before, but not recently
      stuckState = true;
    } else if (lastCompleteFrame == 0 && leaderDataActive && (now - lastLeaderMessage > COMPLETE_FRAME_TIMEOUT_MS)) {
      // Case 2: Never received complete frame but been receiving packets for 5+ seconds
      stuckState = true;
    }
  }

  if (stuckState) {
    // Stuck state detected - restart for clean recovery
    Serial.println("\n!!! CRITICAL: STUCK STATE DETECTED !!!");
    Serial.print("  Time since last complete frame: ");
    if (lastCompleteFrame > 0) {
      Serial.print(now - lastCompleteFrame);
      Serial.println("ms");
    } else {
      Serial.println("NEVER");
    }
    Serial.print("  Time since last leader msg: ");
    Serial.print(now - lastLeaderMessage);
    Serial.println("ms");
    Serial.println("  >>> RESTARTING ESP32 IN 2 SECONDS <<<\n");
    delay(2000);  // Give time for serial message to be sent
    ESP.restart();  // Clean restart - most robust recovery
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

  // Initialize beat detection timer for brightness restoration
  lastBeatDetectedTime = millis();

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

// Handle cross-fade rendering and blending
void updateCrossFade() {
  if (!isFading) return;

  // Update fade progress
  unsigned long elapsed = millis() - fadeStartTime;
  fadeAmount = (float)elapsed / (float)FADE_DURATION_MS;

  if (fadeAmount >= 1.0f) {
    // Fade complete - switch to new pattern
    fadeAmount = 1.0f;
    isFading = false;
    gCurrentPatternNumber = fadeToPattern;
    // DON'T reset pattern - it's already been rendering during fade!
    // g_patternShouldReset = true;  // REMOVED - causes abrupt jump
    Serial.print("Fade complete, now on pattern ");
    Serial.println(gCurrentPatternNumber);
  }
}

// Render current pattern (with cross-fade support)
void renderPattern() {
  if (isFading) {
    // CROSS-FADE MODE: Render both patterns and blend

    // Render old pattern to leds[]
    uint8_t savedPattern = gCurrentPatternNumber;
    gCurrentPatternNumber = fadeFromPattern;
    gPatterns[fadeFromPattern]();

    // Save old pattern result
    CRGB ledsOld[NUM_LEDS];
    memcpy(ledsOld, leds, sizeof(leds));

    // Render new pattern to leds[]
    gCurrentPatternNumber = fadeToPattern;
    gPatterns[fadeToPattern]();

    // Blend: leds = (old * (1-fade)) + (new * fade)
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].r = ((uint16_t)ledsOld[i].r * (uint16_t)((1.0f - fadeAmount) * 255)) / 255 +
                  ((uint16_t)leds[i].r * (uint16_t)(fadeAmount * 255)) / 255;
      leds[i].g = ((uint16_t)ledsOld[i].g * (uint16_t)((1.0f - fadeAmount) * 255)) / 255 +
                  ((uint16_t)leds[i].g * (uint16_t)(fadeAmount * 255)) / 255;
      leds[i].b = ((uint16_t)ledsOld[i].b * (uint16_t)((1.0f - fadeAmount) * 255)) / 255 +
                  ((uint16_t)leds[i].b * (uint16_t)(fadeAmount * 255)) / 255;
    }

    // Restore pattern number (in case it's used elsewhere)
    gCurrentPatternNumber = savedPattern;
  } else {
    // NORMAL MODE: Just render current pattern
    gPatterns[gCurrentPatternNumber]();
  }
}

void loop() {
  // Non-blocking frame rate limiting - 60 FPS without blocking ESP-NOW
  static unsigned long lastFrameTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastFrameTime < 16) {
    yield();
    return;  // Skip this iteration, come back in next loop
  }
  lastFrameTime = currentTime;

  // Reset watchdog timer
  esp_task_wdt_reset();

  M5.update();
  handleButtons();

  // Fluffy mode processing - completely separate from ESP-NOW
  if (currentMode == MODE_FLUFFY) {
    checkFluffyWiFi();
    processE131();
    if (currentTime - lastDisplayUpdate > 200) {
      updateDisplay();
      lastDisplayUpdate = currentTime;
    }
    return;  // Skip all other processing
  }

  handlePatternButtons();

  // Check for leader timeout
  checkLeaderTimeout();

  // Update cross-fade progress
  updateCrossFade();

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
    // Music mode - update audio and run pattern (with cross-fade support)
    updateAudioLevel();
    FastLED.setBrightness(BRIGHTNESS);  // Keep global brightness constant - patterns handle beat scaling
    renderPattern();

    // If leader: broadcast FIRST, then wait for followers to receive/process, then show
    if (currentMode == MODE_MUSIC_LEADER) {
      static unsigned long lastSkipLog = 0;
      if (currentTime - lastBroadcast > BROADCAST_INTERVAL_MS) {
        broadcastLEDData();
        lastBroadcast = currentTime;
        // NO MORE BLOCKING DELAY! Use non-blocking check below instead
      } else if (currentTime - lastSkipLog > 5000) {
        Serial.print("[LEADER] Skipping broadcast (too soon): ");
        Serial.print(currentTime - lastBroadcast);
        Serial.println("ms since last");
        lastSkipLog = currentTime;
      }
      // Non-blocking wait: only show if enough time has passed since last broadcast
      if (currentTime - lastBroadcast >= LEADER_DELAY_MS) {
        FastLED.show();
      }
    } else {
      FastLED.show();  // Non-leader: show immediately
    }
  } else {
    // Normal mode - run pattern (with cross-fade support)
    FastLED.setBrightness(BRIGHTNESS);
    renderPattern();

    // If leader: broadcast FIRST, then wait for followers to receive/process, then show
    if (currentMode == MODE_NORMAL_LEADER) {
      static unsigned long lastSkipLog = 0;
      if (currentTime - lastBroadcast > BROADCAST_INTERVAL_MS) {
        broadcastLEDData();
        lastBroadcast = currentTime;
        // NO MORE BLOCKING DELAY! Use non-blocking check below instead
      } else if (currentTime - lastSkipLog > 5000) {
        Serial.print("[LEADER] Skipping broadcast (too soon): ");
        Serial.print(currentTime - lastBroadcast);
        Serial.println("ms since last");
        lastSkipLog = currentTime;
      }
      // Non-blocking wait: only show if enough time has passed since last broadcast
      if (currentTime - lastBroadcast >= LEADER_DELAY_MS) {
        FastLED.show();
      }
    } else {
      FastLED.show();  // Non-leader: show immediately
    }
  }
  
  // Update display periodically
  if (currentTime - lastDisplayUpdate > 200) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Auto-advance patterns every 30 seconds in all modes (if enabled)
  if (autoAdvancePatterns && currentTime - lastPatternChange > 30000) {
    nextPattern();
    lastPatternChange = currentTime;
  }
  
  // Increment hue for patterns
  EVERY_N_MILLISECONDS(20) { gHue++; }

  yield();
}

// ===== LARRY PATTERN IMPLEMENTATIONS =====
// Four beat-reactive patterns from larry_test_m5stack
// All patterns apply gamma correction for rich colors and dramatic dark gaps

// Pattern 0: Solid Color
// Music mode: Beat triggers smooth fade to new random color
// Normal mode: Slowly fades through rainbow colors
void solidColor() {
  static CRGB currentColor = CRGB(255, 0, 0);
  static bool lastBeatState = false;
  static int currentHue = 0;   // Current hue position
  static int targetHue = 0;    // Target hue (for smooth transitions)

  // Check if pattern should reset (freshly selected)
  if (g_patternShouldReset) {
    currentHue = random(1536);
    targetHue = currentHue;
    lastBeatState = false;
    g_patternShouldReset = false;
  }

  // Check mode (not audioDetected, which can be true in any mode)
  if (currentMode == MODE_MUSIC || currentMode == MODE_MUSIC_LEADER) {
    // MUSIC MODE: Beat triggers new target color, then smoothly fade to it
    if (beatDetected && !lastBeatState) {
      targetHue = random(1536);  // Set new target color on beat
    }
    lastBeatState = beatDetected;

    // Smoothly interpolate current hue toward target hue
    if (currentHue != targetHue) {
      int diff = targetHue - currentHue;
      // Handle wrapping (shortest path around color wheel)
      if (diff > 768) diff -= 1536;
      if (diff < -768) diff += 1536;

      // Move VERY slowly toward target (about 1-2% per frame for smooth fade)
      // At 60 FPS, this takes about 1 second to complete the transition
      int step = diff / 60;
      if (step == 0) step = (diff > 0) ? 1 : -1;  // Minimum step

      currentHue += step;
      if (currentHue < 0) currentHue += 1536;
      if (currentHue >= 1536) currentHue -= 1536;
    }
  } else {
    // NORMAL MODE: Slowly fade through rainbow colors
    currentHue += 2;  // Slow continuous rotation
    if (currentHue >= 1536) currentHue -= 1536;
    targetHue = currentHue;  // Keep in sync
  }

  // Convert current hue to RGB
  byte r, g, b;
  hsvToRgb(currentHue, 255, 255, &r, &g, &b);

  // Apply beat brightness scaling in music mode (BEFORE gamma!)
  float beatScale = getMusicBeatBrightnessScale();
  r = (byte)(r * beatScale);
  g = (byte)(g * beatScale);
  b = (byte)(b * beatScale);

  // Apply gamma correction (MUST be last!)
  r = gammaTable[r];
  g = gammaTable[g];
  b = gammaTable[b];
  currentColor = CRGB(r, g, b);

  // Fill all LEDs with processed color
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = currentColor;
  }
}

// Pattern 1: Rainbow Larry
// Smooth rotating color wheel - dramatic beat-reactive speed boost
void rainbowLarry() {
  static int colorOffset = 0;
  static int totalHueSpan = (1 + random(4 * ((NUM_LEDS + 31) / 32))) * 1536;
  static int baseIncrement = 4 + random(abs(totalHueSpan) / NUM_LEDS);

  // Check if pattern should reset (freshly selected)
  if (g_patternShouldReset) {
    colorOffset = 0;
    totalHueSpan = (1 + random(4 * ((NUM_LEDS + 31) / 32))) * 1536;
    // Medium base speed: 4-8 (will be boosted to 10-20 on beats)
    baseIncrement = 4 + random(5);
    if (random(2) == 0) totalHueSpan = -totalHueSpan;
    if (random(2) == 0) baseIncrement = -baseIncrement;
    g_patternShouldReset = false;
  }

  // Apply speed envelope for dramatic beat-reactive speed boost (1.0x to 3.5x)
  int increment = (int)(baseIncrement * getSpeedMultiplier());

  // Get beat brightness scale for music mode
  float beatScale = getMusicBeatBrightnessScale();

  // Render rainbow across all LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    int hue = colorOffset + totalHueSpan * i / NUM_LEDS;
    byte r, g, b;
    hsvToRgb(hue, 255, 255, &r, &g, &b);

    // Apply beat brightness in music mode (BEFORE gamma!)
    r = (byte)(r * beatScale);
    g = (byte)(g * beatScale);
    b = (byte)(b * beatScale);

    // Apply gamma correction (MUST be last!)
    r = gammaTable[r];
    g = gammaTable[g];
    b = gammaTable[b];
    leds[i] = CRGB(r, g, b);
  }

  colorOffset += increment;
}

// Pattern 2: Sine Wave Chase
// Color waves with dramatic dark gaps - dramatic beat-reactive speed boost
void sineWaveChase() {
  static int baseHue = random(1536);
  static int waveSpan = (1 + random(4 * ((NUM_LEDS + 31) / 32))) * 720;
  static int baseIncrement = 4 + random(20);
  static int waveOffset = 0;

  // Check if pattern should reset (freshly selected)
  if (g_patternShouldReset) {
    baseHue = random(1536);
    waveSpan = (1 + random(4 * ((NUM_LEDS + 31) / 32))) * 720;
    // Medium base speed: 3-6 (will be boosted to 18-36 on beats with 6x multiplier!)
    baseIncrement = 3 + random(4);
    if (random(2) == 0) baseIncrement = -baseIncrement;
    waveOffset = 0;
    g_patternShouldReset = false;
  }

  // Apply EXTRA dramatic speed boost for sine wave pattern
  // Sine wave gets 2x extra multiplier: (1.0x to 3.5x) * 2.0 = (1.0x to 6x)!
  float speedMult = getSpeedMultiplier();
  if (speedMult > 1.0f) {
    speedMult = 1.0f + (speedMult - 1.0f) * 2.0f;  // Amplify the boost by 2x
  }
  int increment = (int)(baseIncrement * speedMult);

  // Get beat brightness scale for music mode
  float beatScale = getMusicBeatBrightnessScale();

  // Render sine wave pattern
  for (int i = 0; i < NUM_LEDS; i++) {
    // Calculate sine value using fixed-point math (-127 to +127)
    signed char sineValue = fixSin(waveOffset + waveSpan * i / NUM_LEDS);

    byte r, g, b;
    if (sineValue >= 0) {
      // Positive sine: vary saturation (254 - foo*2), full brightness
      byte sat = 254 - (sineValue * 2);
      hsvToRgb(baseHue, sat, 255, &r, &g, &b);
    } else {
      // Negative sine: full saturation, vary brightness (254 + foo*2)
      // THIS IS WHERE THE DARK GAPS COME FROM!
      byte val = 254 + sineValue * 2;  // sineValue is negative, so this reduces brightness
      hsvToRgb(baseHue, 255, val, &r, &g, &b);
    }

    // Apply beat brightness in music mode (BEFORE gamma!)
    r = (byte)(r * beatScale);
    g = (byte)(g * beatScale);
    b = (byte)(b * beatScale);

    // Apply gamma correction (critical for dark gaps!)
    r = gammaTable[r];
    g = gammaTable[g];
    b = gammaTable[b];
    leds[i] = CRGB(r, g, b);
  }

  waveOffset += increment;
}

// Pattern 3: Wavy Flag
// Animated red/white/blue patriotic pattern - BPM-synced flag waves
void wavyFlag() {
  // Flag pattern data
  static const byte flagTable[] = {
    160, 0, 0,    255, 255, 255,  160, 0, 0,    255, 255, 255,  // Red, White, Red, White
    160, 0, 0,    255, 255, 255,  160, 0, 0,                     // Red, White, Red
    0, 0, 100,    255, 255, 255,  0, 0, 100,    255, 255, 255,  // Blue, White, Blue, White
    0, 0, 100,    255, 255, 255,  0, 0, 100,                     // Blue, White, Blue
    255, 255, 255, 160, 0, 0,     255, 255, 255, 160, 0, 0,      // White, Red, White, Red
    255, 255, 255, 160, 0, 0                                     // White, Red
  };

  static int waveLength = 720 + random(720);
  static int baseIncrement = 4 + random(10);
  static int stripeWidth = 200 + random(200);
  static int wavePhase = 0;

  // Check if pattern should reset (freshly selected)
  if (g_patternShouldReset) {
    waveLength = 720 + random(720);
    // Slower base speed: 1-3 (will be boosted to 2.5-7.5 on beats)
    baseIncrement = 1 + random(3);
    stripeWidth = 200 + random(200);
    wavePhase = 0;
    g_patternShouldReset = false;
  }

  // Apply speed envelope for dramatic beat-reactive speed boost (1.0x to 3.5x)
  int increment = (int)(baseIncrement * getSpeedMultiplier());

  // Get beat brightness scale for music mode
  float beatScale = getMusicBeatBrightnessScale();

  // Calculate total arc length with wave deformation
  long sum = 0;
  for (int i = 0; i < NUM_LEDS - 1; i++) {
    sum += stripeWidth + fixCos(wavePhase + waveLength * i / NUM_LEDS);
  }

  // Render wavy flag pattern
  long s = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    // Calculate position along flag pattern with wave deformation
    long x = 256L * ((sizeof(flagTable) / 3) - 1) * s / sum;
    int idx1 = (x >> 8) * 3;
    int idx2 = ((x >> 8) + 1) * 3;
    byte b = (x & 255) + 1;
    byte a = 257 - b;

    // Interpolate between flag colors
    byte r = ((flagTable[idx1] * a) + (flagTable[idx2] * b)) >> 8;
    byte g = ((flagTable[idx1 + 1] * a) + (flagTable[idx2 + 1] * b)) >> 8;
    byte bl = ((flagTable[idx1 + 2] * a) + (flagTable[idx2 + 2] * b)) >> 8;

    // Apply beat brightness in music mode (BEFORE gamma!)
    r = (byte)(r * beatScale);
    g = (byte)(g * beatScale);
    bl = (byte)(bl * beatScale);

    // Apply gamma correction (MUST be last!)
    r = gammaTable[r];
    g = gammaTable[g];
    bl = gammaTable[bl];
    leds[i] = CRGB(r, g, bl);

    s += stripeWidth + fixCos(wavePhase + waveLength * i / NUM_LEDS);
  }

  wavePhase += increment;
  if (wavePhase >= 720) wavePhase -= 720;
}