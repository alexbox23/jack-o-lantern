// Candle animation with neopixel ring and PIR sensor

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN        6
#define NUM_PIXELS 12

#define FRAME_MS 33 // 30 fps

#define HYPER_DURATION_MS 5000

#define FLICKER_KHZ_IDLE (1.0 / 500.0) // once every 500 ms
#define FLICKER_KHZ_HYPER (1.0 / 250.0)

#define NUM_HEAT_LEVELS 8

// Blue flame
bool gHyper = false;

// Position and velocity of fireball
int gPos = 0;
int gVel = 0;

// Time in ms after boot for animation changes
uint32_t gNextTurnTime = 0;
uint32_t gHyperEndTime = 0;

// Table of how recent the fireball was in each position
int gHeatMap[NUM_PIXELS] = {};

uint32_t gColorTableIdle[NUM_HEAT_LEVELS] = {
  0xffe000,
  0xff8000,
  0xff6000,
  0xff4000,
  0xff3000,
  0xff2000,
  0xff1000,
  0xff0000,
};

uint32_t gColorTableHyper[NUM_HEAT_LEVELS] = {
  0x05a1a7,
  0x35e6ea,
  0xebf4f4,
  0x8df6f7,
  0x00885e,
  0x005e88,
  0x001a88,
  0x2a0088,
};

Adafruit_NeoPixel gPixels(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Get the time interval for the next random event given an average rate
double get_poisson_interval(double rate) {
  return -log(1.0 - (double)random(100) / 100.0) / rate;
}

void set_spark() {
    gVel = 0;
    gNextTurnTime = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
      gHeatMap[i] = NUM_HEAT_LEVELS - 1;
    }
}

void render_flame() {
  // TODO: add noise
  uint32_t * color_table = gHyper ? gColorTableHyper : gColorTableIdle;

  for (int i = 0; i < NUM_PIXELS; i++) {
    gPixels.setPixelColor(i, color_table[gHeatMap[i]]);
  }

  gPixels.show();
}

void check_hyper() {
  bool detected = false;

  if (detected && !gHyper) {
    gHyper = true;
    gHyperEndTime = millis() + HYPER_DURATION_MS;

    set_spark();
  }
}

void update_fireball(uint32_t time) {
  gPos = (gPos + gVel + NUM_PIXELS) % NUM_PIXELS;

  for (int i = 0; i < NUM_PIXELS; i++) {
    if (gHeatMap[i] <= 6 && gHeatMap[i] >= 4)
      if (gPos % 2 == 0)
        continue;
  
    if (gHeatMap[i] > 0)
      gHeatMap[i]--;
  }
  gHeatMap[gPos] = NUM_HEAT_LEVELS - 1;

  if (time > gNextTurnTime) {
    gVel = (gVel == 0) ? 1 : -gVel;
    gNextTurnTime = time + get_poisson_interval(
      gHyper ? FLICKER_KHZ_HYPER : FLICKER_KHZ_IDLE);
  }

  if (time > gHyperEndTime) {
    gHyper = false;
  }
}

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Jack-o-lantern boot");
#endif

  gPixels.begin();
  gPixels.clear();

  set_spark();
}

void loop() {
  uint32_t start = millis();

  check_hyper();

  render_flame();

  update_fireball(start);
  
  uint32_t d = millis() - start;
  if (d < FRAME_MS)
    delay(FRAME_MS - d); 
}