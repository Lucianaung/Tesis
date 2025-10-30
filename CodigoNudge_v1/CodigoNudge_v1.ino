#include <FastLED.h>

// --- Pines y configuración ---
#define PIN_STRIP     18
#define PIN_INDICADOR 19
#define PIN_POT       34
#define PIN_BUTTON    21

#define NUM_STRIP_LEDS 600
#define NUM_INDICADOR  1
#define BRIGHTNESS     200
#define MAX_VIAJEROS   10   // número máximo de viajeros activos
#define VELOCIDAD      80   // ms entre frames
#define VIAJERO_LONG   10   // LEDs por viajero

CRGB strip[NUM_STRIP_LEDS];
CRGB indicador[NUM_INDICADOR];

// --- Variables ---
int lastButtonState = HIGH;
unsigned long lastMove = 0;

// --- Colores HSV calibrados ---
const CHSV coloresHSV[3] = {
  CHSV(224, 255, 255),  // Fucsia
  CHSV(160, 255, 255),  // Azul
  CHSV(15, 255, 200)    // Naranja
};

CHSV colorSeleccion = coloresHSV[0];

// --- Estructura de un viajero ---
struct Viajero {
  CRGB color;      // color fijo
  int posicion;    // posición actual del primer LED
};

Viajero viajeros[MAX_VIAJEROS];
int numViajeros = 0;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2811, PIN_STRIP, BRG>(strip, NUM_STRIP_LEDS);
  FastLED.addLeds<WS2811, PIN_INDICADOR, BRG>(indicador, NUM_INDICADOR);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  analogReadResolution(12);

  fill_solid(strip, NUM_STRIP_LEDS, CRGB::Black);
  indicador[0] = CHSVtoCRGB(colorSeleccion);

  Serial.println("Sistema listo: multi-viajeros independientes.");
}

// --- Función para convertir CHSV a CRGB ---
CRGB CHSVtoCRGB(CHSV c) {
  CRGB temp;
  hsv2rgb_rainbow(c, temp);
  return temp;
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Leer potenciómetro ---
  int potValue = analogRead(PIN_POT);
  int range = map(potValue, 0, 4095, 0, 2);
  colorSeleccion = coloresHSV[range];

  // --- Mantener LED indicador estable ---
  indicador[0] = CHSVtoCRGB(colorSeleccion);

  // --- Leer botón ---
  int buttonState = digitalRead(PIN_BUTTON);
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (numViajeros < MAX_VIAJEROS) {
      // Crear nuevo viajero
      viajeros[numViajeros].color = CHSVtoCRGB(colorSeleccion);
      viajeros[numViajeros].posicion = 0;
      numViajeros++;
      Serial.print("Viajero agregado, total: ");
      Serial.println(numViajeros);
    }
    delay(200); // anti-rebote
  }
  lastButtonState = buttonState;

  // --- Animación de todos los viajeros ---
  if (currentMillis - lastMove >= VELOCIDAD) {
    lastMove = currentMillis;

    // Limpiar toda la tira
    fill_solid(strip, NUM_STRIP_LEDS, CRGB::Black);

    for (int v = 0; v < numViajeros; v++) {
      for (int i = 0; i < VIAJERO_LONG; i++) {
        int idx = (viajeros[v].posicion + i) % NUM_STRIP_LEDS;
        strip[idx] = viajeros[v].color;
      }
      // Avanzar posición
      viajeros[v].posicion = (viajeros[v].posicion + 1) % NUM_STRIP_LEDS;
    }

    FastLED.show();
  }
}
