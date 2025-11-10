#include "patterns.h"

// ── Names ─────────────────────────────────────────────────────────────────────
const char* STYLE_NAMES[42] = {
  "Rainbow","Chase","Juggle","Rainbow+Glitter",
  "Confetti","BPM","Fire","Color Wheel","Random",
  "Pulse Wave","Meteor Shower","Color Spiral","Plasma Field",
  "Sparkle Storm","Aurora Waves","Organic Flow","Wave Collapse",
  "Color Drift","Liquid Rainbow","Sine Breath","Fractal Noise",
  "Rainbow Strobe",
  // NEW PATTERNS (22-41)
  "Twinkle Stars","Rainbow Ripples","DNA Helix","Neon Pulse",
  "Digital Rain","Plasma Balls","Lightning Storm","Kaleidoscope",
  "Candle Flicker","Color Drips","Galaxy Spiral","Prism",
  "Heartbeat","Aurora Boreal","Matrix Code","Crystal Cave",
  "Lava Flow","Waveform","Rainbow2","Confetti2"
};

// ── Basic Pattern Functions ───────────────────────────────────────────────────
static inline void addGlitter(fract8 c){ 
  if(random8()<c) {
    // Glitter at FULL brightness - brightness scaling happens later
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void styleRainbow(uint8_t sp){ 
  static uint8_t h; 
  h+=sp; 
  fill_rainbow(leds,NUM_LEDS,h,1);
  // NO brightness scaling here - patterns generate at full brightness
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
  
  // Create the color at FULL brightness - scaling happens later
  CRGB color = CHSV(hue, 255, 255);
  fill_solid(leds, NUM_LEDS, color);
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
    
    CRGB color1 = CRGB::Black;
    CRGB color2 = CRGB::Black;
    CRGB color3 = CRGB::Black;
    
    if(wave1 > 100) {
      color1 = CHSV(hue1, 255, (wave1-100)*2);
    }
    if(wave2 > 120) {
      color2 = CHSV(hue2, 240, (wave2-120)*3);
    }
    if(wave3 > 140) {
      color3 = CHSV(hue3, 200, (wave3-140)*4);
    }
    
    leds[i] += color1;
    leds[i] += color2;
    leds[i] += color3;
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
  
  // Moderate strobe rate to prevent system overload and reboots
  uint8_t strobeSpeed = map(sp, 0, 9, 15, 40);  // Reduced from extreme values
  strobe_counter += strobeSpeed;
  
  // Moderate hue advancement - about 20Hz color changes
  hue += map(sp, 0, 9, 8, 20); // Reduced from extreme values
  
  // Toggle strobe state at moderate frequency - about 20Hz
  if(strobe_counter > 32) { // Slower strobe to prevent reboots
    strobe_counter = 0;
    strobe_on = !strobe_on;
  }
  
  if(strobe_on) {
    // Create the color at FULL brightness - scaling happens later
    CRGB color = CHSV(hue, 255, 255);
    fill_solid(leds, NUM_LEDS, color);
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
    // NEW PATTERNS (20 additional patterns)
    case 22: styleTwinkleStars(getSpeed()); break;
    case 23: styleRainbowRipples(getSpeed()); break;
    case 24: styleDNAHelix(getSpeed()); break;
    case 25: styleNeonPulse(getSpeed()); break;
    case 26: styleDigitalRain(getSpeed()); break;
    case 27: stylePlasmaBalls(getSpeed()); break;
    case 28: styleLightningStorm(getSpeed()); break;
    case 29: styleKaleidoscope(getSpeed()); break;
    case 30: styleCandleFlicker(getSpeed()); break;
    case 31: styleColorDrips(getSpeed()); break;
    case 32: styleGalaxySpiral(getSpeed()); break;
    case 33: stylePrism(getSpeed()); break;
    case 34: styleHeartbeat(getSpeed()); break;
    case 35: styleAuroraBoreal(getSpeed()); break;
    case 36: styleMatrixCode(getSpeed()); break;
    case 37: styleCrystalCave(getSpeed()); break;
    case 38: styleLavaFlow(getSpeed()); break;
    case 39: styleWaveform(getSpeed()); break;
    case 40: styleRainbow(getSpeed()); break; // Safe duplicate of pattern 0
    case 41: styleConfetti(getSpeed()); break; // Safe duplicate of pattern 4
  }
}

void effectWildBG(){ 
  effectWild(); 
  // NO brightness scaling here - patterns generate at full brightness
}

void effectMusic(){ 
  effectWild();
  
  // CRITICAL FIX: Apply dramatic music scaling to FULL BRIGHTNESS patterns
  // Enhanced responsiveness: much more dramatic nearly-off to full-bright response
  
  // Always apply music scaling with enhanced contrast
  // musicLevel ranges from 0.0 to 1.0, use the full range for maximum drama
  float enhancedMusicLevel = musicLevel;
  
  // Boost the contrast: square the music level for more dramatic response
  // This makes quiet sections much dimmer and loud sections stay bright
  enhancedMusicLevel = enhancedMusicLevel * enhancedMusicLevel;
  
  // Set minimum to very low (3% instead of 12%) for dramatic contrast
  const float MIN_BRIGHTNESS = 0.03f;  // 3% minimum
  const float MAX_BRIGHTNESS = 1.0f;   // 100% maximum
  
  // Scale from 3% to 100% based on enhanced music level
  float finalLevel = MIN_BRIGHTNESS + (enhancedMusicLevel * (MAX_BRIGHTNESS - MIN_BRIGHTNESS));
  
  uint8_t musicScale = uint8_t(finalLevel * 255);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(musicScale);
  }
}

// ── NEW PATTERNS: Inspired by Pixelblaze Community ──────────────────────────
// stylePerlinWaves removed due to system crashes - too computationally intensive

void styleTwinkleStars(uint8_t sp) {
  static uint8_t density = 80;
  if (random8() < density) {
    leds[random16(NUM_LEDS)] += CHSV(random8(), 255, random8(100, 255));
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250 - sp / 4); // Fade based on speed
  }
}

