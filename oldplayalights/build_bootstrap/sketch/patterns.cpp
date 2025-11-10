#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/patterns.cpp"
#include "patterns.h"

// ── Names ─────────────────────────────────────────────────────────────────────
const char* STYLE_NAMES[22] = {
  "Rainbow","Chase","Juggle","Rainbow+Glitter",
  "Confetti","BPM","Fire","Color Wheel","Random",
  "Pulse Wave","Meteor Shower","Color Spiral","Plasma Field",
  "Sparkle Storm","Aurora Waves","Organic Flow","Wave Collapse",
  "Color Drift","Liquid Rainbow","Sine Breath","Fractal Noise",
  "Rainbow Strobe"
};

// ── Basic Pattern Functions ───────────────────────────────────────────────────
static inline void addGlitter(fract8 c){ 
  if(random8()<c) leds[random16(NUM_LEDS)] += CRGB::White; 
}

void styleRainbow(uint8_t sp){ 
  static uint8_t h; 
  h+=sp; 
  fill_rainbow(leds,NUM_LEDS,h,1); 
}

void styleChase(uint8_t sp){
  static uint32_t last; 
  static uint16_t pos; 
  static uint8_t hue;
  uint32_t now=millis();
  if(now-last<map(9-sp,0,9,5,200)) return;
  last=now; pos=(pos+1)%NUM_LEDS;
  fadeToBlackBy(leds,NUM_LEDS,map(getDe(),0,9,50,4));
  for(int i=0;i<NUM_LEDS;i+=20)
    for(int t=0;t<10;t++){
      int idx=(pos+i+NUM_LEDS-t)%NUM_LEDS;
      leds[idx] |= CHSV(hue+i,255,map(t,0,9,255,50));
    }
  hue++;
}

void styleJuggle(uint8_t sp){
  static uint8_t h; 
  uint16_t bpm=map(sp,0,9,10,120);
  fadeToBlackBy(leds,NUM_LEDS,map(getDe(),0,9,20,200));
  for(int i=0;i<4;i++) 
    leds[beatsin16(bpm,0,NUM_LEDS-1,i*20)] |= CHSV(h+=64,200,255);
}

void styleRainbowGlitter(uint8_t sp){ 
  styleRainbow(sp); 
  addGlitter(getSS()*25); 
}

void styleConfetti(uint8_t sp){
  static uint8_t h;
  fadeToBlackBy(leds,NUM_LEDS,map(getDe(),0,9,10,100));
  addGlitter(getSS()*25);
  for(int i=0;i<sp*2;i++) 
    leds[random16(NUM_LEDS)] |= CHSV(h+random8(64),200,255);
  h++;
}

void styleBPM(uint8_t sp){
  static uint8_t h; 
  uint16_t bpm=map(sp,0,9,30,300);
  CRGBPalette16 pal=PartyColors_p; 
  uint8_t beat=beatsin8(bpm,64,255);
  for(int i=0;i<NUM_LEDS;i++) 
    leds[i]=ColorFromPalette(pal,h+i*2,beat-h+i*10);
  blur1d(leds,NUM_LEDS,map(getDe(),0,9,20,200)); 
  h++;
}

void styleFire(uint8_t sp){
  static uint8_t heat[NUM_LEDS/2]; 
  int half=NUM_LEDS/2;
  uint8_t cool=map(sp,0,9,100,20), spark=map(sp,0,9,50,200);
  for(int i=0;i<half;i++) 
    heat[i]=qsub8(heat[i],random8(0,((cool*10)/half)+2));
  for(int k=half-1;k>=2;k--) 
    heat[k]=(heat[k-1]+heat[k-2]+heat[k-2])/3;
  if(random8()<spark) 
    heat[random8(7)] += random8(160,240);
  for(int j=0;j<half;j++){
    CRGB c=ColorFromPalette(HeatColors_p, scale8(heat[j],200));
    leds[half+j]=leds[half-1-j]=c;
  }
}

void styleColorWheel(uint8_t sp){
  static uint8_t hue = 0;
  uint8_t hueSpeed = map(sp, 0, 9, 1, 12);
  hue += hueSpeed;
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
}

