#include <FastLED.h>

#define LED_PIN 18
#define NUM_LEDS 60
#define COLOR_ORDER BRG
#define LED_TYPE WS2811

CRGB leds[NUM_LEDS];

// Paleta de 12 colores personalizados en formato HSV
CHSV color[12] = {
  CHSV(13, 250, 255),   // naranja
  CHSV(158, 255, 222),  // azul
  CHSV(227, 255, 245),  // rosa

  CHSV(5, 250, 255),  // naranjarojo
  CHSV(5, 153, 255),  // naranja pastel

  CHSV(139, 255, 224),  // celeste
  CHSV(144, 143, 224),  // celeste pastel

  CHSV(206, 255, 245),  // rosavioleta
  CHSV(206, 127, 245),  // rosapastel

  CHSV(13, 76, 255)    // naranjablanco
  CHSV(158, 30, 222),  // azulblanco
  CHSV(227, 60, 255),  // rosablanco
};

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  for (int i = 0; i < 12; i++) {
    leds[i] = color[i];  // asigna el color correspondiente
    FastLED.show();      // pequeño retardo para ver cada LED encenderse
  }
}
