# NeoPixel Controller System

A sophisticated distributed LED art installation system using M5StickC Plus2 microcontrollers to create synchronized light shows across multiple nodes. Each node controls 334 WS2812B NeoPixel LEDs with advanced networking, audio reactivity, and individual user control.

## Current Status: Version 1.1.45 (Latest Development)

**Last Updated**: August 2025  
**Active Nodes**: ~11 devices (USB deployment - OTA discontinued)  
**Network**: Works with or without WiFi (ESP-NOW mesh priority)  
**Recent Focus**: **Performance optimization**, **leader-follower synchronization**, **high-volume audio responsiveness**

## 🆕 Recent Major Improvements (v1.1.45)

### ✅ Advanced Synchronization System
- **5-second crossfade transitions** - extended from 3 seconds for even smoother pattern changes
- **Leader delay compensation** - 0.5 second buffer system to perfectly sync leader with followers
- **Dual buffer architecture** - leader delays its own display while immediately broadcasting to followers
- **Eliminates timing gaps** - no more visible lag between leader and follower LED changes

### ✅ High-Volume Audio Responsiveness
- **Adaptive sensitivity scaling** - maintains beat detection even at very high volume levels (live shows)
- **Dynamic range expansion** - artificially expands compressed audio range when moving averages converge
- **Saturation detection** - automatically detects when microphone reaches limits (soundMin > 0.7 or range < 0.08)
- **Threshold adaptation** - lowers beat detection from 0.6 to 0.35 in high-volume environments
- **Preserves normal operation** - only activates when needed, maintains current performance at moderate levels

### ✅ Enhanced OTA Deployment System
- **Robust retry logic** - up to 3 attempts per node automatically
- **Detailed logging** - comprehensive OTA.txt summary file with attempt counts
- **Selective retry** - option to retry only failed nodes from previous deployment
- **Extended timeouts** - 60-second timeout for reliable long uploads
- **Stricter success detection** - prevents false positive success reports
- **Beautiful summary tables** - clear deployment status for all nodes

### ✅ Pattern System Expansion  
- **42 total patterns** - doubled from original 22 patterns with unique names
- **Pattern cycling fix** - B button now cycles through all 42 patterns correctly
- **Complete pattern library** - comprehensive collection from basic to advanced effects

### ⚠️ OTA Deployment Status
- **Enhanced OTA script** with comprehensive retry and logging capabilities
- **USB deployment preferred** - switching to USB-only deployment for reliability
- **Individual node targeting** - deploy to specific nodes via temporary node files

### ✅ Latency & Performance Fixes
- **0.5 second latency eliminated** - improved ESP-NOW synchronization timing
- **Boot quiet mode** - resolved OTA interference deadlock issues
- **Improved mesh stability** - enhanced leader election and failover

## System Architecture

### Hardware Configuration
- **Controllers**: M5StickC Plus2 ESP32 devices
- **LEDs**: 334 WS2812B NeoPixels per node (`NUM_LEDS = 334`)
- **Communication**: ESP-NOW for pattern synchronization + WiFi for OTA updates
- **Audio**: Built-in microphone for beat detection and music reactivity
- **Power**: Each node independently powered
- **Network**: Multi-WiFi support (Barn, GMA-WIFI_Access_Point, phone hotspot)

### Distributed Leadership Model
- **AUTO Mode**: Nodes automatically elect a leader using MAC-based token algorithm
- **Leader Node**: Generates patterns, detects audio, broadcasts synchronized data at full brightness
- **Follower Nodes**: Receive and display leader's patterns with perfect synchronization
- **Automatic Failover**: If leader goes offline, remaining nodes elect new leader
- **Conflict Resolution**: Higher MAC-based tokens automatically become leader
- **Offline Operation**: ESP-NOW mesh works perfectly without WiFi

### Individual Brightness Control
- **Local Control**: Each node controls its own brightness independently
- **6 Brightness Levels**: 6%, 12%, 25% (default), 50%, 75%, 100%
- **Music Synchronization**: Leader's audio reactivity baked into LED colors, then each node applies local brightness
- **OFF Mode**: Complete sleep with dimmed display for battery savings

## User Interface

### Button Controls
- **Button A Short Press**: Cycle through brightness levels (LOCAL 6% → 12% → 25% → 50% → 75% → 100%)
- **Button A Long Press (2s)**: Enter/exit OFF mode (auto-syncs on wake)
- **Button B**: Pattern freeze/advance (only works when node is leader)
- **Button C**: Manual sync reset (force resynchronization with other nodes)

