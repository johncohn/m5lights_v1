# M5 Lights v3.2.0

Ultra-Simple ESP-NOW LED Sync system for M5StickC Plus 2 with WS2811/WS2812 LED strips. Features wireless multi-device synchronization, music reactivity, and 11 stunning LED patterns.

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
- Automatically switches to **Follow mode (BLUE)** when a Leader is detected

#### 🟣 **Music Mode (PURPLE background)**
- LED patterns react to music/audio in real-time
- Uses built-in M5StickC Plus 2 microphone
- Adaptive audio sensitivity with beat detection
- Dynamic brightness (1-96) based on audio level
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)
- Automatically switches to **Follow mode (BLUE)** when a Leader is detected

#### 🟠 **Normal Leader Mode (ORANGE background)**
- Runs LED patterns locally
- **Broadcasts LED states** to all nearby follower devices via ESP-NOW
- Updates followers every 50ms
- All follower devices mirror the leader's exact LED display
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)

#### 🔴 **Music Leader Mode (RED background)**
- LED patterns react to music/audio in real-time
- **Broadcasts LED states AND brightness** to all nearby follower devices
- Followers mirror both the LED patterns and the music-reactive brightness
- All followers synchronize to the leader's audio
- Auto-cycles through patterns every 15 seconds (when auto-advance enabled)

#### 🔵 **Follow Mode (BLUE background)** - Automatic
- Triggered automatically when Leader broadcasts are detected
- Receives and displays LED states from Leader device in real-time
- Shows exact replica of Leader's patterns
- Mirrors Leader's brightness (including music reactivity)
- 8-second timeout - falls back to previous mode if Leader disconnects
- Display shows "Following..." status
- Pattern control disabled while following

### LED Animation Patterns

11 different LED animation patterns:

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

### Controls

#### Button A (Main Button)
- **Short Press**: Toggle between Normal ↔ Music (preserves leader status)
  - Normal → Music
  - Music → Normal
  - Normal Leader → Music Leader
  - Music Leader → Normal Leader

- **Long Press (1.5 seconds)**: Toggle leader status
  - Normal → Normal Leader
  - Music → Music Leader
  - Normal Leader → Normal
  - Music Leader → Music

#### Button B (Side Button)
- **Short Press**: Manually advance to next pattern (disabled in Follow mode)
- **Long Press (1 second)**: Toggle auto-advance patterns ON/OFF
- **Super Long Press (3 seconds)**: Toggle Debug Mode ON/OFF
  - When toggled, LEDs flash GREEN (enabled) or RED (disabled)
  - Debug mode shows detailed packet reception and sync information via Serial

#### Button C (Power Button)
- Standard M5StickC Plus 2 power functions
- Power cycle can help reset stuck synchronization state

### Display

The M5StickC Plus 2 LCD shows:
- **Background color** indicating current mode (GREEN/PURPLE/ORANGE/RED/BLUE)
- Application title "Simple Sync" and version number
- Current operating mode name
- Current pattern number and name
- In Music modes: Audio level percentage and beat detection status
- In Follow mode: "Following..." status

### Music Reactive System

The music reactive modes use the M5StickC Plus 2's built-in microphone with advanced features:

- **Adaptive Audio Sensitivity**: Automatically adjusts to different volume environments
- **Beat Detection**: Real-time beat detection with BPM calculation
- **Dynamic Range**: 1-96 brightness range for high contrast response
- **Noise Floor Tracking**: Automatically adapts to ambient noise levels
- **Peak Level Tracking**: Captures and adapts to loud sound peaks
- **Audio Detection**: Validates that actual audio is present (not just noise)

## Multi-Device Synchronized Light Show Setup

### Basic Setup (2+ devices)

1. **Setup Leader Device**:
   - Upload the code to your first M5StickC Plus 2
   - Choose your mode: Normal or Music
   - Long press Button A (1.5 seconds) to become a Leader
   - Display will change to **ORANGE** (Normal Leader) or **RED** (Music Leader)

