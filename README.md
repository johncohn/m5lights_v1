# M5 Lights v5.1.0

Advanced LED control system for M5StickC Plus 2 with WS2811/WS2812 LED strips. Features wireless multi-device synchronization via ESP-NOW, E1.31/sACN WiFi receiver mode, music reactivity with dramatic audio-responsive brightness and speed, and 11 stunning LED patterns.

## Hardware Setup

- **Device**: M5StickC Plus 2 (with built-in microphone)
- **LED Strip**: WS2811/WS2812 compatible (200 LEDs)
- **Connection**: LED data wire to GPIO 32 (G32 on Grove connector)
- **Power**: Ensure adequate power supply for LED strip

## Features

### Operating Modes

The system uses **color-coded LCD backgrounds** to indicate the current mode:

#### 🟢 **Normal Mode (GREEN background)**
- Runs LED patterns locally and independently
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)
- Standalone operation, no wireless sync

#### 🟣 **Music Mode (PURPLE background)**
- LED patterns react to music/audio in real-time
- Uses built-in M5StickC Plus 2 microphone
- **Dramatic Audio Reactivity**:
  - Brightness range: 8-80 (0.02-1.0 scale) for extreme contrast
  - Speed envelope: 0.3x-3.0x for visible beat-reactive speed changes
  - Automatic brightness restoration after 3 seconds of silence
  - Aggressive AGC for beat detection even in noisy environments
  - Exponential attack/decay envelope for smooth transitions
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)
- Standalone operation, no wireless sync

#### 🟠 **Normal Leader Mode (ORANGE background)**
- Runs LED patterns locally
- **Broadcasts LED states** to all nearby follower devices via ESP-NOW
- Updates followers every 50ms (20 Hz)
- All follower devices mirror the leader's exact LED display
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)

#### 🔴 **Music Leader Mode (RED background)**
- LED patterns react to music/audio in real-time
- **Broadcasts LED states AND brightness** to all nearby follower devices
- Followers mirror both the LED patterns and the music-reactive brightness/speed
- All followers synchronize to the leader's microphone audio
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)

#### ⚪ **Fluffy Mode (WHITE background)** - E1.31/sACN WiFi Receiver
- **Receives ArtNet/E1.31/sACN DMX data over WiFi**
- Displays DMX data from lighting control software (QLab, ETC EOS, GrandMA, etc.)
- **WiFi Network**: "GMA-WIFI_Access_Point" (password: "3576wifi")
- **Protocol**: E1.31/sACN on port 5568
- **Universe**: 30 (multicast address 239.255.0.30)
- **Channels**: 1-300 (100 LEDs × RGB)
- **LED Mapping**: First 100 LEDs display DMX data, remaining 100 stay black
- **Isolation**: ESP-NOW is deinitialized in this mode (mutually exclusive)
- WiFi reconnection every 30 seconds if connection lost
- White screen shows "Fluffy (WiFi)" when connected, "Fluffy (No WiFi)" when disconnected

### Controls

#### Button A (Main Button)

**Short Press** - Mode Cycling:
- Green (Normal) → Purple (Music) → White (Fluffy) → Green
- Orange (Normal Leader) ↔ Red (Music Leader)

**Long Press (1.5 seconds)** - Leader Mode Toggle:
- Green (Normal) → Orange (Normal Leader)
- Purple (Music) → Red (Music Leader)
- Orange/Red (Leader modes) → Previous non-leader mode

**Leader Toggle Behavior**:
- Short press from Orange ↔ Red toggles between leader types
- Long press from Orange or Red exits back to previous non-leader mode (Green or Purple)

#### Button B (Side Button)
- **Short Press**: Manually advance to next pattern
- **Long Press (1 second)**: Toggle auto-advance patterns ON/OFF
- **Super Long Press (3 seconds)**: Toggle Debug Mode ON/OFF
  - When toggled, LEDs flash GREEN (enabled) or RED (disabled)
  - Debug mode shows detailed ESP-NOW packet reception and sync information via Serial

#### Button C (Power Button)
- Standard M5StickC Plus 2 power functions

### LED Animation Patterns

11 different LED animation patterns with music-reactive speed changes:

1. **Rainbow** - FastLED's built-in rainbow generator
2. **Chase** - Theater-style crawling lights with rainbow effect and trailing tails
3. **Juggle** - 8 colored dots weaving in and out of sync
4. **Rainbow + Glitter** - Rainbow with random sparkly white glitter
5. **Confetti** - Random colored speckles that blink in and fade smoothly
6. **BPM** - Colored stripes pulsing at a defined beats-per-minute
7. **Fire** - Realistic fire simulation with heat algorithm
8. **Lightning** - Lightning storm with random strikes and secondary bolts
9. **Plasma** - Plasma field effect with flowing sine waves
10. **Meteors** - Meteor shower with trailing tails
11. **Aurora** - Aurora borealis waves with shifting colors

**In Music/Music Leader modes**, all patterns have:
- **Beat-reactive speed**: 0.3x (slow baseline) to 3.0x (on beat) for visible tempo changes
- **Beat-reactive brightness**: 8-80 range (0.02-1.0 scale) for dramatic contrast
- **Smooth decay**: Exponential envelope for natural-feeling transitions

### Display

The M5StickC Plus 2 LCD shows:
- **Background color** indicating current mode (GREEN/PURPLE/ORANGE/RED/WHITE)
- Application title and version number
- Current operating mode name
- Current pattern number and name
- In Music modes: Audio level percentage and beat detection status
- In Fluffy mode: WiFi connection status

### Music Reactive System

The music reactive modes (Purple and Red) use advanced audio processing:

#### Audio Processing
- **Sample Rate**: 44.1 kHz for high-quality audio capture
- **Buffer Size**: 240 samples per frame
- **Microphone**: Built-in M5StickC Plus 2 microphone (on bottom of device)

#### Beat Detection & AGC
- **Adaptive Sensitivity**: Automatically adjusts to different volume environments
- **Aggressive AGC**: Detects beats even with high background noise
- **Minimum Dynamic Range**: 0.15 for consistent beat detection
- **High Volume Threshold**: 0.5 trigger point for AGC expansion
- **Beat Threshold**: 0.35 in noisy environments, 0.6 in quiet
- **BPM Validation**: 30-300 BPM range

#### Brightness Envelope
- **Range**: 8-80 (very dark to very bright)
- **Scale**: 0.02-1.0 (extreme contrast for dramatic effects)
- **Idle Brightness**: 50 (when no beats detected)
- **Restoration**: Auto-restore to idle brightness after 3 seconds of silence
- **Attack**: Instant boost on beat detection
- **Decay**: 1.0 second exponential falloff (τ = 1.0s)

#### Speed Envelope
- **Base Speed**: 0.3x (slow baseline for contrast)
- **Beat Speed**: 3.0x (triple speed on beat)
- **Attack**: Instant speed boost on beat
- **Decay**: 1.0 second exponential falloff (same as brightness)
- **Effect**: Visible tempo changes without "white blur" artifacts

#### Audio Validation
- **Noise Floor Tracking**: Moving average with 99.5% smoothing (SMOOTH = 0.995)
- **Peak Level Tracking**: Captures and adapts to loud sound peaks
- **Audio Detection**: Validates actual audio presence (not just noise)

### E1.31/sACN Integration (Fluffy Mode)

Fluffy mode turns the M5Stick into a WiFi DMX receiver for professional lighting control:

#### Network Configuration
- **WiFi SSID**: "GMA-WIFI_Access_Point"
- **WiFi Password**: "3576wifi"
- **Protocol**: E1.31/sACN (Streaming ACN)
- **Port**: 5568 (standard E1.31 port)
- **Transport**: Multicast UDP

#### DMX Configuration
- **Universe**: 30
- **Multicast Address**: 239.255.0.30 (auto-calculated from universe)
- **Start Channel**: 1
- **Channel Count**: 300 (100 LEDs × 3 channels RGB)

#### LED Mapping
- **Channels 1-3**: LED 0 (R, G, B)
- **Channels 4-6**: LED 1 (R, G, B)
- ...
- **Channels 298-300**: LED 99 (R, G, B)
- **LEDs 100-199**: Forced to black (unused)

#### Features
- **Auto-Reconnect**: WiFi reconnection every 30 seconds if disconnected
- **Packet Validation**: Verifies ArtNet ID, OpCode, universe, and length
- **ESP-NOW Isolation**: ESP-NOW deinitialized when entering Fluffy mode, reinitialized on exit
- **Clean Transitions**: All LEDs cleared to black on mode entry/exit

