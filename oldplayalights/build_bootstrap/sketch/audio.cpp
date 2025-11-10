#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/audio.cpp"
#include "audio.h"

void initAudio(){
  M5.Mic.begin(); 
  M5.Mic.setSampleRate(MIC_SR);
  lastBpmMillis = millis();
}

void detectAudioFrame(){
  static int16_t micBuf[MIC_BUF_LEN];
  if(!M5.Mic.record(micBuf, MIC_BUF_LEN)) return;
  
  long sum = 0; 
  for(auto &v : micBuf) sum += abs(v);
  float raw = float(sum) / MIC_BUF_LEN / 32767.0f;
  
  soundMin = min(raw, SMOOTH * soundMin + (1 - SMOOTH) * raw);
  soundMax = max(raw, SMOOTH * soundMax + (1 - SMOOTH) * raw);
  musicLevel = constrain((raw - soundMin) / (soundMax - soundMin + 1e-6f), 0.0f, 1.0f);
  
  bool above = (musicLevel > 0.6f);
  if(above && !prevAbove){
    uint32_t t = millis();
    if(beatCount < 50) {
      beatTimes[beatCount++] = t;
    } else { 
      memmove(beatTimes, beatTimes + 1, 49 * sizeof(uint32_t)); 
      beatTimes[49] = t; 
    }
  }
  prevAbove = above;
}

void updateBPM(){
  uint32_t now = millis();
  if(now - lastBpmMillis >= BPM_WINDOW){
    int cnt = 0; 
    uint32_t cutoff = now - BPM_WINDOW;
    for(int i = 0; i < beatCount; i++) {
      if(beatTimes[i] >= cutoff) cnt++;
    }
    
    float bpm = cnt * (60000.0f / float(BPM_WINDOW));
    audioDetected = (cnt >= 4 && bpm >= 30.0f && bpm <= 300.0f);
    lastBpmMillis += BPM_WINDOW; 
    beatCount = 0;
    
    Serial.printf("BPM: %d→%.1f [%s]\n", cnt, bpm, audioDetected ? "Music" : "Bg");
  }
}