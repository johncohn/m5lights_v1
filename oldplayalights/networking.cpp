#include "networking.h"
#include "ota.h"
#include "audio.h"
#include "patterns.h"

// WiFi networks to try in order
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

WiFiNetwork wifiNetworks[] = {
  {"Barn", "3576wifi"},
  {"johncohn6s", "j0hnc0hn!"}
};
const int numWiFiNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// WiFi management variables
bool wifiConnected = false;
bool wifiPreviouslyConnected = false;
uint32_t lastWiFiCheck = 0;
uint32_t lastWiFiStatusCheck = 0;
const uint32_t WIFI_CHECK_INTERVAL = 120000; // Check every 2 minutes when connected
const uint32_t WIFI_CHECK_INTERVAL_DISCONNECTED = 30000; // Check every 30 seconds when disconnected
const uint32_t WIFI_STATUS_CHECK_INTERVAL = 10000; // Quick status check every 10 seconds
bool wifiCheckInProgress = false;
bool wifiJustDisconnected = false;

void initNetworking(){
  if(DEBUG_SERIAL) Serial.println("Initializing ESP-NOW (priority)...");
  
  // ESP-NOW initialization - no OTA mode checks needed in v56 style
  
  // CRITICAL: Initialize ESP-NOW FIRST with no delays
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // We'll handle reconnection manually
  
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer={};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = 0; 
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  uint8_t mac_raw[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac_raw);
  myToken = ((uint32_t)mac_raw[3]<<16) |
            ((uint32_t)mac_raw[4]<< 8) |
             (uint32_t)mac_raw[5];
  
  if(DEBUG_SERIAL) {
    Serial.printf("ESP-NOW ready! MAC %02X:%02X:%02X:%02X:%02X:%02X → token=0x%06X\n",
      mac_raw[0],mac_raw[1],mac_raw[2],mac_raw[3],mac_raw[4],mac_raw[5], myToken
    );
  }
  
  // Show ESP-NOW ready status immediately
  canvas.fillSprite(TFT_BLACK);
  canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_BLUE);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_WHITE);
  canvas.setCursor(10, 10);
  canvas.print("ESP-NOW READY");
  canvas.setTextSize(1);
  canvas.setCursor(10, 50);
  canvas.print("Local mesh active");
  canvas.setCursor(10, 65);
  canvas.print("WiFi check starting...");
  canvas.pushSprite(0, 0);
  
  // Initialize WiFi state tracking
  wifiConnected = false;
  wifiPreviouslyConnected = false;
  wifiJustDisconnected = false;
  
  // Schedule first WiFi check for 3 seconds from now (non-blocking)
  lastWiFiCheck = millis() - WIFI_CHECK_INTERVAL + 3000;
  lastWiFiStatusCheck = millis();
  
  if(DEBUG_SERIAL) Serial.println("ESP-NOW initialization complete - no blocking!");
}

void checkWiFiStatusQuickly() {
  uint32_t now = millis();
  
  // Quick WiFi status check every 10 seconds (non-blocking)
  if (now - lastWiFiStatusCheck < WIFI_STATUS_CHECK_INTERVAL) return;
  lastWiFiStatusCheck = now;
  
  // Quick check - no scanning, just see if we're still connected
  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
  
  // Detect WiFi state changes
  if (currentlyConnected != wifiPreviouslyConnected) {
    if (currentlyConnected) {
      if(DEBUG_SERIAL) Serial.printf("WiFi reconnected: %s\n", WiFi.localIP().toString().c_str());
      wifiConnected = true;
      wifiJustDisconnected = false;
    } else {
      if(DEBUG_SERIAL) Serial.println("WiFi disconnected - switching to LOCAL mode");
      wifiConnected = false;
      wifiJustDisconnected = true;
      
      // CRITICAL: When WiFi disconnects, ensure ESP-NOW is still working
      // Sometimes WiFi disconnect can interfere with ESP-NOW
      if (esp_now_is_peer_exist(broadcastAddress) == false) {
        if(DEBUG_SERIAL) Serial.println("ESP-NOW peer lost - re-adding broadcast peer");
        esp_now_peer_info_t peer={};
        memcpy(peer.peer_addr, broadcastAddress, 6);
        peer.channel = 0; 
        peer.encrypt = false;
        esp_now_add_peer(&peer);
      }
    }
    
    // Handle WiFi transition with sync reset
    handleWiFiTransition(wifiPreviouslyConnected, currentlyConnected);
    wifiPreviouslyConnected = currentlyConnected;
  }
  
  // Clear the "just disconnected" flag after a few status checks
  if (wifiJustDisconnected && now - lastWiFiStatusCheck > 30000) {
    wifiJustDisconnected = false;
  }
}