#### Compatible Software
Works with any E1.31/sACN lighting control software:
- QLab (theater/events)
- ETC EOS (professional lighting consoles)
- GrandMA (concert/touring)
- Chamsys MagicQ
- LightKey
- Luminair
- Many others...

## Multi-Device Synchronized Light Show Setup

### Basic Setup (2+ devices)

1. **Setup Leader Device**:
   - Upload the code to your first M5StickC Plus 2
   - Choose your mode: Green (Normal) or Purple (Music)
   - Long press Button A (1.5 seconds) to become a Leader
   - Display will change to **ORANGE** (Normal Leader) or **RED** (Music Leader)

2. **Setup Follower Device(s)**:
   - Upload the code to additional M5StickC Plus 2 devices
   - Power them on (they start in Normal mode - GREEN background)
   - They will automatically detect the Leader and display synchronized patterns
   - All followers will mirror the Leader's LED patterns exactly

3. **Switching Between Modes**:
   - **Change Leader Type**: Short press Button A on the Leader to toggle Orange ↔ Red
   - **Stop Being Leader**: Long press Button A on the Leader to return to standalone mode
   - **Cycle Standalone Modes**: Short press Button A to cycle Green → Purple → White → Green

### Advanced: Music Synchronized Show

For a music-reactive synchronized light show across multiple devices:

1. Position the Leader device near the music source
2. Put the Leader in **Music Leader mode (RED)**:
   - Start in Music mode (PURPLE) and long press Button A, OR
   - Start in Normal Leader mode (ORANGE) and short press Button A
3. All followers will mirror both the patterns AND the music-reactive brightness/speed
4. The entire system syncs to the Leader's microphone input

### E1.31/sACN Lighting Control Integration

For professional DMX lighting control software integration:

1. Configure your lighting software:
   - **Protocol**: E1.31/sACN (Streaming ACN)
   - **Universe**: 30
   - **Start Address**: Channel 1
   - **Device Type**: Generic RGB fixture (100 LEDs = 300 channels)
   - **Output Mode**: Multicast (recommended) or Unicast

2. On the M5Stick:
   - Short press Button A to cycle to **Fluffy Mode (WHITE background)**
   - Wait for "Fluffy (WiFi)" confirmation on display
   - Device is now ready to receive DMX data

3. Send DMX data from your lighting software
   - First 100 LEDs will display the DMX values
   - Remaining 100 LEDs stay black

4. Exit Fluffy Mode:
   - Short press Button A to cycle back to Green (Normal) mode
   - ESP-NOW will reinitialize automatically

## Configuration

```cpp
// Hardware
#define LED_PIN 32              // GPIO 32 (Grove connector)
#define NUM_LEDS 200            // Adjust for your strip length
#define BRIGHTNESS 22           // Default brightness (0-255)
#define CHIPSET WS2811          // LED chipset type
#define COLOR_ORDER GRB         // Color channel order

// Timing
#define LONG_PRESS_TIME_MS 1500           // Button long press duration
#define BROADCAST_INTERVAL_MS 50          // Leader broadcast rate (20 Hz)
#define LEADER_TIMEOUT_MS 8000            // Follower timeout period

// Audio (Music modes)
#define MIC_BUF_LEN 240                   // Microphone buffer length
#define MIC_SR 44100                      // Sample rate
#define SMOOTH 0.995f                     // Audio smoothing factor (99.5%)
#define BRIGHTNESS_MIN 8                  // Minimum brightness (very dark)
#define BRIGHTNESS_MAX 80                 // Maximum brightness (very bright)
#define BRIGHTNESS_IDLE 50                // Idle brightness when no beats
#define NO_BEAT_TIMEOUT 3000              // Restore idle brightness after 3s
#define SPEED_BASE 0.3f                   // Minimum speed multiplier
#define SPEED_BOOST_MULTIPLIER 3.0f       // Maximum speed multiplier on beat

// E1.31/sACN (Fluffy mode)
#define FLUFFY_SSID "GMA-WIFI_Access_Point"
#define FLUFFY_PASSWORD "3576wifi"
#define E131_PORT 5568                    // Standard E1.31 port
#define E131_UNIVERSE 30                  // DMX universe
#define E131_START_CHANNEL 1              // Start at channel 1
```

## Libraries Required

- **M5StickCPlus2** - M5Stack official library
- **FastLED** - FastLED library for LED control
- **M5Unified** (dependency) - M5Stack unified library
- **M5GFX** (dependency) - M5Stack graphics library
- **ESP-NOW** (built into ESP32) - ESP32 wireless protocol
- **WiFi** (built into ESP32) - ESP32 WiFi library for Fluffy mode

