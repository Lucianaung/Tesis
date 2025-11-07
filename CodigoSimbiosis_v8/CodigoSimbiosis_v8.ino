
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

int colorSeleccionadoIndex = 0;  // índice del color seleccionado
bool botonPrevio = HIGH;
unsigned long ultimaActividad = 0;
int ultimoValorPot = 0;
int nextViajero = 0;
bool fondoDefinido = false;

bool respiracionActiva = false;

unsigned long tiempoInactividadRandom = 0;
int minRandom = 5000;
int maxRandom = 10000;

unsigned long ultimoDeteccionColor = 0;
bool esperandoAsistentes = false;
unsigned long proximoAsistente = 0;
int asistentesPendientes = 0;
uint32_t colorDominanteActual;

const unsigned long intervaloAsistente = 10000;  // 10 s
int randomMinAsistentes = 1;
int randomMaxAsistentes = 4;
int proximoAsistenteMINMs = 2000;
int proximoAsistenteMAXMs = 5000;

// Variables globales del efecto breathing
float pulseSpeed = 0.5;                            // velocidad general
float valueMin = 20.0;                             // mínimo brillo
float valueMax = 255.0;                            // máximo brillo
float delta = (valueMax - valueMin) / 2.35040238;  // constante del algoritmo
int indiceFondoGlobal = 9;

uint8_t hueA, hueB;  // matices (color base)
uint8_t satA, satB;  // saturaciones (intensidad del color)
int coloresGrupoAsistente[2];

CHSV coloresPredefinidos[12] = {
  CHSV(13, 250, 150),   // naranja 0
  CHSV(158, 255, 150),  // azul 1
  CHSV(227, 255, 150),  // rosa 2

  CHSV(5, 250, 150),  // naranjarojo 3
  CHSV(5, 160, 150),  // naranja pastel 4 -- 120

  CHSV(139, 255, 150),  // celeste 5
  CHSV(144, 143, 150),  // celeste pastel 6

  CHSV(215, 255, 150),  // rosavioleta 7 206, 255, 150
  CHSV(206, 130, 150),  // rosapastel 8

  CHSV(13, 100, 50),   // naranjablanco 9 /100
  CHSV(158, 30, 50),   // azulblanco 10 /30
  CHSV(227, 120, 50),  // rosablanco 11 /120
};

void lienzoBlanco();
void enviarViajero();

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

  for (int i = 0; i < MAX_ASISTENTES; i++) {
    asistentes[i].activo = false;
  }

  randomSeed(analogRead(0));
  ultimaActividad = millis();
  tiempoInactividadRandom = random(minRandom, maxRandom);
}

// -------------------------------------
// LIENZO BLANCO
//--------------------------------------

void lienzoBlanco() {
  int contador[3] = { 0, 0, 0 };

  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (viajeros[i].activo) {
      int index = viajeros[i].colorIndex;
      if (index >= 0 && index <= 8) {
        if (index == 0 || index == 3 || index == 4) contador[0]++;
        else if (index == 1 || index == 5 || index == 6) contador[1]++;
        else if (index == 2 || index == 7 || index == 8) contador[2]++;
      }
    }
  }

  int total = contador[0] + contador[1] + contador[2];
  if (total == 0) {
    Serial.println("No hay viajeros activos para lienzo blanco");
    return;
  }

  int predominante = 0;
  for (int i = 1; i < 3; i++) {
    if (contador[i] > contador[predominante]) predominante = i;
  }

  int indiceFondo;
  if (predominante == 0) indiceFondo = 9;
  else if (predominante == 1) indiceFondo = 10;
  else indiceFondo = 11;

  indiceFondoGlobal = indiceFondo;
  colorFondo = coloresPredefinidos[indiceFondo];
  fondoDefinido = true;

  // Limpiar viajeros y asistentes
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    viajeros[i].activo = false;
    asistentes[i].activo = false;
  }
  nextViajero = 0;

  // Pintar fondo base
  fill_solid(leds, NUM_LEDS, colorFondo);
  FastLED.show();

  Serial.println("-----Lienzo Blanco-----");
  Serial.print("NARANJA: ");
  Serial.print((contador[0] * 100) / total);
  Serial.println("%");
  Serial.print("AZUL: ");
  Serial.print((contador[1] * 100) / total);
  Serial.println("%");
  Serial.print("ROSA: ");
  Serial.print((contador[2] * 100) / total);
  Serial.println("%");
}