2. **Setup Follower Device(s)**:
   - Upload the code to additional M5StickC Plus 2 devices
   - Power them on (they start in Normal mode - GREEN background)
   - They will automatically detect the Leader and switch to **Follow mode (BLUE)**
   - Display will show "Following..." and background turns **BLUE**
   - All followers will mirror the Leader's LED patterns exactly

3. **Switching Between Modes**:
   - **Leader to Follower**: Short press Button A on the Leader twice (Leader → Normal → detects other Leader → Follow)
   - **Follower to Leader**: Followers will automatically become standalone after 8 seconds if the Leader disconnects
   - **Change Leader Type**: Short press Button A on the Leader to toggle Normal Leader ↔ Music Leader
   - **Stop Being Leader**: Long press Button A on the Leader to return to standalone mode

### Advanced: Music Synchronized Show

For a music-reactive synchronized light show across multiple devices:

1. Position the Leader device near the music source
2. Put the Leader in **Music Leader mode (RED)** - either:
   - Start in Music mode (PURPLE) and long press Button A, OR
   - Start in Normal Leader mode (ORANGE) and short press Button A
3. All followers (BLUE) will mirror both the patterns AND the music-reactive brightness
4. The entire system syncs to the Leader's microphone input

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
#define REJOIN_SCAN_INTERVAL_MS 15000     // Rejoin scan interval