void styleRainbowRipples(uint8_t sp) {
  static uint8_t center = NUM_LEDS / 2;
  static uint8_t step = 0;
  
  if (step == 0) {
    center = random8(NUM_LEDS);
    step = 1;
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t distance = abs(i - center);
    uint8_t brightness = sin8(distance * 8 - millis() / (20 - sp / 15));
    leds[i] = CHSV((distance * 4 + millis() / 100) % 255, 255, brightness);
  }
  
  if (millis() % 3000 < 50) step = 0; // New ripple every 3 seconds
}

void styleDNAHelix(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t angle1 = (millis() / (30 - sp / 10) + i * 8) % 255;
    uint8_t angle2 = (millis() / (30 - sp / 10) + i * 8 + 128) % 255;
    uint8_t bright1 = sin8(angle1);
    uint8_t bright2 = sin8(angle2);
    
    CRGB color1 = CHSV(160, 255, bright1); // Cyan strand
    CRGB color2 = CHSV(0, 255, bright2);   // Red strand
    leds[i] = color1 + color2;
  }
}

void styleNeonPulse(uint8_t sp) {
  static uint8_t hue = 0;
  uint8_t beat = sin8(millis() / (50 - sp / 6));
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t brightness = qadd8(beat, sin8(i * 4 + millis() / 100));
    leds[i] = CHSV(hue, 200, brightness);
  }
  hue += 1;
}

