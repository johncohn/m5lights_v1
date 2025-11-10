#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/patterns.h"
#ifndef PATTERNS_H
#define PATTERNS_H

#include "config.h"

// ── Pattern Function Declarations ────────────────────────────────────────────
void styleRainbow(uint8_t sp);
void styleChase(uint8_t sp);
void styleJuggle(uint8_t sp);
void styleRainbowGlitter(uint8_t sp);
void styleConfetti(uint8_t sp);
void styleBPM(uint8_t sp);
void styleFire(uint8_t sp);
void styleColorWheel(uint8_t sp);
void styleRandom(uint8_t sp);
void stylePulseWave(uint8_t sp);
void styleMeteorShower(uint8_t sp);
void styleColorSpiral(uint8_t sp);
void stylePlasmaField(uint8_t sp);
void styleSparkleStorm(uint8_t sp);
void styleAuroraWaves(uint8_t sp);
void styleOrganicFlow(uint8_t sp);
void styleWaveCollapse(uint8_t sp);
void styleColorDrift(uint8_t sp);
void styleLiquidRainbow(uint8_t sp);
void styleSineBreath(uint8_t sp);
void styleFractalNoise(uint8_t sp);
void styleRainbowStrobe(uint8_t sp);

// ── Effect Control Functions ──────────────────────────────────────────────────
void effectWild();
void effectWildBG();
void effectMusic();
void runTimed(void (*fn)());

#endif