// Audio (Music modes)
#define MIC_BUF_LEN 240                   // Microphone buffer length
#define MIC_SR 44100                      // Sample rate
#define SMOOTH 0.995f                     // Audio smoothing factor
#define BPM_WINDOW 5000                   // BPM calculation window (ms)
```

## Libraries Required

- **M5StickCPlus2** - M5Stack official library
- **FastLED** - FastLED library for LED control
- **M5Unified** (dependency) - M5Stack unified library
- **M5GFX** (dependency) - M5Stack graphics library
- **ESP-NOW** (built into ESP32) - ESP32 wireless protocol
- **WiFi** (built into ESP32) - ESP32 WiFi library

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

### v3.2.0 (2024-10-26) - **Synchronization Bug Fix Release**
- **Fixed intermittent follower desync bug**
  - Added frame ID tracking to prevent partial frame rendering
  - Implemented packet completion detection before calling FastLED.show()
  - Added fallback FastLED.show() in main loop (every 100ms) for followers
  - Fixed issue where followers would show BLUE screen but LEDs were out of sync
  - Fixed mode flickering between BLUE (Follow) and GREEN (Normal)
- **Enhanced debugging**
  - Added Debug Mode toggle via Button B super-long press (3 seconds)
  - Debug mode shows frame reception, packet tracking, and completion status
  - Visual confirmation with LED flashes (GREEN=enabled, RED=disabled)
  - Packet loss warnings when no complete frames received in 500ms
- **Improved broadcast reliability**
  - Added 500μs delay between packets to prevent receiver overflow
  - Send frame ID, total packets, and packet index with each transmission
  - Better packet sequencing and completion tracking

### v3.1.1 (2024-10-26)
- Removed problematic lava pattern causing device crashes
- Improved stability
- 11 patterns total

### v3.1.0 (2024-10-26)
- Added ultra-dynamic lava pattern with faster, more organic movement
- Enhanced color temperature variations
- Improved flickering effects

### v3.0.0 (2024-10-26)
- **Complete rewrite** with ultra-simple 4-mode system
- Normal, Music, Normal Leader, Music Leader modes
- Color-coded LCD backgrounds (GREEN/PURPLE/ORANGE/RED/BLUE)
- Button A: Short press = Normal ↔ Music, Long press = Become Leader
- Direct LED data sync (153 bytes per packet, 50 LEDs)
- Zero processing delay on followers
- All 12 patterns + working music system
- High contrast music mode (1-96 brightness range)
- Button B controls for manual pattern advancement and auto-advance toggle

### v2.x
- Previous implementations with different synchronization approaches

### v1.0.0 (2024-10-24)
- Initial release
- Basic FastLED patterns for M5StickC Plus 2

## Technical Details

### ESP-NOW Protocol
- **Communication**: Uses ESP-NOW for low-latency, connectionless wireless communication
- **Broadcast Mode**: Uses WiFi broadcast address (FF:FF:FF:FF:FF:FF) for one-to-many communication
- **Update Rate**: 50ms broadcast interval (20 Hz) for smooth synchronization
- **Message Structure**: Custom LEDSync structure with sequence numbers and brightness data
- **LED Data Transfer**: Splits 200 LEDs into chunks of 49 LEDs (147 bytes RGB data + overhead)
- **Channel**: Fixed to WiFi channel 1 for all devices

### Synchronization Features
- **Automatic Leader Discovery**: Followers automatically detect and follow leaders
- **Timeout Handling**: 8-second timeout with automatic fallback to standalone mode
- **Rejoin Logic**: Automatic reconnection attempts when leader connection is lost
- **Brightness Sync**: Followers mirror leader's brightness (crucial for music sync)
- **Zero Processing Delay**: Followers directly apply received LED data without re-computation

### Audio Processing (Music Modes)
- **Sample Rate**: 44.1 kHz for high-quality audio capture
- **Buffer Size**: 240 samples per frame
- **Beat Detection**: Threshold-based beat detection with BPM validation
- **Adaptive Sensitivity**: Automatically adjusts to quiet and loud environments
- **Dynamic Range**: Full contrast from 1 (nearly off) to 96 (full brightness)
- **Noise Floor Tracking**: Moving average with 99.8% smoothing
- **Peak Tracking**: Moving average with 99% smoothing
- **Audio Validation**: Ensures actual audio is present (minimum 4 beats in 5 seconds, 30-300 BPM range)

### Watchdog Timer
- 30-second watchdog timer prevents system hangs
- Automatically resets if system becomes unresponsive

### Pattern Auto-Advance
- Patterns automatically change every 15 seconds (configurable)
- Can be disabled via Button B long press
- Works in all modes (Normal, Music, Normal Leader, Music Leader)
- Disabled in Follow mode (followers mirror leader's pattern)

## Troubleshooting

### Followers Not Detecting Leader
- Ensure all devices are on the same WiFi channel (channel 1)
- Check that devices are within range (~10-50 meters depending on environment)
- Verify ESP-NOW is initialized properly (check Serial output)
- Try power cycling both leader and follower devices

### Followers Keep Timing Out or Desyncing
- **Enable Debug Mode** (Button B 3-second hold) to see packet reception details
- Leader may be out of range or experiencing interference
- Check Serial output for:
  - "Leader timeout" messages
  - "WARNING: No complete frames" - indicates packet loss
  - Frame completion messages - should show consistent frame IDs
- Reduce distance between devices
- Ensure leader is actively broadcasting (ORANGE or RED background)
- Try power cycling the follower (Button C) to reset sync state
- Check for WiFi interference from other devices on channel 1

### Music Mode Not Responding
- Ensure music/audio source is loud enough and near the M5Stick
- Check Serial output for audio level percentages
- Built-in microphone is on the bottom of the M5StickC Plus 2
- Try adjusting volume or moving device closer to audio source
- Check that audio detection is working (Beat: YES/NO on display)

### LEDs Flickering or Partial Updates
- Check power supply - LED strips require substantial current
- Ensure good ground connection between M5Stick and LED strip
- Verify data line connection to GPIO 32
- Reduce NUM_LEDS if power supply is insufficient

### Device Crashes or Resets
- Watchdog timer may be triggering due to long pattern execution
- Some patterns (like removed lava pattern) may cause crashes
- Check Serial output for crash dumps
- Try reducing NUM_LEDS or FRAMES_PER_SECOND

## Credits

Original FastLED patterns by Mark Kriegsman, December 2014.
Ultra-Simple ESP-NOW Sync System and Music Reactivity by John Cohn, 2024.

## License

This project is open source. Feel free to modify and share!
