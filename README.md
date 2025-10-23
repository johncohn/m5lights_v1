# M5 Lights v1

FastLED demo reel adapted for M5StickC Plus 2 with WS2811/WS2812 LED strips.

## Hardware Setup

- **Device**: M5StickC Plus 2
- **LED Strip**: WS2811/WS2812 compatible (250 LEDs)
- **Connection**: LED data wire to G32 pin on Grove connector (white wire)

## Features

- 6 different LED animation patterns:
  - Rainbow
  - Theater chase with rainbow
  - Juggle (8 colored dots weaving)
  - Rainbow with glitter
  - Confetti (random colored speckles)
  - BPM (beat-synchronized stripes)

- **Controls**:
  - Button A: Manual pattern cycling
  - Auto-cycle: Changes pattern every 5 seconds

- **Display**: Shows current pattern info on M5StickC Plus 2 screen

## Libraries Required

- M5StickCPlus2
- FastLED
- M5Unified (dependency)
- M5GFX (dependency)

## Configuration

- `NUM_LEDS`: Set to 250 (adjust for your strip length)
- `DATA_PIN`: 32 (G32 on Grove connector)
- `BRIGHTNESS`: 128 (50% brightness)
- `FRAMES_PER_SECOND`: 400

## Installation

1. Install required libraries in Arduino IDE
2. Select board: M5Stack → M5StickC Plus 2
3. Upload the sketch
4. Connect LED strip data wire to G32

## Original Code

Based on FastLED "100 lines of code" demo reel by Mark Kriegsman, adapted for M5StickC Plus 2.