## Installation

1. Install required libraries in Arduino IDE:
   - M5StickCPlus2
   - FastLED
   - M5Unified
   - M5GFX

2. Select board: **M5Stack → M5StickC Plus 2**

3. Upload the sketch to your M5StickC Plus 2

4. Connect LED strip:
   - Data wire to GPIO 32 (Grove connector white wire)
   - Ground to GND (Grove connector black wire)
   - Power to external 5V supply (LED strips require significant current)

## Version History

### v5.1.0 (2026-01-03) - **Audio Brightness & Speed Tuning**
- Expanded brightness range to 8-80 (was 18-50) for more dramatic contrast
- Reduced brightness scale minimum to 0.02 (was 0.1) for very dark troughs
- Added speed envelope with 0.3x-3.0x range for beat-reactive tempo changes
- Prevents "white blur" effect from excessive speed
- Improved AGC with higher MIN_DYNAMIC_RANGE (0.15) and expansion (1.0x)
- More visible beat response without overwhelming speed changes

### v5.0.0 (2026-01-03) - **Fluffy Mode (E1.31/sACN WiFi Receiver)**
- Added MODE_FLUFFY as 5th mode for E1.31/sACN DMX reception
- WiFi network: "GMA-WIFI_Access_Point" / "3576wifi"
- Universe 30, port 5568, multicast 239.255.0.30
- Displays first 100 LEDs from channels 1-300, clears remaining 100
- White screen indicator for Fluffy mode
- Complete ESP-NOW isolation (deinit on enter, reinit on exit)
- Button cycling: Green → Purple → White → Green
- Auto-reconnect WiFi every 30 seconds
- Compatible with QLab, ETC EOS, GrandMA, and other E1.31/sACN software

### v4.2.0 (2026-01-03) - **Audio Brightness Restoration & AGC**
- Brightness restoration: Auto-restore to idle brightness after 3s of no beats
- Fixed hardcoded 0.3 brightness override that prevented restoration
- Improved AGC for beat detection in noisy environments
- Lowered beat threshold to 0.35 in high-volume environments
- Purple mode now starts bright instead of dim
- Full dynamic range from dim to bright in all noise conditions

### v4.1.0 (2026-01-02)
- Re-sync timeout reduced to 1.5s (was 8s) for faster recovery
- Sine wave speed boost: 6x on beats for more visible response
- Brightness range improvements

### v3.3.2 (2024-10-26) - **ESP-NOW Packet Loss Fix**
- Added 500μs delay between packet sends to prevent buffer overflow
- Fixes "2 packets failed, 3 succeeded" broadcast failures
- Eliminates follower desync and stuck states

### v3.0.0 (2024-10-26) - **Complete Rewrite**
- Ultra-simple 4-mode system (before Fluffy mode added)
- Color-coded LCD backgrounds
- Direct LED data sync (153 bytes per packet, 50 LEDs)
- Zero processing delay on followers
- High contrast music mode

### v1.0.0 (2024-10-24)
- Initial release
- Basic FastLED patterns for M5StickC Plus 2

## Technical Details

### ESP-NOW Protocol
- **Communication**: Low-latency, connectionless wireless (MAC layer)
- **Broadcast Mode**: WiFi broadcast address (FF:FF:FF:FF:FF:FF)
- **Update Rate**: 50ms broadcast interval (20 Hz)
- **LED Data Transfer**: 200 LEDs split into chunks of 49 LEDs per packet
- **Message Structure**: Custom LEDSync structure with sequence numbers
- **Channel**: Fixed to WiFi channel 1

### E1.31/sACN Protocol (Fluffy Mode)
- **Standard**: ANSI E1.31 (Streaming ACN)
- **Transport**: UDP multicast (239.255.0.X where X = universe)
- **Port**: 5568 (standard E1.31 port)
- **Packet Format**: ArtNet-compatible (validated by ID, OpCode, universe, length)
- **Universe Calculation**: Multicast IP = 239.255.0.0 + universe number
- **Packet Validation**: Checks "Art-Net\0" header, OpCode 0x5000, universe match, min 300 channels

