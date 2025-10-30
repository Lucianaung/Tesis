#include <FastLED.h>

// --- Pines y configuración ---
#define PIN_STRIP     18     // WS2811 12V (dos tiras en paralelo)
#define PIN_INDICADOR 19     // LED indicador WS2812 5V
#define PIN_POT        34    // Potenciómetro
#define PIN_BUTTON     21    // Pulsador

#define NUM_STRIP_LEDS 600
#define NUM_INDICADOR  1
#define BRIGHTNESS     200
#define CANTIDAD_VIAJEROS 6

CRGB strip[NUM_STRIP_LEDS];
CRGB indicador[NUM_INDICADOR];

// --- Variables ---
int lastButtonState = HIGH;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50; // ms
int lastRange = -1; // Para actualizar indicador solo si cambia

// Colores definidos en HSV
const CHSV colores[3] = {
  CHSV(321, 191, 245),  // Magenta
  CHSV(158, 245, 222),  // Azul eléctrico
  CHSV(18, 250, 255)    // Naranja
};

void setup() {
  Serial.begin(115200);

  // Inicializar LEDs con color order correcto
  FastLED.addLeds<WS2811, PIN_STRIP, BRG>(strip, NUM_STRIP_LEDS);
  FastLED.addLeds<WS2812, PIN_INDICADOR, GRB>(indicador, NUM_INDICADOR);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  analogReadResolution(12); // 0–4095
  Serial.println("Sistema listo (optimizado, no bloqueante).");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Actualización del indicador según potenciómetro ---
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    int potValue = analogRead(PIN_POT);
    int range = map(potValue, 0, 4095, 0, 2);

    // Actualizar indicador solo si cambió el color
    if (range != lastRange) {
      CRGB tempColor = colores[range];
      // Ajuste azul si es necesario
      tempColor.b = qadd8(tempColor.b, 30); 
      indicador[0] = tempColor;
      FastLED.show(); // Solo el indicador se actualiza
      lastRange = range;
    }

    // --- Leer botón ---
    int buttonState = digitalRead(PIN_BUTTON);
    if (buttonState == LOW && lastButtonState == HIGH) {
      // Pintar primeros CANTIDAD_VIAJEROS LEDs de la tira WS2811
      for (int i = 0; i < CANTIDAD_VIAJEROS; i++) {
        strip[i] = colores[range];
      }
      FastLED.show(); // Solo se envía la tira al presionar el botón
      Serial.print("Color enviado: ");
      Serial.println(range);
    }
    lastButtonState = buttonState;
  }

}
