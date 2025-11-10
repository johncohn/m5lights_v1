#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/networking.cpp"
#include "networking.h"
#include "audio.h"
#include "patterns.h"

void initNetworking(){
  // Connect to WiFi first
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Show connection progress on LCD
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_WHITE);
  canvas.setCursor(10, 10);
  canvas.print("WiFi Connecting...");
  canvas.pushSprite(0, 0);
  
  // Wait for connection with timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Update progress on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("WiFi Connecting");
    canvas.setTextSize(1);
    canvas.setCursor(10, 40);
    canvas.printf("Attempt %d/30", attempts);
    canvas.pushSprite(0, 0);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("SSID: %s, Signal: %d dBm\n", WiFi.SSID().c_str(), WiFi.RSSI());
    
    // Show success on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_GREEN);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("WiFi Connected!");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print("IP: " + WiFi.localIP().toString());
    canvas.setCursor(10, 70);
    canvas.printf("Signal: %d dBm", WiFi.RSSI());
    canvas.pushSprite(0, 0);
    delay(2000); // Show for 2 seconds
  } else {
    Serial.println();
    Serial.println("WiFi connection failed! Check credentials.");
    
    // Show error on LCD
    canvas.fillSprite(TFT_BLACK);
    canvas.fillRect(0, 0, M5.Lcd.width(), 40, TFT_RED);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(10, 10);
    canvas.print("WiFi Failed!");
    canvas.setTextSize(1);
    canvas.setCursor(10, 50);
    canvas.print("Check credentials");
    canvas.setCursor(10, 70);
    canvas.print("OTA disabled");
    canvas.pushSprite(0, 0);
    delay(3000); // Show error for 3 seconds
  }
  
  // Initialize ESP-NOW regardless of WiFi status
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
  Serial.printf("MAC %02X:%02X:%02X:%02X:%02X:%02X → token=0x%06X\n",
    mac_raw[0],mac_raw[1],mac_raw[2],mac_raw[3],mac_raw[4],mac_raw[5], myToken
  );
}

void handleNetworking(){
  uint32_t now = millis();
  
  if(currentMode != AUTO) return;
  
  switch(fsmState){
    case FOLLOWER: {
      if(chunkMask == ((1u << ((NUM_LEDS + 74) / 75)) - 1u)){
        FastLED.show(); 
        chunkMask = 0;
      }
      
      uint32_t timeSinceLastMsg = now - lastRecvMillis;
      if(timeSinceLastMsg > LEADER_TIMEOUT){
        missedFrameCount++;
        if(missedFrameCount >= 3) {
          fsmState = ELECT;
          electionStart = now; 
          electionEnd = now + ELECTION_TIMEOUT;
          highestTokenSeen = myToken;
          myDelay = ((0xFFFFFFFF - myToken) * ELECTION_BASE_DELAY) / 0xFFFFFFFF
                    + random(0, ELECTION_JITTER);
          electionBroadcasted = false;
          missedFrameCount = 0;
          Serial.printf("FSM: FOLLOWER→ELECT (timeout=%ums) token=0x%06X delay=%ums\n",
            timeSinceLastMsg, myToken, myDelay
          );
        } else {
          Serial.printf("FSM: FOLLOWER timeout warning (%ums) - missed count: %u/3\n", 
            timeSinceLastMsg, missedFrameCount
          );
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
        Serial.println("FSM: ELECT broadcast token");
      }
      if(now >= electionEnd){
        if(highestTokenSeen > myToken){
          fsmState = FOLLOWER; 
          lastRecvMillis = now;
          Serial.printf("FSM: ELECT lost→FOLLOWER (high=0x%06X)\n", highestTokenSeen);
        } else {
          fsmState = LEADER;
          Serial.printf("FSM: ELECT won→LEADER (high=0x%06X)\n", highestTokenSeen);
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
        fsmState = FOLLOWER; 
        lastRecvMillis = now; 
        chunkMask = 0;
        missedFrameCount = 0;
        Serial.printf("FSM: LEADER saw higher token→FOLLOWER (0x%06X)\n", highestTokenSeen);
        break;
      }
      
      if(freezeActive) {
        detectAudioFrame();
        if(audioDetected) effectMusic();
        else             effectWildBG();
      } else {
        detectAudioFrame();
        if(audioDetected) runTimed(effectMusic);
        else             runTimed(effectWildBG);
      }
      FastLED.show();
      sendRaw();
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
      Serial.printf("Heartbeat: token=0x%06X (leader alive)\n", incomingToken);
    }
    return;
  }
  
  if(len < 10 || data[0] != MSGTYPE_RAW) return;
  
  uint32_t incomingToken; 
  memcpy(&incomingToken, data+5, 4);
  
  Serial.printf("onRecv: fsm=%s my=0x%06X in=0x%06X seq=%u\n",
    (fsmState==LEADER?"LEADER":fsmState==FOLLOWER?"FOLLOWER":"ELECT"),
    myToken, incomingToken, *(uint32_t*)(data+1)
  );
  
  if(fsmState == LEADER && incomingToken > myToken){
    Serial.printf("  >> Conflict: stepping DOWN (saw higher token)\n");
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
  uint8_t buf[1+4+4+1+75*3], bri = FastLED.getBrightness();
  
  for(int c = 0; c < chunks; c++){
    int base = c * 75, cnt = min(75, NUM_LEDS - base);
    buf[0] = MSGTYPE_RAW;
    memcpy(buf+1, &masterSeq, 4);
    memcpy(buf+5, &myToken, 4);
    buf[9] = c;
    
    for(int i = 0; i < cnt; i++){
      CRGB &led = leds[base + i];
      buf[10 + i*3    ] = scale8(led.r, bri);
      buf[10 + i*3 + 1] = scale8(led.g, bri);
      buf[10 + i*3 + 2] = scale8(led.b, bri);
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