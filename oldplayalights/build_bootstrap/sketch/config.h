#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/config.h"
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
#define NUM_LEDS        300
#define COLOR_ORDER     GRB
#define CHIPSET         WS2812B
#define FRAME_DELAY_MS  20

static constexpr size_t MIC_BUF_LEN = 240;
static constexpr int      MIC_SR     = 44100;

// ── Enums ─────────────────────────────────────────────────────────────────────
enum Mode     { AUTO = 0, GO, QUIET, MODE_COUNT };
enum FsmState { FOLLOWER = 1, ELECT, LEADER };
enum Control  { STYLE=0, SPEED, BRIGHT, SSENS, BSENS, VSENS, DECAY, TIME, CTRL_COUNT };

// ── Network Config ────────────────────────────────────────────────────────────
#define MSGTYPE_RAW    0x00
#define MSGTYPE_TOKEN  0x01

// ── WiFi Configuration ────────────────────────────────────────────────────────
// TODO: Change these to your WiFi network credentials
#define WIFI_SSID     "Barn"     // Replace with your WiFi name
#define WIFI_PASSWORD "3576wifi"    // Replace with your WiFi password

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
extern const char* STYLE_NAMES[22];

// ── Global Variables ──────────────────────────────────────────────────────────
extern Mode      currentMode;
extern FsmState  fsmState;
extern uint8_t   styleIdx;
extern bool      freezeActive;
extern CRGB      leds[NUM_LEDS];
extern LGFX_Sprite canvas;
extern Preferences prefs;

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