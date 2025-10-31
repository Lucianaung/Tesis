#include <FastLED.h>

// Definición de tiras
#define NUM_LEDS_1 398
#define NUM_LEDS_2 99
#define BOTON_PIN 21
#define POT_PIN 34

#define DATA_PIN_1 18
#define DATA_PIN_2 19

#define LED_TYPE WS2811
#define COLOR_ORDER BRG  // Asegurate que el orden coincide con tu tira
#define BRILLO 200

CRGB leds1[NUM_LEDS_1];
CRGB leds2[NUM_LEDS_2];

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE, DATA_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);
  FastLED.setBrightness(BRILLO);

  // Encender todo en blanco
  fill_solid(leds1, NUM_LEDS_1, CRGB::White);
  fill_solid(leds2, NUM_LEDS_2, CRGB::White);


  pinMode(BOTON_PIN, INPUT_PULLUP);
  FastLED.show();
}

void loop() {
  // No hace falta nada más si solo querés encenderlas en blanco fijo
}