void styleRandom(uint8_t sp){
  fadeToBlackBy(leds,NUM_LEDS,map(getDe(),0,9,10,100));
  for(int i=0;i<sp;i++) 
    leds[random16(NUM_LEDS)] = CHSV(random8(),200,255);
}

// ── Creative Patterns ─────────────────────────────────────────────────────────
void stylePulseWave(uint8_t sp){
  static uint8_t center = NUM_LEDS/2;
  static uint8_t hue = 0;
  static uint8_t wave = 0;
  
  fadeToBlackBy(leds, NUM_LEDS, map(getDe(),0,9,30,150));
  
  uint8_t waveSpeed = map(sp, 0, 9, 1, 8);
  wave += waveSpeed;
  
  for(int i = 0; i < NUM_LEDS; i++){
    uint8_t distance = abs(i - center);
    uint8_t brightness = sin8(wave - distance * 8);
    if(brightness > 128){
      CRGB color = CHSV(hue + distance * 2, 255 - getSS() * 20, brightness);
      leds[i] += color;
    }
  }
  
  hue += 2;
  if(random8() < 5) center = random16(NUM_LEDS/4, 3*NUM_LEDS/4);
}

void styleMeteorShower(uint8_t sp){
  struct Meteor { int16_t pos; uint8_t hue; uint8_t size; int8_t speed; };
  static Meteor meteors[8];
  static bool initialized = false;
  
  if(!initialized){
    for(int i = 0; i < 8; i++){
      meteors[i] = {-20, (uint8_t)random8(), (uint8_t)(3 + random8(5)), (int8_t)(1 + random8(3))};
    }
    initialized = true;
  }
  
  fadeToBlackBy(leds, NUM_LEDS, map(getDe(),0,9,20,120));
  
  for(int m = 0; m < 8; m++){
    Meteor &meteor = meteors[m];
    
    for(int t = 0; t < meteor.size; t++){
      int16_t trailPos = meteor.pos - t;
      if(trailPos >= 0 && trailPos < NUM_LEDS){
        uint8_t brightness = map(t, 0, meteor.size-1, 255, 50);
        leds[trailPos] += CHSV(meteor.hue + random8(getSS()*5), 200, brightness);
      }
    }
    
    if(random8() < map(sp, 0, 9, 30, 200)){
      meteor.pos += meteor.speed;
    }
    
    if(meteor.pos >= NUM_LEDS + meteor.size){
      meteor.pos = -meteor.size;
      meteor.hue = random8();
      meteor.size = 3 + random8(5);
      meteor.speed = 1 + random8(3);
    }
  }
}

void styleColorSpiral(uint8_t sp){
  static uint16_t spiral_pos = 0;
  static uint8_t hue_offset = 0;
  
  uint8_t spiralSpeed = map(sp, 0, 9, 1, 12);
  spiral_pos += spiralSpeed;
  hue_offset += 1;
  
  for(int i = 0; i < NUM_LEDS; i++){
    uint8_t spiral_hue = hue_offset + (spiral_pos/16) + sin8((i * 256 / NUM_LEDS) + spiral_pos/4) / 8;
    uint8_t brightness = sin8(spiral_pos/2 + i * 8) + 128;
    
    if(random8() < getSS() * 10){
      spiral_hue += random8(30);
    }
    
    leds[i] = CHSV(spiral_hue, 240, brightness);
  }
  
  if(random8() < 3){
    hue_offset += random8(60);
  }
}

