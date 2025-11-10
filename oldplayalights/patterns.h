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

// ── NEW PATTERNS: Inspired by Pixelblaze Community ──────────────────────────
void styleTwinkleStars(uint8_t sp);
void styleRainbowRipples(uint8_t sp);
void styleDNAHelix(uint8_t sp);
void styleNeonPulse(uint8_t sp);
void styleDigitalRain(uint8_t sp);
void stylePlasmaBalls(uint8_t sp);
void styleLightningStorm(uint8_t sp);
void styleKaleidoscope(uint8_t sp);
void styleCandleFlicker(uint8_t sp);
void styleColorDrips(uint8_t sp);
void styleGalaxySpiral(uint8_t sp);
void stylePrism(uint8_t sp);
void styleHeartbeat(uint8_t sp);
void styleAuroraBoreal(uint8_t sp);
void styleMatrixCode(uint8_t sp);
void styleCrystalCave(uint8_t sp);
void styleLavaFlow(uint8_t sp);
void styleWaveform(uint8_t sp);

// ── Effect Control Functions ──────────────────────────────────────────────────
void effectWild();
void effectWildBG();
void effectMusic();
void runTimed(void (*fn)());

// ── Crossfade System ──────────────────────────────────────────────────────────
void runTimedWithCrossfade(void (*fn)());
void executePattern(uint8_t patternIndex, CRGB* buffer);

#endif