void styleDigitalRain(uint8_t sp) {
  // Fade all pixels
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(230); // Slower fade for more visible trails
  }
  
  // Add multiple new "drops" - much more active
  if (random8() < (sp + 40)) { // Much higher frequency
    leds[random8(NUM_LEDS / 3)] = CHSV(96, 255, 255); // Bright green from top third
  }
  
  // Add additional drops from various positions
  if (random8() < (sp / 2 + 25)) {
    leds[random8(NUM_LEDS / 5)] = CHSV(120, 255, 200); // Lighter green variation
  }
  
  // Add occasional bright white "data packets"
  if (random8() < (sp / 4 + 15)) {
    leds[random8(NUM_LEDS / 6)] = CHSV(0, 0, 255); // Bright white
  }
  
  // Rain effect - move pixels down (faster)
  static uint32_t lastMove = 0;
  if (millis() - lastMove > (60 - sp)) { // Faster movement
    for (int i = NUM_LEDS - 1; i > 0; i--) {
      if (leds[i-1].g > leds[i].g || (leds[i-1].r + leds[i-1].g + leds[i-1].b) > 50) {
        leds[i] = leds[i-1];
        leds[i-1].nscale8(180); // More visible trail
      }
    }
    lastMove = millis();
  }
}

void stylePlasmaBalls(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t x = i;
    uint8_t t = millis() / (30 - sp / 10);
    
    uint8_t plasma = sin8(x * 16 + t) + 
                    sin8(x * 23 + t * 2) + 
                    sin8(x * 33 + t * 3);
    
    uint8_t hue = plasma / 3 + millis() / 200;
    leds[i] = CHSV(hue, 255, plasma);
  }
}

void styleLightningStorm(uint8_t sp) {
  // Fade to dark blue background (less aggressive fading)
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(160, 255, 25); // Slightly brighter background
    leds[i].nscale8(220); // Less aggressive fade for more atmosphere
  }
  
  // Multiple lightning strikes - much more frequent and varied
  if (random8() < (sp / 4 + 15)) { // Much higher frequency
    uint8_t strike_pos = random8(NUM_LEDS - 30);
    uint8_t strike_len = random8(8, 25); // Longer strikes
    
    for (int i = strike_pos; i < strike_pos + strike_len && i < NUM_LEDS; i++) {
      leds[i] = CHSV(0, 0, 255); // Bright white
    }
  }
  
  // Add secondary smaller lightning strikes
  if (random8() < (sp / 6 + 10)) {
    uint8_t strike_pos = random8(NUM_LEDS - 15);
    uint8_t strike_len = random8(3, 12);
    
    for (int i = strike_pos; i < strike_pos + strike_len && i < NUM_LEDS; i++) {
      leds[i] = CHSV(45, 100, 200); // Yellowish lightning
    }
  }
  
  // Add occasional purple lightning (different voltage)
  if (random8() < (sp / 8 + 5)) {
    uint8_t strike_pos = random8(NUM_LEDS - 10);
    uint8_t strike_len = random8(2, 8);
    
    for (int i = strike_pos; i < strike_pos + strike_len && i < NUM_LEDS; i++) {
      leds[i] = CHSV(200, 150, 180); // Purple lightning
    }
  }
}

void styleKaleidoscope(uint8_t sp) {
  static uint8_t offset = 0;
  uint8_t center = NUM_LEDS / 2;
  
  for (int i = 0; i < center; i++) {
    uint8_t hue = (i * 8 + offset) % 255;
    uint8_t brightness = sin8(i * 16 + millis() / (40 - sp / 8));
    CRGB color = CHSV(hue, 255, brightness);
    
    leds[i] = color;
    leds[NUM_LEDS - 1 - i] = color; // Mirror effect
  }
  offset += sp / 20;
}

void styleCandleFlicker(uint8_t sp) {
  uint8_t base_hue = 20; // Warm orange
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t flicker = random8(180, 255);
    uint8_t hue_variation = base_hue + random8(20) - 10;
    leds[i] = CHSV(hue_variation, 255, flicker);
  }
  
  // Occasional brighter flickers
  if (random8() < (sp / 5 + 10)) {
    leds[random8(NUM_LEDS)] = CHSV(base_hue, 200, 255);
  }
}