void stylePlasmaField(uint8_t sp){
  static uint16_t time_counter = 0;
  static uint8_t plasma_hue = 0;
  static uint8_t wave_offset1 = 0, wave_offset2 = 85, wave_offset3 = 170;
  static uint8_t drift_counter = 0;
  
  uint8_t plasmaSpeed = map(sp, 0, 9, 1, 8);
  time_counter += plasmaSpeed;
  plasma_hue += 1;
  
  if(++drift_counter > 60) {
    drift_counter = 0;
    wave_offset1 += random8(3) - 1;
    wave_offset2 += random8(3) - 1;
    wave_offset3 += random8(3) - 1;
  }
  
  for(int i = 0; i < NUM_LEDS; i++){
    uint8_t layer1 = sin8(time_counter/4 + i * 8 + wave_offset1);
    uint8_t layer2 = sin8(time_counter/3 + i * 6 + wave_offset2);
    uint8_t layer3 = sin8(time_counter/5 + i * 4 + wave_offset3);
    uint8_t layer4 = sin8(time_counter/7 + i * 12);
    
    uint8_t combined = (layer1/4 + layer2/3 + layer3/3 + layer4/6);
    uint8_t hue = plasma_hue + combined/2 + sin8(time_counter/6 + i * 3)/4;
    uint8_t saturation = 200 + (sin8(layer1 + layer2)/4);
    
    if(random8() < getSS() * 6){
      hue += random8(getSS() * 8);
    }
    
    uint8_t brightness = combined + sin8(time_counter/8 + i)/4;
    leds[i] = CHSV(hue, saturation, brightness);
  }
}

void styleSparkleStorm(uint8_t sp){
  static uint8_t storm_intensity = 0;
  static uint8_t base_hue = 0;
  
  if(random8() < 10){
    storm_intensity = random8(50, 255);
  }
  storm_intensity = scale8(storm_intensity, 250);
  
  fadeToBlackBy(leds, NUM_LEDS, map(getDe(),0,9,10,80));
  
  uint8_t sparkle_count = map(sp, 0, 9, 2, 20);
  uint8_t storm_sparkles = (storm_intensity * sparkle_count) / 255;
  
  for(int i = 0; i < sparkle_count; i++){
    if(random8() < 150){
      uint16_t pos = random16(NUM_LEDS);
      uint8_t hue = base_hue + random8(getSS() * 30);
      leds[pos] = CHSV(hue, 200 + random8(55), 200 + random8(55));
    }
  }
  
  for(int i = 0; i < storm_sparkles; i++){
    uint16_t pos = random16(NUM_LEDS);
    uint8_t hue = base_hue + random8(60);
    leds[pos] = CHSV(hue, 255, 255);
  }
  
  base_hue += 2;
}

void styleAuroraWaves(uint8_t sp){
  static uint16_t wave1_pos = 0, wave2_pos = 0, wave3_pos = 0;
  static uint8_t aurora_hue = 96;
  
  uint8_t waveSpeed = map(sp, 0, 9, 1, 5);
  wave1_pos += waveSpeed;
  wave2_pos += waveSpeed * 2;
  wave3_pos += waveSpeed / 2;
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  for(int i = 0; i < NUM_LEDS; i++){
    uint8_t wave1 = sin8(wave1_pos + i * 4);
    uint8_t wave2 = sin8(wave2_pos + i * 6 + 85);
    uint8_t wave3 = sin8(wave3_pos + i * 2 + 170);
    
    uint8_t hue1 = aurora_hue + sin8(i * 8)/8;
    uint8_t hue2 = aurora_hue + 40 + sin8(i * 6)/6;
    uint8_t hue3 = aurora_hue + 80 + sin8(i * 4)/4;
    
    if(random8() < getSS() * 12){
      hue1 += random8(20);
      hue2 += random8(20);
      hue3 += random8(20);
    }
    
    if(wave1 > 100) leds[i] += CHSV(hue1, 255, (wave1-100)*2);
    if(wave2 > 120) leds[i] += CHSV(hue2, 240, (wave2-120)*3);
    if(wave3 > 140) leds[i] += CHSV(hue3, 200, (wave3-140)*4);
  }
  
  if(random8() < 2){
    aurora_hue += random8(10) - 5;
    aurora_hue = constrain(aurora_hue, 80, 140);
  }
}

