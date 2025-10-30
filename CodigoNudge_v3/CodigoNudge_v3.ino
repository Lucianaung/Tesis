#include <FastLED.h>

#define TIRA_PIN 18
#define BOTON_PIN 21
#define INDICADOR_PIN 19
#define POT_PIN 34

#define NUM_LEDS 200
#define COLOR_ORDEN BRG
#define LED_TYPE WS2811

#define NUM_INDICADOR 1

#define MAX_LEDS 200
#define SIZE_VIAJERO 6
#define MAX_VIAJEROS 32

CRGB leds[NUM_LEDS];
CRGB indicador[NUM_INDICADOR];

struct Viajero {
  int pos;
  CRGB color;
  bool activo;
};

Viajero viajeros[MAX_VIAJEROS];

CRGB colorSeleccionado;  // color definido por potenciómetro

bool botonPrevio = HIGH;
bool monitorActivo = false;
int segundosTranscurridos = 0;
int tiempoMonitoreo = 0;
int contadorColores[3] = { 0, 0, 0 };
int nextViajero = 0;  // global, apunta al viajero a sobrescribir
unsigned long ultimaActividad = 0;
int ultimoValorPot = 0;
bool ultimoEstadoBoton = HIGH;
bool controlInactivo = false;


const CHSV coloresPredefinidos[3] = {
  CHSV(227, 255, 245),  // MAGENTA
  CHSV(158, 255, 222),  // AZUL
  CHSV(13, 250, 255)    // NARANJA
};

void setup() {
  Serial.begin(115200);  // Inicializa el monitor serial a 115200 baudios
  FastLED.addLeds<LED_TYPE, TIRA_PIN, COLOR_ORDEN>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  FastLED.addLeds<LED_TYPE, INDICADOR_PIN, COLOR_ORDEN>(indicador, NUM_INDICADOR);
  indicador[0] = CRGB::Black;
  FastLED.show();

  pinMode(BOTON_PIN, INPUT_PULLUP);

  // Inicializar viajeros
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    viajeros[i].activo = false;
  }
}

void loop() {
  // Leer potenciómetro siempre, para futuros viajeros
  static int valorPotFiltrado = 0;
  int valorPot = analogRead(POT_PIN);
  valorPotFiltrado = (valorPotFiltrado * 0.9) + (valorPot * 0.1);
  int rangoColor = map(valorPot, 0, 4095, 0, 2);
  colorSeleccionado = coloresPredefinidos[rangoColor];

  // LED indicador siempre refleja la selección
  indicador[0] = colorSeleccionado;

  bool botonActual = digitalRead(BOTON_PIN);

  // Detectar actividad en potenciómetro o botón
  if (abs(valorPot - ultimoValorPot) > 200 || botonActual != ultimoEstadoBoton) {
    ultimaActividad = millis();  //hubo actividad se empieza a contar
    controlInactivo = false;
    ultimoValorPot = valorPotFiltrado;
    ultimoEstadoBoton = botonActual;
  }

  // Verificar inactividad despúes de 5 segundos
  if (millis() - ultimaActividad > 5000) {
    controlInactivo = true;
  }

  // Detectar actividad en boton y enviar viajeros
  if (botonPrevio == HIGH && botonActual == LOW) {
    bool enviado = false;

    // Buscar un viajero libre
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (!viajeros[i].activo) {
        viajeros[i].pos = 0;
        viajeros[i].color = colorSeleccionado;
        viajeros[i].activo = true;
        enviado = true;
        break;
      }
    }

    // Si no hay viajeros libres, sobrescribir el siguiente
    if (!enviado) {
      viajeros[nextViajero].pos = 0;
      viajeros[nextViajero].color = colorSeleccionado;
      viajeros[nextViajero].activo = true;

      nextViajero++;
      if (nextViajero >= MAX_VIAJEROS) nextViajero = 0;
    }
  }
  botonPrevio = botonActual;

  if (controlInactivo) {
    Serial.println("SISTEMA INACTIVO");
  } else {
    Serial.println("ACTIVIDAD DETECTADA");
  }
  Serial.println(valorPotFiltrado);

  // --- Animación de viajeros ---
  EVERY_N_MILLISECONDS(150) {
    // Limpiar solo la tira, no el indicador
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;

    // Actualizar cada viajero activo
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        viajeros[i].pos = (viajeros[i].pos + 1) % (MAX_LEDS + 1);

        for (int j = 0; j < SIZE_VIAJERO; j++) {
          int ledIndex = (viajeros[i].pos + j) % (MAX_LEDS + 1);
          leds[ledIndex] = viajeros[i].color;  // sobrescribe colores anteriores
        }
      }
    }
    // actualizarMonitoreo();
    FastLED.show();
  }
  FastLED.show();  // Mostrar indicador siempre
}


//-----------------------MONITOREOS INTELIGENTES-----------------------------------------
