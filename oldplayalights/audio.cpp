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
  
  // Adaptive sensitivity for high volume environments
  float dynamicRange = soundMax - soundMin;
  const float MIN_DYNAMIC_RANGE = 0.08f;  // Minimum range for reliable beat detection
  const float HIGH_VOLUME_THRESHOLD = 0.7f;  // When average level indicates high volume environment
  
  float adaptedMin = soundMin;
  float adaptedMax = soundMax;
  float beatThreshold = 0.6f;  // Default threshold
  
  // Detect high volume saturation scenario
  bool highVolumeEnvironment = (soundMin > HIGH_VOLUME_THRESHOLD) || (dynamicRange < MIN_DYNAMIC_RANGE);
  
  if(highVolumeEnvironment) {
    // In high volume environments, expand the dynamic range artificially
    if(dynamicRange < MIN_DYNAMIC_RANGE) {
      float expansion = (MIN_DYNAMIC_RANGE - dynamicRange) * 0.5f;
      adaptedMin = max(0.0f, soundMin - expansion);
      adaptedMax = min(1.0f, soundMax + expansion);
    }
    
    // Lower beat detection threshold in high volume environments
    beatThreshold = 0.35f;
    
    if(DEBUG_BPM && millis() % 2000 < 50) {
      Serial.printf("HIGH-VOL: range=%.3f->%.3f, thresh=%.2f, raw=%.3f\n", 
        dynamicRange, adaptedMax - adaptedMin, beatThreshold, raw);
    }
  }
  
  musicLevel = constrain((raw - adaptedMin) / (adaptedMax - adaptedMin + 1e-6f), 0.0f, 1.0f);
  
  bool above = (musicLevel > beatThreshold);
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
    
    if(DEBUG_BPM) {
      Serial.printf("BPM: %d→%.1f [%s]\n", cnt, bpm, audioDetected ? "Music" : "Bg");
    }
  }
}