// ── Organic Patterns ──────────────────────────────────────────────────────────
void styleOrganicFlow(uint8_t sp){
  static uint16_t flow_time = 0;
  static uint8_t base_hue = 0;
  static float node_positions[8];
  static float node_velocities[8];
  static uint8_t node_hues[8];
  static bool initialized = false;
  
  if(!initialized) {
    for(int i = 0; i < 8; i++) {
      node_positions[i] = random(NUM_LEDS);
      node_velocities[i] = (random(20) - 10) / 10.0f;
      node_hues[i] = random(256);
    }
    initialized = true;
  }
  
  uint8_t flowSpeed = map(sp, 0, 9, 1, 6);
  flow_time += flowSpeed;
  base_hue += 1;
  
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(map(getDe(), 0, 9, 240, 200));
  }
  
  for(int n = 0; n < 8; n++) {
    if(random8() < 5) {
      node_velocities[n] += (random(10) - 5) / 20.0f;
      node_velocities[n] = constrain(node_velocities[n], -2.0f, 2.0f);
    }
    
    node_positions[n] += node_velocities[n];
    if(node_positions[n] < 0) node_positions[n] = NUM_LEDS - 1;
    if(node_positions[n] >= NUM_LEDS) node_positions[n] = 0;
    
    node_hues[n] += random8(3);
    
    int center = (int)node_positions[n];
    for(int spread = -6; spread <= 6; spread++) {
      int pos = (center + spread + NUM_LEDS) % NUM_LEDS;
      float distance = abs(spread) / 6.0f;
      uint8_t brightness = 255 * (1.0f - distance * distance);
      uint8_t hue = base_hue + node_hues[n] + random8(getSS() * 10);
      
      leds[pos] += CHSV(hue, 200 + random8(55), brightness);
    }
  }
}

void styleWaveCollapse(uint8_t sp){
  static uint16_t wave_time = 0;
  static uint8_t collapse_hue = 160;
  static int16_t collapse_center = NUM_LEDS / 2;
  static uint8_t collapse_phase = 0;
  static uint8_t wave_radius = 0;
  
  uint8_t waveSpeed = map(sp, 0, 9, 1, 8);
  wave_time += waveSpeed;
  
  if(random8() < 3) {
    collapse_center += random8(10) - 5;
    collapse_center = constrain(collapse_center, 20, NUM_LEDS - 20);
  }
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  if(collapse_phase == 0) {
    wave_radius += waveSpeed;
    if(wave_radius > NUM_LEDS/2) {
      collapse_phase = 1;
      collapse_hue += 60 + random8(getSS() * 30);
    }
  } else {
    if(wave_radius > 0) wave_radius -= waveSpeed * 2;
    else {
      collapse_phase = 0;
      wave_radius = 0;
      collapse_hue += 90 + random8(40);
    }
  }
  
  for(int i = 0; i < NUM_LEDS; i++) {
    float distance = abs(i - collapse_center);
    float wave_dist = distance - wave_radius;
    
    float wave1 = sin((wave_dist * 0.3f) + (wave_time * 0.1f));
    float wave2 = sin((wave_dist * 0.5f) + (wave_time * 0.07f));
    float wave3 = sin((wave_dist * 0.2f) + (wave_time * 0.13f));
    
    float combined = (wave1 + wave2 * 0.7f + wave3 * 0.5f) / 2.2f;
    
    if(combined > 0) {
      uint8_t brightness = combined * 255;
      uint8_t hue = collapse_hue + distance * 2 + sin8(wave_time + i * 8) / 8;
      if(random8() < getSS() * 8) hue += random8(20);
      
      leds[i] = CHSV(hue, 240, brightness);
    }
  }
}

void styleColorDrift(uint8_t sp){
  static uint8_t drift_hues[NUM_LEDS];
  static float drift_velocities[NUM_LEDS];
  static bool initialized = false;
  static uint16_t drift_time = 0;
  
  if(!initialized) {
    for(int i = 0; i < NUM_LEDS; i++) {
      drift_hues[i] = random(256);
      drift_velocities[i] = (random(40) - 20) / 100.0f;
    }
    initialized = true;
  }
  
  uint8_t driftSpeed = map(sp, 0, 9, 1, 10);
  drift_time += driftSpeed;
  
  for(int i = 0; i < NUM_LEDS; i++) {
    uint8_t left_hue = (i > 0) ? drift_hues[i-1] : drift_hues[NUM_LEDS-1];
    uint8_t right_hue = (i < NUM_LEDS-1) ? drift_hues[i+1] : drift_hues[0];
    
    float influence = 0.02f;
    drift_hues[i] = drift_hues[i] * (1.0f - influence) + 
                    (left_hue + right_hue) * influence * 0.5f;
    
    if(random8() < 5) {
      drift_velocities[i] += (random(20) - 10) / 500.0f;
      drift_velocities[i] = constrain(drift_velocities[i], -0.5f, 0.5f);
    }
    
    drift_hues[i] += drift_velocities[i] * driftSpeed;
    
    if(random8() < getSS() * 2) {
      drift_hues[i] += random8(getSS() * 15) - getSS() * 7;
    }
    
    uint8_t brightness = 150 + abs(drift_velocities[i]) * 2000 + 
                        sin8(drift_time + i * 16) / 4;
    uint8_t saturation = 180 + sin8(drift_time * 2 + i * 8) / 4;
    
    leds[i] = CHSV(drift_hues[i], saturation, brightness);
  }
}