// --------------------------------------------
// ANIMACIÓN DEL LIENZO BLANCO
// ---------------------------------------------
void animarLienzoBlanco() {
  static float fase = 0.0;
  float velocidadFase;  // controla la velocidad del pulso

  // // brillo oscilante entre min y max
  // float factor = (sin(fase) + 1.0) * 0.5;
  // uint8_t brilloActual = minBrillo + factor * (maxBrillo - minBrillo);
  // uint8_t saturacion = colorFondo.s - factor * 30;   // opcional, varía la saturación
  // uint8_t hue = colorFondo.h + sin(fase * 0.3) * 3;  // pequeña variación tonal


  // CHSV fondoBase = rgb2hsv_approximate(colorFondo);  // usa tu color de fondo global
  // CHSV fondoAnimado = CHSV(fondoBase.h, saturacion, brilloActual);


  // fill_solid(leds, NUM_LEDS, fondoAnimado);

  uint8_t minBrilloLocal, maxBrilloLocal;
  uint8_t satBase, satOscilacion;

  switch (indiceFondoGlobal) {
    case 9: // NARANJA BLANCO
      velocidadFase = 0.05;        // oscilación un poco más rápida
      minBrilloLocal = 30;
      maxBrilloLocal = 50;
      satBase = 100;
      satOscilacion = 80;          // saturación respira suavemente
      break;

    case 10: // AZUL BLANCO
      velocidadFase = 0.03;        // más lento
      minBrilloLocal = 40;
      maxBrilloLocal = 50;
      satBase = 30;
      satOscilacion = 10;
      break;

    case 11: // ROSA BLANCO
      velocidadFase = 0.03;        // intermedio
      minBrilloLocal = 30;
      maxBrilloLocal = 50;
      satBase = 120;
      satOscilacion = 30;
      break;

    default: // fallback, por si algo falla
      velocidadFase = 0.05;
      minBrilloLocal = 30;
      maxBrilloLocal = 50;
      satBase = 100;
      satOscilacion = 15;
      break;
  }

  // Animación
  fase += velocidadFase;
  float factor = (sin(fase) + 1.0) * 0.5;

  uint8_t brilloActual = minBrilloLocal + factor * (maxBrilloLocal - minBrilloLocal);
  uint8_t saturacionActual = satBase - factor * satOscilacion;

  CHSV fondoHSV = rgb2hsv_approximate(colorFondo);
  fondoHSV.v = brilloActual;
  fondoHSV.s = saturacionActual;

  fill_solid(leds, NUM_LEDS, fondoHSV);
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

  // Inactividad → activar lienzo blanco
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

  // Botón presionado
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

  // ANIMACIÓN o VIAJEROS
  if (respiracionActiva) {
    animarLienzoBlanco();
  } else {
    EVERY_N_MILLISECONDS(100) {
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

//-----------------------FUNCION ENVIAR VIAJERO-----------------------------------------

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

//-----------------------FUNCION ASISTENTE-----------------------------------------

void asistente() {
  int contador[3] = { 0, 0, 0 };

  // Contador de los viajeros activos según su color
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (viajeros[i].activo) {
      int index = viajeros[i].colorIndex;
      if (index >= 0 && index <= 8) {
        if (index == 0) contador[0]++;       // naranja
        else if (index == 1) contador[1]++;  // azul
        else if (index == 2) contador[2]++;  // rosa
      }
    }
  }

  int total = contador[0] + contador[1] + contador[2];
  if (total == 0) return;

  float porceNaranja = (contador[0] * 100.0) / total;
  float porceAzul = (contador[1] * 100.0) / total;
  float porceRosa = (contador[2] * 100.0) / total;

  //Detectar dominio
  if (porceNaranja > 85) {
    Serial.print("⚠️ Dominio naranja detectado: ");
    Serial.print(porceNaranja);
    Serial.println("%");
  } else if (porceAzul > 85) {
    Serial.print("⚠️ Dominio azul detectado: ");
    Serial.print(porceAzul);
    Serial.println("%");
  } else if (porceRosa > 85) {
    Serial.print("⚠️ Dominio rosa detectado: ");
    Serial.print(porceRosa);
    Serial.println("%");
  }
}

//-----------------------FUNCION VIAJEROS ASISTENTE-----------------------------------------

void viajerosAsistentes() {
  unsigned long ahora = millis();

  // --- Paso 1: detección de dominio cada 10 segundos ---
  static int predominante = -1;  // guardamos el último dominio detectado

  if (ahora - ultimoDeteccionColor >= intervaloAsistente && !esperandoAsistentes) {
    ultimoDeteccionColor = ahora;
    esperandoAsistentes = true;

    int contador[3] = { 0, 0, 0 };
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        int index = viajeros[i].colorIndex;
        if (index == 0 || index == 3 || index == 4) contador[0]++;       // naranjas
        else if (index == 1 || index == 5 || index == 6) contador[1]++;  // azules
        else if (index == 2 || index == 7 || index == 8) contador[2]++;  // rosas
      }
    }

    int total = contador[0] + contador[1] + contador[2];
    if (total == 0) {
      esperandoAsistentes = false;
      return;
    }

    predominante = 0;
    for (int i = 1; i < 3; i++) {
      if (contador[i] > contador[predominante]) predominante = i;
    }

    Serial.print("🌐 Dominio detectado: ");
    if (predominante == 0) Serial.println("NARANJA");
    else if (predominante == 1) Serial.println("AZUL");
    else Serial.println("ROSA");

    // Definir grupo de colores según dominio
    if (predominante == 0) {
      coloresGrupoAsistente[0] = 3;
      coloresGrupoAsistente[1] = 4;
    } else if (predominante == 1) {
      coloresGrupoAsistente[0] = 5;
      coloresGrupoAsistente[1] = 6;
    } else {
      coloresGrupoAsistente[0] = 7;
      coloresGrupoAsistente[1] = 8;
    }

    asistentesPendientes = random(randomMinAsistentes, randomMaxAsistentes);
    proximoAsistente = ahora + random(proximoAsistenteMINMs, proximoAsistenteMAXMs);
  }

  // --- Paso 2: lanzar asistentes escalonados ---
  if (esperandoAsistentes && ahora >= proximoAsistente && asistentesPendientes > 0) {
    for (int i = 0; i < MAX_ASISTENTES; i++) {
      if (!asistentes[i].activo) {
        asistentes[i].pos = 0;
        asistentes[i].colorIndex = coloresGrupoAsistente[random(0, 2)];  // random entre los dos colores del grupo
        asistentes[i].activo = true;
        Serial.print("💫 Asistente lanzado con color ");
        Serial.println(asistentes[i].colorIndex);
        break;
      }
    }

    asistentesPendientes--;
    if (asistentesPendientes > 0) {
      proximoAsistente = ahora + random(proximoAsistenteMINMs, proximoAsistenteMAXMs);
    } else {
      esperandoAsistentes = false;
    }
  }
}

// ---------------------------------------------------------
void aplicarEfectoBreathing(CRGB colorBase) {
  // Convertimos color base a HSV para obtener su matiz y saturación
  CHSV hsvBase = rgb2hsv_approximate(colorBase);

  hueA = hsvBase.hue;
  hueB = hsvBase.hue;  // mantenemos color estable, pero podrías variar un poco si querés
  satA = hsvBase.sat;
  satB = hsvBase.sat;

  // cálculo de brillo “orgánico”
  float dV = ((exp(sin(pulseSpeed * millis() / 2000.0 * PI)) - 0.36787944) * delta);
  float val = valueMin + dV;

  uint8_t hue = map(val, valueMin, valueMax, hueA, hueB);
  uint8_t sat = map(val, valueMin, valueMax, satA, satB);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);
  }
}