void checkWiFiPeriodically() {
  uint32_t now = millis();
  
  // Use different intervals based on connection status
  uint32_t checkInterval = wifiConnected ? WIFI_CHECK_INTERVAL : WIFI_CHECK_INTERVAL_DISCONNECTED;
  if (now - lastWiFiCheck < checkInterval) return;
  if (wifiCheckInProgress) return; // Prevent overlapping checks
  
  // Don't do WiFi scanning if we just disconnected - let things settle
  if (wifiJustDisconnected) {
    if(DEBUG_SERIAL) Serial.println("Skipping WiFi scan - recently disconnected, letting ESP-NOW stabilize");
    lastWiFiCheck = now; // Reset timer
    return;
  }
  
  lastWiFiCheck = now;
  wifiCheckInProgress = true;
  
  if(DEBUG_SERIAL) Serial.println("Background WiFi scan starting...");
  
  // Quick check if already connected
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      wifiPreviouslyConnected = true;
      if(DEBUG_SERIAL) Serial.printf("WiFi already connected: %s\n", WiFi.localIP().toString().c_str());
    }
    wifiCheckInProgress = false;
    return;
  }
  
  // WiFi not connected - try to find and connect
  wifiConnected = false;
  wifiPreviouslyConnected = false;
  
  // CRITICAL: Temporarily reduce ESP-NOW activity during WiFi scan
  // Save current FSM state
  FsmState savedState = fsmState;
  
  // Quick scan with very short timeout
  WiFi.disconnect(); // Ensure clean state
  int networksFound = WiFi.scanNetworks(false, false, false, 300); // 300ms max
  
  if (networksFound > 0) {
    for(int i = 0; i < numWiFiNetworks; i++) {
      // Check if this network is available
      bool networkFound = false;
      for(int j = 0; j < networksFound; j++) {
        if(WiFi.SSID(j) == String(wifiNetworks[i].ssid)) {
          networkFound = true;
          break;
        }
      }
      
      if(!networkFound) continue;
      
      if(DEBUG_SERIAL) Serial.printf("Trying WiFi: %s (background)\n", wifiNetworks[i].ssid);
      
      WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
      
      // Very short connection attempt - only 3 seconds max
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 6) {
        delay(500);
        attempts++;
        
        // Feed watchdog during WiFi connection
        if (attempts % 2 == 0) {
          // Let ESP-NOW continue working during connection attempt
          esp_now_send(broadcastAddress, (uint8_t*)"", 0); // Keepalive
        }
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiPreviouslyConnected = true;
        if(DEBUG_SERIAL) {
          Serial.printf("WiFi connected (background): %s, IP: %s\n", 
            wifiNetworks[i].ssid, WiFi.localIP().toString().c_str());
        }
        break;
      } else {
        WiFi.disconnect();
      }
    }
  }
  
  if (!wifiConnected && DEBUG_SERIAL) {
    Serial.println("No WiFi available - continuing in LOCAL mode");
  }
  
  // Restore ESP-NOW state after WiFi operations
  if (savedState != fsmState) {
    if(DEBUG_SERIAL) Serial.printf("FSM state changed during WiFi scan: %d -> %d\n", savedState, fsmState);
  }
  
  wifiCheckInProgress = false;
}

void forceSyncReset() {
  if(DEBUG_SERIAL) Serial.println("[SYNC] Forcing synchronization reset...");
  
  // Clear all FSM state
  fsmState = FOLLOWER;
  lastRecvMillis = 0;
  lastHeartbeat = 0;
  electionEnd = 0;
  highestTokenSeen = 0;
  
  // Clear LED state to force fresh pattern
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  // Reset networking state
  esp_now_deinit();
  delay(200); // Brief pause for cleanup
  
  // Reinitialize ESP-NOW
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer={};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = 0; 
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  
  // Force new election after brief delay
  uint32_t now = millis();
  electionEnd = now + ELECTION_TIMEOUT + 500; // Extra delay for stability
  myDelay = random(100, 300); // Randomize to avoid conflicts
  
  if(DEBUG_SERIAL) {
    Serial.println("[SYNC] Reset complete - forcing new election");
    Serial.printf("[SYNC] Token: 0x%06X, Election delay: %ums\n", myToken, myDelay);
  }
}