void styleLiquidRainbow(uint8_t sp){
  static uint16_t liquid_time = 0;
  static float wave_phases[5] = {0, 85, 170, 42, 213};
  static float wave_speeds[5] = {1.0f, 1.3f, 0.7f, 1.7f, 0.9f};
  
  uint8_t liquidSpeed = map(sp, 0, 9, 1, 8);
  liquid_time += liquidSpeed;
  
  for(int w = 0; w < 5; w++) {
    wave_phases[w] += wave_speeds[w] * liquidSpeed;
    if(random8() < 2) {
      wave_speeds[w] += (random(10) - 5) / 100.0f;
      wave_speeds[w] = constrain(wave_speeds[w], 0.3f, 2.5f);
    }
  }
  
  for(int i = 0; i < NUM_LEDS; i++) {
    float wave_sum = 0;
    for(int w = 0; w < 5; w++) {
      wave_sum += sin8(wave_phases[w] + i * (8 + w * 2)) / 255.0f;
    }
    wave_sum /= 5.0f;
    
    uint8_t hue = (wave_sum + 1.0f) * 128 + liquid_time / 4;
    uint8_t saturation = 200 + sin8(liquid_time + i * 6) / 8;
    uint8_t brightness = 180 + wave_sum * 75 + sin8(liquid_time * 2 + i * 4) / 6;
    
    if(random8() < getSS() * 4) {
      hue += random8(getSS() * 20);
      brightness = 255;
    }
    
    leds[i] = CHSV(hue, saturation, brightness);
  }
}

void styleSineBreath(uint8_t sp){
  static uint16_t breath_time = 0;
  static uint8_t breath_hue = 64;
  static uint8_t hue_drift_timer = 0;
  
  uint8_t breathSpeed = map(sp, 0, 9, 1, 6);
  breath_time += breathSpeed;
  
  if(++hue_drift_timer > 30) {
    hue_drift_timer = 0;
    breath_hue += random8(5) - 2;
    if(breath_hue < 40 || breath_hue > 180) {
      breath_hue += (random8(2) == 0) ? 10 : -10;
    }
  }
  
  float master_breath = (sin8(breath_time) / 255.0f + 1.0f) / 2.0f;
  
  for(int i = 0; i < NUM_LEDS; i++) {
    float center_distance = abs(i - NUM_LEDS/2) / (float)(NUM_LEDS/2);
    
    float breath1 = sin8(breath_time + i * 4) / 255.0f;
    float breath2 = sin8(breath_time * 0.7f + i * 2) / 255.0f;
    float breath3 = sin8(breath_time * 1.3f + center_distance * 100) / 255.0f;
    
    float combined_breath = (breath1 + breath2 + breath3 + master_breath) / 4.0f;
    
    uint8_t brightness = 50 + combined_breath * 200;
    uint8_t hue = breath_hue + center_distance * 30 + combined_breath * 20;
    
    if(random8() < getSS() * 3) {
      brightness = 255;
      hue += random8(getSS() * 15);
    }
    
    uint8_t saturation = 180 + combined_breath * 50;
    leds[i] = CHSV(hue, saturation, brightness);
  }
}