### LCD Display
- **Color-coded Status**: Orange (Leader), Green (Follower), Purple (Election), Black (OFF)
- **Brightness Display**: Shows current local brightness level (e.g., "LOCAL 50%")
- **Pattern Info**: Current pattern name or "Following Leader"
- **Connection Status**: ".236" (WiFi IP) or "LOCAL" (offline mode) in bottom right
- **Version Display**: "v1.1.x" in bottom left corner
- **Freeze Indicator**: [F] shown when patterns are frozen

## Pattern System

### 42 LED Patterns
**Basic**: Rainbow, Chase, Juggle, Rainbow+Glitter, Confetti, BPM, Fire, Color Wheel, Random  
**Advanced**: Pulse Wave, Meteor Shower, Color Spiral, Plasma Field, Sparkle Storm, Aurora Waves  
**Organic**: Organic Flow, Wave Collapse, Color Drift, Liquid Rainbow, Sine Breath, Fractal Noise, Rainbow Strobe  
**Extended Collection**: 20 additional unique patterns with descriptive names for complete visual variety

### Pattern Generation
- **Leader Only**: Generates patterns with audio reactivity baked into LED colors
- **Music Mode**: When audio detected, `effectMusic()` scales LED colors by `musicLevel` (dramatic nearly-off to full-bright response)
- **Background Mode**: Normal patterns when no audio detected
- **Extended Timing**: Automatic pattern cycling every 15 seconds (3x longer than original 5 seconds)
- **✅ NEW: Crossfade System**: 5-second smooth transitions between patterns with both patterns running simultaneously
- **Full Brightness Broadcast**: Leader sends 100% brightness data, each node applies local scaling

## Audio Reactivity

### Music Detection (Leader Only)
- **BPM Analysis**: 5-second rolling window for beat detection
- **Threshold Detection**: `musicLevel > 0.6f` triggers beat events
- **Adaptive Levels**: Moving averages for sound min/max provide full dynamic range
- **Synchronized Response**: All nodes react identically to leader's audio analysis

### Implementation
- **Color Scaling**: Music reactivity applied via `leds[i].nscale8(musicScale)` in `effectMusic()`
- **Network Distribution**: Music-scaled colors transmitted at full brightness
- **Local Brightness**: Each node applies its brightness percentage to received data
- **Dramatic Response**: Audio scaling ranges from ~12% (quiet) to 100% (loud beats)

## Networking Protocol

### ESP-NOW Communication (Primary)
- **Message Types**: RAW data (0x00), Token broadcasts (0x01)
- **Chunked Transmission**: LED data split into 75-LED chunks for reliability
- **Token System**: MAC-based tokens for leader election and heartbeats
- **Robust Failover**: 3-strike timeout system with automatic re-election
- **Offline Priority**: Works perfectly without WiFi, mesh-first design

### WiFi Management (Secondary - OTA Only)
- **Multi-Network Support**: Tries multiple WiFi networks automatically
- **Non-Blocking**: WiFi operations never interfere with ESP-NOW mesh
- **Background Scanning**: WiFi checked every 2 minutes, status every 10 seconds
- **Clean Transitions**: Handles WiFi connect/disconnect gracefully
- **OTA Ready**: When WiFi available, enables over-the-air updates

### Network Resilience
- **Heartbeat Monitoring**: 1.5-second timeout detection
- **State Recovery**: Automatic LED blanking and state reset on leadership changes
- **Chunk Validation**: Complete frame assembly before display
- **Health Monitoring**: System checks prevent stuck states

## Development & Deployment

### Modular Codebase
- **config.h**: Hardware/network configuration, debug controls
- **main.ino**: Setup and main loop coordination with version display  
- **patterns.cpp/.h**: All 42 LED pattern implementations with proper brightness handling and crossfading
- **networking.cpp/.h**: ESP-NOW communication, leader election, WiFi management with improved timing
- **audio.cpp/.h**: Microphone processing and BPM detection
- **ui.cpp/.h**: LCD display and button handling with 42-pattern cycling fix
- **ota.cpp/.h**: Over-the-air update functionality with ESP-NOW conflict resolution
- **version.h**: Auto-generated version information (currently v1.1.45)

### USB Deployment System (Primary)
- **USB-First Approach**: Reliable node-by-node deployment via USB cable
- **Auto USB Detection**: Automatically detects USB-connected nodes for upload
- **Smart Caching**: Only recompiles when source files change
- **Version Management**: Auto-incrementing version numbers with each compile
- **Individual Targeting**: Deploy to specific nodes one at a time
- **Enhanced OTA System**: Comprehensive retry logic and detailed logging (backup option)
- **OTA.txt Summary**: Clean deployment status tracking for retry automation