void handleWiFiTransition(bool wasConnected, bool nowConnected) {
  if (wasConnected == nowConnected) return; // No change
  
  if(DEBUG_SERIAL) {
    Serial.printf("[WIFI] Transition: %s -> %s\n", 
      wasConnected ? "CONNECTED" : "DISCONNECTED",
      nowConnected ? "CONNECTED" : "DISCONNECTED");
  }
  
  // Brief pause to let network settle
  delay(100);
  
  // Force sync reset on any WiFi transition
  // This prevents nodes from getting stuck in stale states
  forceSyncReset();
  
  if(DEBUG_SERIAL) Serial.println("[WIFI] Sync reset complete - nodes should resynchronize");
}

void handleNetworking(){
  uint32_t now = millis();
  
  if(currentMode != AUTO) return;
  
  
  // No OTA mode checks needed - v56 style handles OTA gracefully
  
  // Check WiFi status quickly and frequently (non-blocking)
  checkWiFiStatusQuickly();
  
  // Check WiFi connectivity periodically in background (blocking but infrequent)
  if (!wifiJustDisconnected) { // Skip if we just disconnected
    checkWiFiPeriodically();
  }
  
  switch(fsmState){
    case FOLLOWER: {
      if(chunkMask == ((1u << ((NUM_LEDS + 74) / 75)) - 1u)){
        // Skip LED updates if ESP-NOW suspended for OTA
        if(!otaSuspended) {
          // Followers apply only their LOCAL brightness (no audio detection)
          // The LED data already contains the leader's music reactivity at FULL brightness
          FastLED.setBrightness(globalBrightnessScale);
          FastLED.show(); 
        }
        chunkMask = 0;
      }
      
      uint32_t timeSinceLastMsg = now - lastRecvMillis;
      if(timeSinceLastMsg > LEADER_TIMEOUT){
        missedFrameCount++;
        if(missedFrameCount >= 3) {
          // IMPORTANT: Reset LED state when becoming disconnected
          fill_solid(leds, NUM_LEDS, CRGB::Black);
          FastLED.show();
          
          fsmState = ELECT;
          electionStart = now; 
          electionEnd = now + ELECTION_TIMEOUT;
          highestTokenSeen = myToken;
          myDelay = ((0xFFFFFFFF - myToken) * ELECTION_BASE_DELAY) / 0xFFFFFFFF
                    + random(0, ELECTION_JITTER);
          electionBroadcasted = false;
          missedFrameCount = 0;
          chunkMask = 0;  // Reset chunk mask
          if(DEBUG_SERIAL) {
            Serial.printf("FSM: FOLLOWER→ELECT (timeout=%ums) token=0x%06X delay=%ums\n",
              timeSinceLastMsg, myToken, myDelay
            );
          }
        }
      } else {
        if(timeSinceLastMsg < LEADER_TIMEOUT / 2) {
          missedFrameCount = 0;
        }
      }
      break;
    }
    
    case ELECT: {
      if(!electionBroadcasted && now >= electionStart + myDelay){
        sendToken(); 
        electionBroadcasted = true;
        if(DEBUG_SERIAL) Serial.println("FSM: ELECT broadcast token");
      }
      if(now >= electionEnd){
        if(highestTokenSeen > myToken){
          fsmState = FOLLOWER; 
          lastRecvMillis = now;
          chunkMask = 0;  // CRITICAL: Reset chunk tracking
          missedFrameCount = 0;
          if(DEBUG_SERIAL) {
            Serial.printf("FSM: ELECT lost→FOLLOWER (high=0x%06X)\n", highestTokenSeen);
          }
        } else {
          fsmState = LEADER;
          if(DEBUG_SERIAL) {
            Serial.printf("FSM: ELECT won→LEADER (high=0x%06X)\n", highestTokenSeen);
          }
        }
      }
      break;
    }
    
    case LEADER: {
      if(now - lastHeartbeat >= LEADER_HEARTBEAT_INTERVAL){
        sendToken(); 
        lastHeartbeat = now;
      }
      
      if(highestTokenSeen > myToken){
        // CRITICAL: Properly reset state when stepping down
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        
        fsmState = FOLLOWER; 
        lastRecvMillis = now; 
        chunkMask = 0;  // Reset chunk mask completely
        missedFrameCount = 0;
        if(DEBUG_SERIAL) {
          Serial.printf("FSM: LEADER saw higher token→FOLLOWER (0x%06X) - resetting LED state\n", highestTokenSeen);
        }
        break;
      }
      
      // Use simple, fast pattern execution to eliminate latency
      // Crossfade disabled for performance - was causing 0.5s delays
      if(freezeActive) {
        detectAudioFrame();
        if(audioDetected) effectMusic();
        else             effectWildBG();
      } else {
        detectAudioFrame();
        if(audioDetected) runTimed(effectMusic);
        else             runTimed(effectWildBG);
      }
      
      // Send the LED data with music reactivity baked into the colors at FULL brightness
      sendRaw();
      
      // Skip LED updates if ESP-NOW suspended for OTA
      if(!otaSuspended) {
        // Apply leader's local brightness for its own display
        FastLED.setBrightness(globalBrightnessScale);
        FastLED.show();
      }
      
      if(DEBUG_SERIAL && millis() % 10000 < 50) { // Less frequent debug
        Serial.printf("LEADER: music=%.2f, audioDetected=%s, localBright=%d, wifi=%s\n", 
          musicLevel, audioDetected ? "true" : "false", globalBrightnessScale, 
          wifiConnected ? "connected" : "local");
      }
      break;
    }
  }
}