void styleFractalNoise(uint8_t sp){
  static uint16_t noise_time = 0;
  static uint8_t noise_hue_base = 0;
  static float noise_scale = 0.1f;
  
  uint8_t noiseSpeed = map(sp, 0, 9, 1, 8);
  noise_time += noiseSpeed;
  noise_hue_base += 1;
  
  if(random8() < 3) {
    noise_scale += (random(10) - 5) / 1000.0f;
    noise_scale = constrain(noise_scale, 0.05f, 0.3f);
  }
  
  for(int i = 0; i < NUM_LEDS; i++) {
    float noise1 = sin8(noise_time + i * 8 * noise_scale * 100) / 255.0f;
    float noise2 = sin8(noise_time * 1.3f + i * 16 * noise_scale * 100) / 255.0f;
    float noise3 = sin8(noise_time * 0.7f + i * 32 * noise_scale * 100) / 255.0f;
    float noise4 = sin8(noise_time * 2.1f + i * 4 * noise_scale * 100) / 255.0f;
    
    float combined_noise = (noise1 * 0.5f + noise2 * 0.3f + noise3 * 0.15f + noise4 * 0.05f);
    
    uint8_t hue = noise_hue_base + combined_noise * 120 + 
                  sin8(noise_time / 3 + i * 2) / 8;
    uint8_t brightness = 100 + combined_noise * 155;
    uint8_t saturation = 150 + combined_noise * 80 + 
                        sin8(noise_time * 1.5f + i * 6) / 6;
    
    if(random8() < getSS() * 5) {
      hue += random8(getSS() * 25) - getSS() * 12;
      brightness += random8(50);
    }
    
    leds[i] = CHSV(hue, saturation, brightness);
  }
}

void styleRainbowStrobe(uint8_t sp){
  static uint8_t hue = 0;
  static uint8_t strobe_counter = 0;
  static bool strobe_on = true;
  
  // ULTRA fast strobe rate for maximum persistence of vision effect
  uint8_t strobeSpeed = map(sp, 0, 9, 60, 160);  // 4x faster than before!
  strobe_counter += strobeSpeed;
  
  // Advance hue extremely rapidly - about 80Hz color changes
  hue += map(sp, 0, 9, 32, 80); // 4x faster hue advancement
  
  // Toggle strobe state extremely rapidly - about 80Hz
  if(strobe_counter > 16) { // 4x faster strobe frequency
    strobe_counter = 0;
    strobe_on = !strobe_on;
  }
  
  if(strobe_on) {
    // Cap brightness at about half (128 instead of 255) for eye comfort
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 128));
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

// ── Effect Control Functions ──────────────────────────────────────────────────
void effectWild(){
  switch(styleIdx){
    case 0: styleRainbow(getSpeed());      break;
    case 1: styleChase(getSpeed());        break;
    case 2: styleJuggle(getSpeed());       break;
    case 3: styleRainbowGlitter(getSpeed()); break;
    case 4: styleConfetti(getSpeed());     break;
    case 5: styleBPM(getSpeed());          break;
    case 6: styleFire(getSpeed());         break;
    case 7: styleColorWheel(getSpeed());   break;
    case 8: styleRandom(getSpeed());       break;
    case 9: stylePulseWave(getSpeed());    break;
    case 10: styleMeteorShower(getSpeed()); break;
    case 11: styleColorSpiral(getSpeed());  break;
    case 12: stylePlasmaField(getSpeed());  break;
    case 13: styleSparkleStorm(getSpeed()); break;
    case 14: styleAuroraWaves(getSpeed());  break;
    case 15: styleOrganicFlow(getSpeed());  break;
    case 16: styleWaveCollapse(getSpeed()); break;
    case 17: styleColorDrift(getSpeed());   break;
    case 18: styleLiquidRainbow(getSpeed()); break;
    case 19: styleSineBreath(getSpeed());   break;
    case 20: styleFractalNoise(getSpeed()); break;
    case 21: styleRainbowStrobe(getSpeed()); break;
  }
}

void effectWildBG(){ 
  FastLED.setBrightness(map(getBright(),0,9,0,255)); 
  effectWild(); 
}

void effectMusic(){ 
  FastLED.setBrightness(uint8_t(musicLevel*255)); 
  effectWild(); 
}

void runTimed(void (*fn)()){
  static uint32_t lastT=0;
  uint32_t now=millis(), d=getTi()*5000;
  if(d==0||now-lastT>=d){ 
    styleIdx=(styleIdx+1)%22; 
    lastT=now; 
  }
  fn();
}