### Synchronization Features
- **Automatic Leader Discovery**: Followers detect and sync to leaders
- **Timeout Handling**: 8-second timeout with fallback to standalone
- **Brightness Sync**: Followers mirror leader's music-reactive brightness
- **Speed Sync**: Followers mirror leader's beat-reactive speed changes
- **Zero Processing Delay**: Followers directly apply received LED data

### Audio Processing (Music Modes)
- **Beat Detection**: Threshold-based with BPM validation (30-300 BPM)
- **Brightness Envelope**: Exponential attack (instant) / decay (τ=1.0s)
- **Speed Envelope**: Exponential attack (instant) / decay (τ=1.0s)
- **Moving Averages**: soundMin/soundMax with 99.5% smoothing
- **AGC Expansion**: 1.0x multiplier for aggressive dynamic range expansion
- **Noise Floor**: Moving average with 99.5% smoothing
- **Peak Tracking**: Moving average with 99.5% smoothing

### Watchdog Timer
- 30-second watchdog prevents system hangs
- Automatically resets if system becomes unresponsive

### Pattern Auto-Advance
- Patterns change every 15 seconds (configurable)
- Toggle via Button B long press
- Works in all modes except Fluffy (Fluffy displays DMX data only)

## Troubleshooting

### Fluffy Mode (E1.31/sACN) Issues

**Not Connecting to WiFi**
- Verify SSID "GMA-WIFI_Access_Point" and password "3576wifi" are correct
- Check that WiFi network is active and in range
- Display shows "Fluffy (No WiFi)" when disconnected
- Auto-reconnect attempts every 30 seconds

**No DMX Data Displayed**
- Verify lighting software is sending to Universe 30
- Check that E1.31/sACN output is enabled in software
- Ensure multicast mode is enabled (or send unicast to device IP)
- Verify channels 1-300 are being sent
- Check Serial output for packet reception debug info

**LEDs Show Wrong Colors**
- Verify channel mapping: Channel 1=LED0 Red, 2=Green, 3=Blue
- Some software uses different channel ordering (RGB vs GRB)
- Adjust COLOR_ORDER in code if needed

**Can't Exit Fluffy Mode**
- Short press Button A to cycle back to Green (Normal) mode
- ESP-NOW will reinitialize automatically
- Device may need power cycle if stuck

### ESP-NOW Sync Issues

**Followers Not Detecting Leader**
- Ensure all devices are on WiFi channel 1
- Check that devices are within range (~10-50 meters)
- Verify ESP-NOW initialized properly (check Serial output)
- Try power cycling both leader and follower

**Followers Timing Out**
- Check Serial output on leader for "BROADCAST FAILED" messages
- Reduce distance between devices
- Check for WiFi interference on channel 1
- Ensure all devices running v3.3.2 or later (packet loss fix)

### Music Mode Issues

**Not Responding to Audio**
- Ensure audio source is loud enough
- Microphone is on bottom of M5StickC Plus 2
- Check Serial output for audio level percentages
- Display shows "Beat: YES/NO" status
- Try adjusting volume or moving device closer

**Too Sensitive / Not Sensitive Enough**
- AGC automatically adjusts, but may take a few seconds
- Very loud environments: AGC expands dynamic range
- Very quiet: Increase source volume
- Check BRIGHTNESS_THRESHOLD (default 0.35)

**Brightness Stays Dim**
- Fixed in v4.2.0 with brightness restoration
- Should auto-restore to idle brightness after 3 seconds of silence
- Check that NO_BEAT_TIMEOUT is set to 3000 (3 seconds)

**Patterns Moving Too Fast**
- Fixed in v5.1.0 with reduced speed range
- SPEED_BASE 0.3x and SPEED_BOOST_MULTIPLIER 3.0x prevents "white blur"
- Adjust these constants if different range desired

### General Issues

**LEDs Flickering or Partial Updates**
- Check power supply (LED strips need substantial current)
- Ensure good ground connection between M5Stick and LED strip
- Verify data line connection to GPIO 32
- Reduce NUM_LEDS if power insufficient

**Device Crashes or Resets**
- Watchdog timer triggers if patterns take too long
- Check Serial output for crash dumps
- Try reducing NUM_LEDS or pattern complexity
- Ensure adequate power supply

## Credits

Original FastLED patterns by Mark Kriegsman.
ESP-NOW Sync System, Music Reactivity, and E1.31/sACN Integration by John Cohn, 2024-2026.

## License

This project is open source. Feel free to modify and share!