void styleColorDrips(uint8_t sp) {
  // Fade all pixels
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(240); // Slower fade for more visible trails
  }
  
  // Add new drips from multiple positions (much more frequent)
  if (random8() < (sp + 60)) { // Significantly increased frequency
    uint8_t start_pos = random8(NUM_LEDS / 4); // Random starting position in top quarter
    leds[start_pos] = CHSV(random8(), 255, 255);
  }
  
  // Add occasional bright drips from the very top
  if (random8() < (sp / 2 + 30)) {
    leds[0] = CHSV(random8(), 255, 255);
  }
  
  // Move drips down (faster movement)
  static uint32_t lastMove = 0;
  if (millis() - lastMove > (80 - sp)) { // Much faster dripping
    for (int i = NUM_LEDS - 1; i > 0; i--) {
      if (leds[i-1].r + leds[i-1].g + leds[i-1].b > 20) { // Lower threshold for movement
        leds[i] = leds[i-1];
        leds[i-1].nscale8(200); // More visible trail
      }
    }
    lastMove = millis();
  }
}

void styleGalaxySpiral(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t angle = (i * 4 + millis() / (60 - sp / 5)) % 255;
    uint8_t radius = i * 255 / NUM_LEDS;
    
    uint8_t brightness = sin8(angle) * sin8(radius) / 255;
    uint8_t hue = angle / 2 + radius / 4;
    
    leds[i] = CHSV(hue, 255, brightness);
  }
}

void stylePrism(uint8_t sp) {
  static uint8_t rotation = 0;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t segment = (i * 6) / NUM_LEDS; // 6 color segments
    uint8_t hue = (segment * 42 + rotation) % 255; // Spread across spectrum
    uint8_t brightness = sin8((i * 8 + millis() / (30 - sp / 10)) % 255);
    
    leds[i] = CHSV(hue, 255, brightness);
  }
  rotation += sp / 30;
}

void styleHeartbeat(uint8_t sp) {
  static uint32_t lastBeat = 0;
  static bool inBeat = false;
  uint32_t now = millis();
  
  uint32_t beatInterval = 1200 - sp * 8; // Speed affects heart rate
  
  if (now - lastBeat > beatInterval) {
    lastBeat = now;
    inBeat = true;
  }
  
  uint8_t brightness = 0;
  if (inBeat) {
    uint32_t beatProgress = now - lastBeat;
    if (beatProgress < 100) {
      brightness = sin8(beatProgress * 255 / 100);
    } else if (beatProgress < 200) {
      brightness = sin8((beatProgress - 100) * 255 / 100) / 3;
    } else {
      inBeat = false;
    }
  }
  
  fill_solid(leds, NUM_LEDS, CHSV(0, 255, brightness)); // Red heartbeat
}

void styleAuroraBoreal(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t x = i * 255 / NUM_LEDS;
    uint8_t t = millis() / (100 - sp);
    
    uint8_t green = inoise8(x, t) / 2 + 127;
    uint8_t blue = inoise8(x + 1000, t + 1000) / 3 + 85;
    
    leds[i] = CRGB(0, green, blue);
  }
  
  // Add occasional bright streaks
  if (random8() < (sp / 10 + 5)) {
    uint8_t streak_pos = random8(NUM_LEDS - 10);
    for (int i = 0; i < 8; i++) {
      if (streak_pos + i < NUM_LEDS) {
        leds[streak_pos + i] += CRGB(random8(50), random8(100, 255), random8(100, 200));
      }
    }
  }
}