### Deployment Options
1. **Deploy** (compile if needed + upload with retries) [DEFAULT]
2. **Force recompile and deploy** (clean cache + full compile + upload)
3. **Upload only** (skip compile, use existing build)
4. **Compile only** (increment version + fast build, no upload)
5. **Clean cache** (remove all cached builds)

## Debug Controls
```cpp
#define DEBUG_SERIAL    1     // Essential messages only
#define DEBUG_HEARTBEAT 0     // Disable heartbeat spam
#define DEBUG_BPM       0     // Disable BPM detection spam
```

## Current Node Configuration
- **Active Nodes**: 10 devices on network 192.168.0.x
- **Current IPs**: 113, 122, 140, 141, 142, 148, 150, 181, 236, 238
- **OTA Password**: "neopixel123"  
- **Default Brightness**: 25% (64/255) per node
- **WiFi Networks**: Barn, johncohn6s (GMA-WIFI removed)
- **Firmware Status**: All nodes ready for v1.1.45 update with performance improvements via USB deployment

## Key Features Working ✅
- ✅ Distributed leader election with automatic failover
- ✅ **42 diverse LED patterns** with smooth 5-second crossfading transitions  
- ✅ **Pattern cycling fix** - B button now cycles through all 42 patterns correctly
- ✅ Synchronized music reactivity across all nodes (dramatic response restored)
- ✅ Individual brightness control (6%-100% + OFF)
- ✅ **Leader-follower synchronization** - 0.5 second delay compensation system for perfect timing
- ✅ **High-volume audio responsiveness** - adaptive scaling maintains beat detection at live show volumes
- ✅ Robust ESP-NOW networking with chunk validation and improved stability
- ✅ OTA deployment system with ESP-NOW conflict resolution (fixed August 2025)
- ✅ Audio-responsive brightness scaling with full dynamic range
- ✅ Comprehensive LCD UI with status indicators and version display
- ✅ Watchdog timer and error recovery systems
- ✅ Multi-WiFi support with clean offline operation
- ✅ Fast incremental compilation with smart caching
- ✅ USB batch update automation for emergency deployments

## Recent Major Fixes & Updates (August 2025)
- **OTA System Restoration**: Fixed ESP-NOW interference causing OTA upload failures
- **Pattern Duration Extension**: Patterns now run 15 seconds instead of 5 seconds (3x longer)
- **USB Deployment Automation**: Created `usb_simple.sh` for efficient batch USB updates
- **Board Configuration Fix**: Corrected FQBN from m5stack_core to m5stack_stickc_plus2
- **All Nodes Updated**: Successfully deployed v1.1.7 to all 10 nodes via USB
- **Network List Updates**: Removed GMA-WIFI, updated active node IP addresses
- **Deployment Script Improvements**: Enhanced error handling and connectivity testing

## Usage Workflow

### Development
```bash
# Fast development iteration - compile only
arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus2
# → Fast incremental build, version bump auto-incremented

# USB deployment (primary method)
arduino-cli upload --fqbn esp32:esp32:m5stack_stickc_plus2 --port <USB_PORT>
# → Direct USB upload to connected node

# Enhanced OTA deployment (backup method)
./deploy_nodes.sh    # Choose option 3: Retry failed nodes from OTA.txt
# → Uses OTA.txt to retry only previously failed nodes
```

### Operation
1. **Power On**: Nodes connect to WiFi (if available) and enter AUTO mode at 25% brightness
2. **Auto-Sync**: Nodes elect leader and begin synchronized pattern display
3. **User Control**: Button A cycles brightness, Button B controls patterns (leader only) 
4. **Music Mode**: Leader's microphone drives synchronized audio reactivity across all nodes
5. **Offline Ready**: Full functionality without WiFi, mesh network self-manages

## System Requirements
- **Arduino IDE** with ESP32 support
- **M5StickC Plus2** board definitions
- **FastLED library** (3.10.1+)
- **M5Unified library** (0.2.7+)
- **ESP32 Arduino Core** (3.2.1+)

## Network Architecture
The system prioritizes ESP-NOW mesh networking for real-time LED synchronization, with WiFi as an optional secondary network for OTA updates and remote management. This ensures perfect performance whether in your shop with WiFi or out in the woods at Burning Man.

**Local mesh network handles**: Pattern synchronization, leader election, audio reactivity  
**WiFi network handles**: OTA updates, remote configuration, version management

This architecture provides the reliability of a local mesh with the convenience of WiFi connectivity when available.