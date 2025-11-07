#include <FastLED.h>

#define TIRA_PIN 18
#define BOTON_PIN 21
#define INDICADOR_PIN 19
#define POT_PIN 34

#define NUM_LEDS 497
#define COLOR_ORDEN BRG
#define LED_TYPE WS2811

#define NUM_INDICADOR 1

#define MAX_LEDS 497
#define SIZE_VIAJERO 8
#define MAX_VIAJEROS 200

#define MAX_ASISTENTES 200
#define SIZE_ASISTENTE 2
#define SIZE_VIAJERO_ASISTENTE 6
#define INTERVALO_ASISTENTE 5000

CRGB leds[NUM_LEDS];
CRGB indicador[NUM_INDICADOR];
CRGB colorFondo = CRGB::Black;

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

uint8_t minBrillo = 5;
uint8_t maxBrillo = 50;

bool respiracionActiva = false;

unsigned long tiempoInactividadRandom = 0;
int minRandom = 5000;
int maxRandom = 10000;

unsigned long ultimoDeteccionColor = 0;
bool esperandoAsistentes = false;
unsigned long proximoAsistente = 0;
int asistentesPendientes = 0;
uint32_t colorDominanteActual;

const unsigned long intervaloAsistente = 10000;  
int randomMinAsistentes = 1;
int randomMaxAsistentes = 4;
int proximoAsistenteMINMs = 2000;
int proximoAsistenteMAXMs = 5000;

int coloresGrupoAsistente[2] = { 0, 0 };

// ----- Variables para efecto breathing -----
static float pulseSpeed = 0.5;  
float valueMin = 20.0;          
float valueMax = 200.0;         
static float delta = (valueMax - valueMin) / 2.35040238;
uint8_t colorDominante = 0; // 0=naranja, 1=azul, 2=rosa

CHSV coloresPredefinidos[12] = {
  CHSV(13, 250, 150),   // 0 naranja
  CHSV(158, 255, 150),  // 1 azul
  CHSV(227, 255, 150),  // 2 rosa
  CHSV(5, 250, 150),    // 3 naranjarojo
  CHSV(5, 160, 150),    // 4 naranja pastel
  CHSV(139, 255, 150),  // 5 celeste
  CHSV(144, 143, 150),  // 6 celeste pastel
  CHSV(206, 255, 150),  // 7 rosavioleta
  CHSV(206, 130, 150),  // 8 rosapastel
  CHSV(13, 100, 255),   // 9 naranjablanco
  CHSV(158, 30, 255),   // 10 azulblanco
  CHSV(227, 50, 255),   // 11 rosablanco
};

void lienzoBlanco();
void enviarViajero();
void efectoBreathingLienzoBlanco();
void viajerosAsistentes();
void asistente();

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, TIRA_PIN, COLOR_ORDEN>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, INDICADOR_PIN, COLOR_ORDEN>(indicador, NUM_INDICADOR);
  FastLED.clear();
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
  if (abs(valorPotFiltrado - ultimoValorPot) > 15) {
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);
  }
  ultimoValorPot = valorPotFiltrado;

  if (!respiracionActiva && millis() - ultimaActividad > tiempoInactividadRandom) {
    Serial.println("SISTEMA INACTIVO - Activando lienzo blanco");
    lienzoBlanco();
    respiracionActiva = true;
  }

  if (botonPrevio == HIGH && botonActual == LOW) {
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);

    if (respiracionActiva) {
      respiracionActiva = false;
      fill_solid(leds, NUM_LEDS, colorFondo);
      enviarViajero();
    } else {
      enviarViajero();
    }
  }
  botonPrevio = botonActual;

  // --- Respiración orgánica de Marc Miller ---
  EVERY_N_MILLISECONDS(40) {
    if (respiracionActiva) efectoBreathingLienzoBlanco();
  }

  // --- Movimiento de viajeros / asistentes ---
  EVERY_N_MILLISECONDS(100) {
    if (!respiracionActiva) {
      fill_solid(leds, NUM_LEDS, colorFondo);

      for (int i = 0; i < MAX_VIAJEROS; i++) {
        if (viajeros[i].activo) {
          viajeros[i].pos = (viajeros[i].pos + 1.2);
          if (viajeros[i].pos >= NUM_LEDS) viajeros[i].activo = false;
          else {
            for (int j = 0; j < SIZE_VIAJERO; j++) {
              int ledIndex = ((int)viajeros[i].pos + j) % NUM_LEDS;
              leds[ledIndex] = coloresPredefinidos[viajeros[i].colorIndex];
            }
          }
        }
      }

      for (int i = 0; i < MAX_ASISTENTES; i++) {
        if (asistentes[i].activo) {
          asistentes[i].pos = fmod(asistentes[i].pos + 0.5, NUM_LEDS);
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

//-----------------------------------------------
//  EFECTO BREATHING CON COLOR DINÁMICO
//-----------------------------------------------
void efectoBreathingLienzoBlanco() {
  // Elegir color base según dominante
  uint8_t colorBaseIndex = 9;
  if (colorDominante == 1) colorBaseIndex = 10;
  else if (colorDominante == 2) colorBaseIndex = 11;

  // Cálculo de la curva breathing (Mac Miller)
  float dV = ((exp(sin(pulseSpeed * millis() / 2000.0 * PI)) - 0.36787944) * delta);
  float val = valueMin + dV;
  uint8_t brillo = constrain(val, valueMin, valueMax);

  CHSV base = coloresPredefinidos[colorBaseIndex];
  CHSV colorBreathing = CHSV(base.hue, base.sat, brillo);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = colorBreathing;
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);
  }

  FastLED.show();
}

//-----------------------------------------------
void lienzoBlanco() {
  int contador[3] = {0, 0, 0};
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (viajeros[i].activo) {
      int index = viajeros[i].colorIndex;
      if (index == 0 || index == 3 || index == 4) contador[0]++;
      else if (index == 1 || index == 5 || index == 6) contador[1]++;
      else if (index == 2 || index == 7 || index == 8) contador[2]++;
    }
  }

  int total = contador[0] + contador[1] + contador[2];
  if (total == 0) {
    Serial.println("No hay viajeros activos para lienzo blanco");
    return;
  }

  colorDominante = 0;
  for (int i = 1; i < 3; i++) {
    if (contador[i] > contador[colorDominante]) colorDominante = i;
  }

  int indiceFondo = (colorDominante == 0) ? 9 : (colorDominante == 1) ? 10 : 11;
  colorFondo = coloresPredefinidos[indiceFondo];
  fondoDefinido = true;

  for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
  nextViajero = 0;

  fill_solid(leds, NUM_LEDS, colorFondo);

  Serial.println("-----Lienzo Blanco-----");
  Serial.print("Dominante: "); Serial.println(colorDominante == 0 ? "Naranja" : colorDominante == 1 ? "Azul" : "Rosa");
}

void enviarViajero() {
  bool enviado = false;
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (!viajeros[i].activo) {
      viajeros[i].pos = 0;
      viajeros[i].colorIndex = colorSeleccionadoIndex;
      viajeros[i].activo = true;
      enviado = true;
      break;
    }
  }
  if (!enviado) {
    viajeros[nextViajero].pos = 0;
    viajeros[nextViajero].colorIndex = colorSeleccionadoIndex;
    viajeros[nextViajero].activo = true;
    nextViajero = (nextViajero + 1) % MAX_VIAJEROS;
  }
  asistente();
}