void styleMatrixCode(uint8_t sp) {
  // Fade background
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(230);
  }
  
  // Add falling code streams
  static uint8_t streams[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate > (200 - sp * 2)) {
    for (int s = 0; s < 10; s++) {
      if (streams[s] == 255) {
        if (random8() < 50) {
          streams[s] = 0; // Start new stream
        }
      } else {
        uint8_t pos = streams[s] * NUM_LEDS / 255;
        if (pos < NUM_LEDS) {
          leds[pos] = CHSV(96, 255, 255); // Bright green
          if (pos > 0) leds[pos-1] = CHSV(96, 255, 150);
          if (pos > 1) leds[pos-2] = CHSV(96, 255, 80);
        }
        
        streams[s] += 20;
        if (streams[s] > 255 + 50) streams[s] = 255; // Reset stream
      }
    }
    lastUpdate = millis();
  }
}

void styleCrystalCave(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t noise1 = inoise16(i * 60, millis() / (40 - sp / 8));
    uint16_t noise2 = inoise16(i * 80 + 5000, millis() / (60 - sp / 6));
    
    uint8_t brightness = (noise1 + noise2) / 512;
    uint8_t hue = 160 + (noise1 / 1000); // Blue to cyan range
    
    leds[i] = CHSV(hue, 200, brightness);
  }
  
  // Add sparkle effect
  if (random8() < (sp / 8 + 10)) {
    leds[random8(NUM_LEDS)] += CRGB(100, 100, 255);
  }
}

void styleLavaFlow(uint8_t sp) {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t heat = inoise8(i * 40, millis() / (80 - sp));
    
    // Create lava colors (black -> red -> orange -> yellow -> white)
    CRGB color;
    if (heat < 128) {
      color = CRGB(heat * 2, 0, 0); // Black to red
    } else {
      uint8_t excess = heat - 128;
      color = CRGB(255, excess * 2, excess / 4); // Red to orange to yellow
    }
    
    leds[i] = color;
  }
}

void styleWaveform(uint8_t sp) {
  static uint8_t phase = 0;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t wave1 = sin8(i * 8 + phase);
    uint8_t wave2 = sin8(i * 12 + phase * 2);
    uint8_t wave3 = sin8(i * 16 + phase * 3);
    
    uint8_t combined = (wave1 + wave2 + wave3) / 3;
    uint8_t hue = i * 255 / NUM_LEDS + phase;
    
    leds[i] = CHSV(hue, 255, combined);
  }
  
  phase += sp / 8;
}

void runTimed(void (*fn)()){
  static uint32_t lastT=0;
  uint32_t now=millis(), d=getTi()*15000;
  if(d==0||now-lastT>=d){ 
    styleIdx=(styleIdx+1)%42; 
    lastT=now; 
  }
  fn();
}

// ── Crossfade System Implementation ───────────────────────────────────────────
void executePattern(uint8_t patternIndex, CRGB* buffer) {
  // First clear the buffer
  memset(buffer, 0, sizeof(CRGB) * NUM_LEDS);
  
  // Store original styleIdx
  uint8_t originalStyleIdx = styleIdx;
  styleIdx = patternIndex;
  
  // Execute the specific pattern directly into the buffer
  switch(patternIndex) {
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
    // NEW PATTERNS (20 additional patterns)
    case 22: styleTwinkleStars(getSpeed()); break;
    case 23: styleRainbowRipples(getSpeed()); break;
    case 24: styleDNAHelix(getSpeed()); break;
    case 25: styleNeonPulse(getSpeed()); break;
    case 26: styleDigitalRain(getSpeed()); break;
    case 27: stylePlasmaBalls(getSpeed()); break;
    case 28: styleLightningStorm(getSpeed()); break;
    case 29: styleKaleidoscope(getSpeed()); break;
    case 30: styleCandleFlicker(getSpeed()); break;
    case 31: styleColorDrips(getSpeed()); break;
    case 32: styleGalaxySpiral(getSpeed()); break;
    case 33: stylePrism(getSpeed()); break;
    case 34: styleHeartbeat(getSpeed()); break;
    case 35: styleAuroraBoreal(getSpeed()); break;
    case 36: styleMatrixCode(getSpeed()); break;
    case 37: styleCrystalCave(getSpeed()); break;
    case 38: styleLavaFlow(getSpeed()); break;
    case 39: styleWaveform(getSpeed()); break;
    case 40: styleRainbow(getSpeed()); break; // Safe duplicate of pattern 0
    case 41: styleConfetti(getSpeed()); break; // Safe duplicate of pattern 4
  }
  
  // Copy the results from leds to our buffer
  memcpy(buffer, leds, sizeof(CRGB) * NUM_LEDS);
  
  // Restore original styleIdx
  styleIdx = originalStyleIdx;
}

