// Candle animation with neopixel ring and PIR sensor

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define FRAME_MS 33 // 30 fps

#define PIN_RGB    6
#define NUM_PIXELS 12

#define PIN_PIR    2

#define WAKE_DURATION_MS  180000 // 3 minutes
#define HYPER_DURATION_MS 5000

#define FLICKER_KHZ_IDLE (1.0 / 500.0) // once every 500 ms
#define FLICKER_KHZ_HYPER (1.0 / 250.0)

#define NUM_HEAT_LEVELS 8

#define COLOR_SLEEP 0x7f0000

const uint32_t gColorTableIdle[NUM_HEAT_LEVELS] = {
  0xffe000,
  0xff8000,
  0xff6000,
  0xff4000,
  0xff3000,
  0xff2000,
  0xff1000,
  0xff0000,
};

// Blue flame
const uint32_t gColorTableHyper[NUM_HEAT_LEVELS] = {
  0x05a1a7,
  0x35e6ea,
  0xebf4f4,
  0x8df6f7,
  0x00885e,
  0x005e88,
  0x001a88,
  0x2a0088,
};

enum : int {
  kStateSleep = 0,
  kStateIdle,
  kStateHyper
} gState = kStateIdle;

// Position and velocity of fireball
int gPos = 0;
int gVel = 0;

// Time in ms after boot for animation changes
uint32_t gSleepTime = 0;
uint32_t gHyperEndTime = 0;
uint32_t gNextTurnTime = 0;

// Table of how recent the fireball was in each position
int gHeatMap[NUM_PIXELS] = {};

Adafruit_NeoPixel gPixels(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

// Get the time interval for the next random event given an average rate
double get_poisson_interval(double rate) {
  return -log(1.0 - (double)random(100) / 100.0) / rate;
}

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
  const uint32_t t = val < min ? min : val;
  return t > max ? max : t;
}

uint32_t color_add_noise(uint32_t color)
{
  const double mag = 0x04;

  double r_d = random(-0x40, 0x40);
  double g_d = random(-0x40, 0x40);
  double b_d = random(-0x40, 0x40);

  double norm = sqrt(r_d * r_d + g_d * g_d + b_d * b_d);

  r_d *= mag / norm;
  g_d *= mag / norm;
  b_d *= mag / norm;

  uint32_t r = (color & 0xff0000) >> 16;
  uint32_t g = (color & 0xff00) >> 8;
  uint32_t b = color & 0xff;

  return Adafruit_NeoPixel::Color(
    clamp(r + r_d, 0, 0xff),
    clamp(g + g_d, 0, 0xff),
    clamp(b + b_d, 0, 0xff));
}

void set_spark() {
  gVel = 0;
  gNextTurnTime = 0;
  for (int i = 0; i < NUM_PIXELS; i++) {
    gHeatMap[i] = NUM_HEAT_LEVELS - 1;
  }
}

void set_sleep() {
  gPixels.clear();

  // Maintain min current to avoid power bank sleep
  for (int i = 0; i < NUM_PIXELS; i++)
    gPixels.setPixelColor(i, COLOR_SLEEP);
  gPixels.show();
}

void render_flame() {
  uint32_t * color_table = (gState == kStateHyper) ?
    gColorTableHyper : gColorTableIdle;

  for (int i = 0; i < NUM_PIXELS; i++) {
    gPixels.setPixelColor(i,
      color_add_noise(color_table[gHeatMap[i]]));
  }

  gPixels.show();
}

void check_presence() {
  int detected = digitalRead(PIN_PIR) == HIGH;

  if (detected) {
    gState = kStateHyper;

    uint32_t now = millis();
    gHyperEndTime = now + HYPER_DURATION_MS;
    gSleepTime = now + WAKE_DURATION_MS;

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
      gState == kStateHyper ? FLICKER_KHZ_HYPER : FLICKER_KHZ_IDLE);
  }

  if (gState == kStateHyper && time > gHyperEndTime) {
    gState = kStateIdle;
  }
  if (gState != kStateSleep && time > gSleepTime) {
    gState = kStateSleep;
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

  gSleepTime = WAKE_DURATION_MS;
  set_spark();
}

void loop() {
  uint32_t start = millis();

  check_presence();

  if (gState == kStateSleep) {
    set_sleep();
  }
  else {
    render_flame();
    update_fireball(start);
  }

  uint32_t d = millis() - start;
  if (d < FRAME_MS)
    delay(FRAME_MS - d); 
}