//-----------------------------------------------
void asistente() {
  int contador[3] = {0, 0, 0};
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (viajeros[i].activo) {
      int index = viajeros[i].colorIndex;
      if (index == 0) contador[0]++;
      else if (index == 1) contador[1]++;
      else if (index == 2) contador[2]++;
    }
  }

  int total = contador[0] + contador[1] + contador[2];
  if (total == 0) return;

  float porceNaranja = (contador[0] * 100.0) / total;
  float porceAzul = (contador[1] * 100.0) / total;
  float porceRosa = (contador[2] * 100.0) / total;

  if (porceNaranja > 85) Serial.println("⚠️ Dominio naranja detectado");
  else if (porceAzul > 85) Serial.println("⚠️ Dominio azul detectado");
  else if (porceRosa > 85) Serial.println("⚠️ Dominio rosa detectado");
}

//-----------------------------------------------
void viajerosAsistentes() {
  unsigned long ahora = millis();
  static int predominante = -1;

  if (ahora - ultimoDeteccionColor >= intervaloAsistente && !esperandoAsistentes) {
    ultimoDeteccionColor = ahora;
    esperandoAsistentes = true;

    int contador[3] = {0, 0, 0};
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        int index = viajeros[i].colorIndex;
        if (index == 0 || index == 3 || index == 4) contador[0]++;
        else if (index == 1 || index == 5 || index == 6) contador[1]++;
        else if (index == 2 || index == 7 || index == 8) contador[2]++;
      }
    }

    predominante = 0;
    for (int i = 1; i < 3; i++) if (contador[i] > contador[predominante]) predominante = i;

    Serial.print("🌐 Dominio detectado: ");
    if (predominante == 0) Serial.println("NARANJA");
    else if (predominante == 1) Serial.println("AZUL");
    else Serial.println("ROSA");

    if (predominante == 0) { coloresGrupoAsistente[0] = 3; coloresGrupoAsistente[1] = 4; }
    else if (predominante == 1) { coloresGrupoAsistente[0] = 5; coloresGrupoAsistente[1] = 6; }
    else { coloresGrupoAsistente[0] = 7; coloresGrupoAsistente[1] = 8; }

    asistentesPendientes = random(randomMinAsistentes, randomMaxAsistentes);
    proximoAsistente = ahora + random(proximoAsistenteMINMs, proximoAsistenteMAXMs);
  }

  if (esperandoAsistentes && ahora >= proximoAsistente && asistentesPendientes > 0) {
    for (int i = 0; i < MAX_ASISTENTES; i++) {
      if (!asistentes[i].activo) {
        asistentes[i].pos = 0;
        asistentes[i].colorIndex = coloresGrupoAsistente[random(0, 2)];
        asistentes[i].activo = true;
        Serial.print("💫 Asistente lanzado con color ");
        Serial.println(asistentes[i].colorIndex);
        break;
      }
    }
    asistentesPendientes--;
    if (asistentesPendientes > 0)
      proximoAsistente = ahora + random(proximoAsistenteMINMs, proximoAsistenteMAXMs);
    else
      esperandoAsistentes = false;
  }
}