void onRecv(const esp_now_recv_info_t*, const uint8_t* data, int len){
  uint32_t now = millis();
  
  if(len >= 5 && data[0] == MSGTYPE_TOKEN) {
    uint32_t incomingToken; 
    memcpy(&incomingToken, data+1, 4);
    
    if(incomingToken > highestTokenSeen) {
      highestTokenSeen = incomingToken;
    }
    
    if(fsmState == FOLLOWER && currentMode == AUTO) {
      lastRecvMillis = now;
      missedFrameCount = 0;
      if(DEBUG_HEARTBEAT) {
        Serial.printf("Heartbeat: token=0x%06X (leader alive)\n", incomingToken);
      }
    }
    return;
  }
  
  
  if(len < 10 || data[0] != MSGTYPE_RAW) return;
  
  uint32_t incomingToken; 
  memcpy(&incomingToken, data+5, 4);
  
  // Minimal debug output to avoid blocking (only if heartbeat debug enabled)
  if(DEBUG_HEARTBEAT && millis() % 5000 < 50) { // Much less frequent
    Serial.printf("onRecv: fsm=%s token=0x%06X\n",
      (fsmState==LEADER?"LEADER":fsmState==FOLLOWER?"FOLLOWER":"ELECT"), incomingToken
    );
  }
  
  if(fsmState == LEADER && incomingToken > myToken){
    if(DEBUG_SERIAL) Serial.printf("Conflict: stepping DOWN (saw higher token)\n");
    fsmState = FOLLOWER; 
    lastRecvMillis = now; 
    chunkMask = 0;
    missedFrameCount = 0;
    return;
  }
  
  if(fsmState == FOLLOWER && currentMode == AUTO){
    uint8_t idx = data[9];
    int base = idx * 75, cnt = min(75, NUM_LEDS - base);
    memcpy(leds + base, data + 10, cnt * 3);
    chunkMask |= (1 << idx);
    lastRecvMillis = now;
    missedFrameCount = 0;
  }
}

void sendRaw(){
  int chunks = (NUM_LEDS + 74) / 75;
  uint8_t buf[1+4+4+1+75*3];
  
  for(int c = 0; c < chunks; c++){
    int base = c * 75, cnt = min(75, NUM_LEDS - base);
    buf[0] = MSGTYPE_RAW;
    memcpy(buf+1, &masterSeq, 4);
    memcpy(buf+5, &myToken, 4);
    buf[9] = c;
    
    // Send FULL BRIGHTNESS LED data - each node applies its own brightness locally
    for(int i = 0; i < cnt; i++){
      CRGB &led = leds[base + i];
      buf[10 + i*3    ] = led.r;  // No brightness scaling here
      buf[10 + i*3 + 1] = led.g;
      buf[10 + i*3 + 2] = led.b;
    }
    
    esp_now_send(broadcastAddress, buf, 10 + cnt*3);
    masterSeq++;
  }
}

void sendToken(){
  uint8_t buf[5] = {MSGTYPE_TOKEN, 0, 0, 0, 0};
  memcpy(buf+1, &myToken, 4);
  esp_now_send(broadcastAddress, buf, sizeof(buf));
}

// ── OTA Coordination Functions ───────────────────────────────────────────────



// handleOTASuspendTimeout() function removed - inlined for performance