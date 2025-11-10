#ifndef CONFIG_H
#define CONFIG_H

#include <M5Unified.h>
#include <FastLED.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoOTA.h>
#include <math.h>

// ── Hardware Config ──────────────────────────────────────────────────────────
#define LED_PIN         33
#define NUM_LEDS        334
#define COLOR_ORDER     GRB
#define CHIPSET         WS2812B
#define FRAME_DELAY_MS  20

static constexpr size_t MIC_BUF_LEN = 240;
static constexpr int      MIC_SR     = 44100;

// ── Debug Control ────────────────────────────────────────────────────────────
#define DEBUG_SERIAL    1     // Set to 0 to disable most serial output
#define DEBUG_HEARTBEAT 0     // Set to 1 to enable heartbeat messages
#define DEBUG_BPM       0     // Set to 1 to enable BPM messages

// ── Enums ─────────────────────────────────────────────────────────────────────
enum Mode     { AUTO = 0, OFF, MODE_COUNT };
enum FsmState { FOLLOWER = 1, ELECT, LEADER };
enum Control  { STYLE=0, SPEED, BRIGHT, SSENS, BSENS, VSENS, DECAY, TIME, CTRL_COUNT };

// ── Brightness Level Structure ───────────────────────────────────────────────
struct BrightnessLevel {
  const char* name;
  uint8_t scale;        // 0-255 global brightness
};

// ── Network Config ────────────────────────────────────────────────────────────
#define MSGTYPE_RAW           0x00
#define MSGTYPE_TOKEN         0x01
#define MSGTYPE_OTA_SUSPEND   0x02  // Request all nodes to suspend ESP-NOW for OTA
#define MSGTYPE_OTA_RESUME    0x03  // Request all nodes to resume ESP-NOW after OTA

// ── WiFi Configuration (now handled in networking.cpp) ───────────────────────
// WiFi networks are now defined in networking.cpp to support multiple networks

static const uint32_t LEADER_TIMEOUT        = 1500;
static const uint32_t ELECTION_BASE_DELAY   = 200;
static const uint32_t ELECTION_JITTER       = 50;
static const uint32_t ELECTION_TIMEOUT      = ELECTION_BASE_DELAY + ELECTION_JITTER + 50;
static const uint32_t LEADER_TOKEN_INTERVAL = FRAME_DELAY_MS;
static const uint32_t LEADER_HEARTBEAT_INTERVAL = 100;

// ── Audio Config ──────────────────────────────────────────────────────────────
static constexpr float SMOOTH = 0.995f;
static constexpr uint32_t BPM_WINDOW = 5000;

// ── Names ─────────────────────────────────────────────────────────────────────
extern const char* MODE_NAMES[MODE_COUNT];
extern const char* STYLE_NAMES[42];
extern BrightnessLevel brightnessLevels[6];

// ── Global Variables ──────────────────────────────────────────────────────────
extern Mode      currentMode;
extern FsmState  fsmState;
extern uint8_t   styleIdx;
extern bool      freezeActive;
extern CRGB      leds[NUM_LEDS];
extern LGFX_Sprite canvas;
extern Preferences prefs;

// ── Global Brightness Control ────────────────────────────────────────────────
extern uint8_t   globalBrightnessScale;  // 0-255, runtime adjustable

// ── Control Arrays ────────────────────────────────────────────────────────────
extern uint8_t speedVals[MODE_COUNT][22], brightVals[MODE_COUNT][22],
               ssensVals[MODE_COUNT][22], bsensVals[MODE_COUNT][22],
               vsensVals[MODE_COUNT][22], decayVals[MODE_COUNT][22],
               timeVals[MODE_COUNT][22];

// ── Network Variables ─────────────────────────────────────────────────────────
extern uint8_t  broadcastAddress[6];
extern uint32_t masterSeq, chunkMask, lastRecvMillis;
extern uint32_t electionStart, electionEnd;
extern uint32_t myToken, highestTokenSeen, myDelay;
extern bool     electionBroadcasted;
extern uint32_t lastTokenBroadcast, lastHeartbeat, missedFrameCount;

// ── OTA Coordination Variables ───────────────────────────────────────────────
extern bool     otaSuspended;           // True when ESP-NOW is suspended for OTA
extern uint32_t otaSuspendTimeout;      // When to auto-resume if OTA fails
extern uint32_t otaSuspendDuration;     // How long to suspend (default 300 seconds)
extern uint32_t bootupQuietTime;        // How long to stay quiet after boot (prevent OTA interference)

// ── Audio Variables ───────────────────────────────────────────────────────────
extern float   soundMin, soundMax, musicLevel;
extern bool    prevAbove;
extern uint32_t beatTimes[50];
extern uint8_t  beatCount;
extern uint32_t lastBpmMillis;
extern bool    audioDetected;

// ── Helper Functions ──────────────────────────────────────────────────────────
inline uint8_t getSpeed()  { return speedVals[currentMode][styleIdx]; }
inline uint8_t getBright() { return brightVals[currentMode][styleIdx]; }
inline uint8_t getSS()     { return ssensVals[currentMode][styleIdx]; }
inline uint8_t getBS()     { return bsensVals[currentMode][styleIdx]; }
inline uint8_t getVS()     { return vsensVals[currentMode][styleIdx]; }
inline uint8_t getDe()     { return decayVals[currentMode][styleIdx]; }
inline uint8_t getTi()     { return timeVals[currentMode][styleIdx]; }

#endif