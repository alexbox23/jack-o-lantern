// Neopixel ring and PIR sensor

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define FRAME_MS 33 // 30 fps

#define PIN_RGB    0
#define PIN_PIR    1

#define NUM_PIXELS 12
#define HUE_CYCLE_MS 0x1000 // Must be less than 0x10000

#define WAKE_DURATION_MS  300000 // 5 minutes

// Time in ms after boot
uint32_t gSleepTime = 0;

Adafruit_NeoPixel gPixels(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

void check_presence() {
  uint32_t now = millis();

  int detected = digitalRead(PIN_PIR) == HIGH;

  // Detected and asleep
  if (detected && now > gSleepTime) {
    gSleepTime = now + WAKE_DURATION_MS;
  }
}

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  gPixels.begin();
  gPixels.clear();

  gSleepTime = WAKE_DURATION_MS;
}

void loop() {
  uint32_t start = millis();

  check_presence();

  if (start < gSleepTime) { // Awake
    gPixels.rainbow((start % HUE_CYCLE_MS) * (0x10000 / HUE_CYCLE_MS));
  }
  else { // Asleep
    gPixels.clear();
  }

  gPixels.show();

  uint32_t d = millis() - start;
  if (d < FRAME_MS)
    delay(FRAME_MS - d);
}