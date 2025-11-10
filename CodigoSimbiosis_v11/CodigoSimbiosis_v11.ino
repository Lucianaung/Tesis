#include <FastLED.h>

#define TIRA_PIN 18
#define BOTON_PIN 21
#define INDICADOR_PIN 19
#define POT_PIN 34

#define NUM_LEDS 497
#define COLOR_ORDEN BRG
#define LED_TYPE WS2811
#define NUM_INDICADOR 1

#define SIZE_VIAJERO 12
#define MAX_VIAJEROS 200

#define MAX_ASISTENTES 200
#define SIZE_ASISTENTE 6

CRGB leds[NUM_LEDS];
CRGB indicador[NUM_INDICADOR];
CHSV colorFondo = CHSV(0, 0, 0);

struct Viajero {
  int pos;
  int colorIndex;
  bool activo;
};

struct ViajeroAsistente {
  float pos;
  int colorIndex;
  bool activo;
};

Viajero viajeros[MAX_VIAJEROS];
ViajeroAsistente asistentes[MAX_ASISTENTES];

int colorSeleccionadoIndex = 0;
bool botonPrevio = HIGH;
unsigned long ultimaActividad = 0;
int ultimoValorPot = 0;
int nextViajero = 0;
bool fondoDefinido = false;
bool respiracionActiva = false;

// VARIABLES LIENZO BLANCO
unsigned long tiempoInactividadRandom = 0;
int minRandom = 180000;   //3 min 180000
int maxRandom = 300000;  //10 min 900000 5min 300000

bool lienzoInicializado = false;
unsigned long inicioTransicion = 0;
const unsigned long duracionTransicion = 3000;  // en milisegundos, ajustable
bool transicionLienzoActiva = false;
float brilloLienzo = 0.0;

// VARIABLES VIAJEROS ASISTENTES
unsigned long ultimoDeteccionColor = 0;
bool esperandoAsistentes = false;
unsigned long proximoAsistente = 0;
int asistentesPendientes = 0;

const unsigned long intervaloAsistente = 30000;  // 30s
int randomMinAsistentes = 1;
int randomMaxAsistentes = 6;
int proximoAsistenteMINMs = 1000;
int proximoAsistenteMAXMs = 8000;

int indiceFondoGlobal;
int coloresGrupoAsistente[2];


float brilloMax = 255;
float brilloMed = 200;
float brilloMin = 180;

CHSV coloresPredefinidos[12] = {
  CHSV(13, 255, brilloMax),   // naranja 0
  CHSV(160, 255, brilloMax),  // azul 1
  CHSV(224, 255, brilloMax),  // rosa 2
  CHSV(5, 250, brilloMed),    // naranjarojo 3
  CHSV(5, 160, brilloMax),    // naranja pastel 4
  CHSV(144, 255, brilloMed),  // celeste 5
  CHSV(144, 120, brilloMax),  // celeste pastel 6
  CHSV(215, 255, brilloMed),  // rosavioleta 7
  CHSV(205, 180, brilloMax),  // rosapastel 8
  CHSV(13, 140, brilloMin),   // naranjablanco 9
  CHSV(158, 60, brilloMin),   // azulblanco 10
  CHSV(230, 80, brilloMin)    // rosablanco 11
};

void lienzoBlanco();
void animarLienzoBlanco();
void enviarViajero();
void asistente();
void viajerosAsistentes();

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, TIRA_PIN, COLOR_ORDEN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.clear();
  FastLED.show();

  FastLED.addLeds<LED_TYPE, INDICADOR_PIN, COLOR_ORDEN>(indicador, NUM_INDICADOR);
  indicador[0] = CRGB::Black;
  FastLED.show();

  pinMode(BOTON_PIN, INPUT_PULLUP);

  for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
  for (int i = 0; i < MAX_ASISTENTES; i++) asistentes[i].activo = false;

  randomSeed(analogRead(0));
  ultimaActividad = millis();
  tiempoInactividadRandom = random(minRandom, maxRandom);
}

void loop() {
  static int valorPotFiltrado = 0;
  int valorPot = analogRead(POT_PIN);
  valorPotFiltrado = (valorPotFiltrado * 0.8) + (valorPot * 0.2);

  if (valorPotFiltrado < 1362) colorSeleccionadoIndex = 0;
  else if (valorPotFiltrado < 2725) colorSeleccionadoIndex = 1;
  else colorSeleccionadoIndex = 2;
  indicador[0] = coloresPredefinidos[colorSeleccionadoIndex];

  bool botonActual = digitalRead(BOTON_PIN);
  if (abs(valorPotFiltrado - ultimoValorPot) > 40) {
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);
  }
  ultimoValorPot = valorPotFiltrado;

  if (!respiracionActiva && millis() - ultimaActividad > tiempoInactividadRandom) {
    bool hayViajeros = false;
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        hayViajeros = true;
        break;
      }
    }

    if (hayViajeros) {
      Serial.println("SISTEMA INACTIVO - Activando lienzo blanco");
      lienzoBlanco();
      respiracionActiva = true;
    } else {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
  }

  if (botonPrevio == HIGH && botonActual == LOW) {
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);

    if (respiracionActiva) {
      respiracionActiva = false;
      fondoDefinido = false;
      FastLED.setBrightness(180);
      fill_solid(leds, NUM_LEDS, colorFondo);
      enviarViajero();
      FastLED.show();
    } else {
      enviarViajero();
    }
  }
  botonPrevio = botonActual;

  if (respiracionActiva) {
    if (transicionLienzoActiva) {
      lienzoBlanco();  // avanza la transición
    } else {
      animarLienzoBlanco(); 
    }
  } else {
    EVERY_N_MILLISECONDS(60) {
      fill_solid(leds, NUM_LEDS, colorFondo);
      for (int i = 0; i < MAX_VIAJEROS; i++) {
        if (viajeros[i].activo) {
          viajeros[i].pos = (viajeros[i].pos + 1) % (NUM_LEDS + 1);
          for (int j = 0; j < SIZE_VIAJERO; j++) {
            int ledIndex = (viajeros[i].pos + j) % (NUM_LEDS + 1);
            leds[ledIndex] = coloresPredefinidos[viajeros[i].colorIndex];
          }
        }
      }
      for (int i = 0; i < MAX_ASISTENTES; i++) {
        if (asistentes[i].activo) {
          asistentes[i].pos = fmod(asistentes[i].pos + 0.4, NUM_LEDS);
          for (int j = 0; j < SIZE_ASISTENTE; j++) {
            int ledIndex = ((int)asistentes[i].pos + j) % NUM_LEDS;
            leds[ledIndex] = coloresPredefinidos[asistentes[i].colorIndex];
          }
        }
      }
    }
  }

  FastLED.show();
  viajerosAsistentes();
}