void runTimedWithCrossfade(void (*fn)()){
  static uint32_t lastPatternChange = 0;
  static uint32_t crossfadeStartTime = 0;
  static uint8_t currentPattern = 0;
  static uint8_t nextPattern = 1;
  static bool inCrossfade = false;
  static CRGB buffer1[NUM_LEDS];
  static CRGB buffer2[NUM_LEDS];
  static bool firstRun = true;
  
  uint32_t now = millis();
  uint32_t patternDuration = getTi() * 15000; // 15 second patterns
  const uint32_t CROSSFADE_DURATION = 5000;   // 5 second crossfade
  
  // Initialize on first run
  if(firstRun) {
    lastPatternChange = now;
    currentPattern = styleIdx;
    nextPattern = (currentPattern + 1) % 42;
    firstRun = false;
  }
  
  // Check if it's time to start a crossfade
  if(!inCrossfade && (patternDuration == 0 || now - lastPatternChange >= (patternDuration - CROSSFADE_DURATION))) {
    inCrossfade = true;
    crossfadeStartTime = now;
    nextPattern = (currentPattern + 1) % 42;
  }
  
  if(inCrossfade) {
    uint32_t crossfadeElapsed = now - crossfadeStartTime;
    
    if(crossfadeElapsed >= CROSSFADE_DURATION) {
      // Crossfade complete - switch to next pattern
      inCrossfade = false;
      currentPattern = nextPattern;
      styleIdx = currentPattern;
      lastPatternChange = now;
      
      // Run the new current pattern normally
      fn();
    } else {
      // We're in crossfade - blend both patterns
      float crossfadeProgress = (float)crossfadeElapsed / CROSSFADE_DURATION;
      
      // Generate current pattern in buffer1
      executePattern(currentPattern, buffer1);
      
      // Generate next pattern in buffer2  
      executePattern(nextPattern, buffer2);
      
      // Blend the two patterns into the main leds array
      for(int i = 0; i < NUM_LEDS; i++) {
        // Smooth crossfade using ease-in-out curve
        float blend = crossfadeProgress * crossfadeProgress * (3.0f - 2.0f * crossfadeProgress);
        
        leds[i].r = (uint8_t)(buffer1[i].r * (1.0f - blend) + buffer2[i].r * blend);
        leds[i].g = (uint8_t)(buffer1[i].g * (1.0f - blend) + buffer2[i].g * blend);
        leds[i].b = (uint8_t)(buffer1[i].b * (1.0f - blend) + buffer2[i].b * blend);
      }
      
      // Apply music effect if this is the music function
      if(fn == effectMusic) {
        // Apply enhanced dramatic music scaling to the blended result
        float enhancedMusicLevel = musicLevel * musicLevel; // Square for more contrast
        const float MIN_BRIGHTNESS = 0.03f;  // 3% minimum  
        const float MAX_BRIGHTNESS = 1.0f;   // 100% maximum
        float finalLevel = MIN_BRIGHTNESS + (enhancedMusicLevel * (MAX_BRIGHTNESS - MIN_BRIGHTNESS));
        
        uint8_t musicScale = uint8_t(finalLevel * 255);
        for(int i = 0; i < NUM_LEDS; i++) {
          leds[i].nscale8(musicScale);
        }
      }
    }
  } else {
    // Not in crossfade - run pattern normally
    styleIdx = currentPattern;
    fn();
  }
}