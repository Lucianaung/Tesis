// corrección de asistentes. comportamiento. no aparecen

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
  // CRGB color;
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

const unsigned long intervaloAsistente = 10000;  // 10 s
int randomMinAsistentes = 1;
int randomMaxAsistentes = 4;
int proximoAsistenteMINMs = 2000;
int proximoAsistenteMAXMs = 5000;

int coloresGrupoAsistente[2] = { 0, 0 };

static float pulseSpeed = 0.6;  // velocidad del pulso
float valueMin = 30.0;          // brillo mínimo
float valueMax = 255.0;         // brillo máximo
static float delta = (valueMax - valueMin) / 2.35040238;

CHSV coloresPredefinidos[12] = {
  CHSV(13, 250, 150),   // naranja 0
  CHSV(158, 255, 150),  // azul 1
  CHSV(227, 255, 150),  // rosa 2

  CHSV(5, 250, 150),  // naranjarojo 3
  CHSV(5, 160, 150),  // naranja pastel 4 -- 120

  CHSV(139, 255, 150),  // celeste 5
  CHSV(144, 143, 150),  // celeste pastel 6

  CHSV(206, 255, 150),  // rosavioleta 7
  CHSV(206, 130, 150),  // rosapastel 8

  CHSV(13, 100, maxBrillo),  // naranjablanco 9
  CHSV(158, 30, maxBrillo),  // azulblanco 10
  CHSV(227, 50, maxBrillo),  // rosablanco 11
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

void loop() {
  // Leer potenciómetro
  static int valorPotFiltrado = 0;
  int valorPot = analogRead(POT_PIN);
  valorPotFiltrado = (valorPotFiltrado * 0.8) + (valorPot * 0.2);

  if (valorPotFiltrado < 1362) {
    colorSeleccionadoIndex = 0;
  } else if (valorPotFiltrado < 2725) {
    colorSeleccionadoIndex = 1;
  } else {
    colorSeleccionadoIndex = 2;
  }
  indicador[0] = coloresPredefinidos[colorSeleccionadoIndex];

  // Detectar actividad en potenciómetro o botón
  bool botonActual = digitalRead(BOTON_PIN);
  if (abs(valorPotFiltrado - ultimoValorPot) > 15) {
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);
  }
  ultimoValorPot = valorPotFiltrado;

  // Inactividad: activar lienzo blanco y respiración
  if (!respiracionActiva && millis() - ultimaActividad > tiempoInactividadRandom) {
    Serial.println("SISTEMA INACTIVO - Activando lienzo blanco");
    lienzoBlanco();
    respiracionActiva = true;
  }

  // Boton
  if (botonPrevio == HIGH && botonActual == LOW) {
    ultimaActividad = millis();  // el botón también cuenta como actividad
    tiempoInactividadRandom = random(minRandom, maxRandom);

    if (respiracionActiva) {
      respiracionActiva = false;
      fill_solid(leds, NUM_LEDS, colorFondo);
      enviarViajero();
      FastLED.show();
    } else {
      enviarViajero();
    }
  }
  botonPrevio = botonActual;

  // --- Animación principal / respiración ---
  // --- Respiración ---
  EVERY_N_MILLISECONDS(50) {
      if (respiracionActiva) {
        unsigned long tAhora = millis();
        float periodo = 2800.0;
        float tRelativo = fmod(tAhora, periodo) / periodo;
        float valorSeno = sin(tRelativo * 2 * PI);  // -1 a 1

        float easing = (valorSeno + 1.0) / 2.0;

        easing = (pow(easing, 2.5) + pow(1 - easing, 2.5));

        uint8_t brillo = minBrillo + easing * (maxBrillo - minBrillo);  // 30..100 %
        for (int i = 0; i < NUM_LEDS; i++) {
          CRGB colorResp = colorFondo;             // base limpia
          colorResp.nscale8_video(brillo * 2.55);  // aplica brillo
          leds[i] = colorResp;
        }
      }
    }
    // --- Animación de viajeros ---
    EVERY_N_MILLISECONDS(100) {
      if (!respiracionActiva) {
        fill_solid(leds, NUM_LEDS, colorFondo);
        // for (int i = 0; i < NUM_LEDS; i++) leds[i] = colorFondo;

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
            asistentes[i].pos = fmod(asistentes[i].pos + 0.5, NUM_LEDS);  // VELOCIDAD más rápido
            for (int j = 0; j < SIZE_ASISTENTE; j++) {
              int ledIndex = ((int)asistentes[i].pos + j) % NUM_LEDS;

              // se sobreescribe directamente, igual que los viajeros
              leds[ledIndex] = coloresPredefinidos[asistentes[i].colorIndex];
            }
          }
        }
      }
    }
    FastLED.show();
    Serial.println(valorPotFiltrado);
    viajerosAsistentes();
  }


  //-----------------------FUNCION LIENZO BLANCO-----------------------------------------

  void lienzoBlanco() {
    int contador[3] = { 0, 0, 0 };

    //Contar colores de los viajeros activos
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        int index = viajeros[i].colorIndex;
        if (index >= 0 && index <= 8) {
          // clasificación de grupos
          if (index == 0 || index == 3 || index == 4) contador[0]++;
          else if (index == 1 || index == 5 || index == 6) contador[1]++;
          else if (index == 2 || index == 7 || index == 8) contador[2]++;
        }
      }
    }

    // Calcular total y existencia de viajeros
    int total = contador[0] + contador[1] + contador[2];
    if (total == 0) {
      Serial.println("No hay viajeros activos para lienzo blanco");
      return;
    }

    //Determinar color predominante
    int predominante = 0;
    for (int i = 1; i < 3; i++) {
      if (contador[i] > contador[predominante]) {
        predominante = i;
      }
    }

    int indiceFondo;
    if (predominante == 0) {
      indiceFondo = 9;
    } else if (predominante == 1) {
      indiceFondo = 10;
    } else if (predominante == 2) {
      indiceFondo = 11;
    }

    colorFondo = coloresPredefinidos[indiceFondo];
    fondoDefinido = true;

    //resetar borrar viajeros hasta el momento
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      viajeros[i].activo = false;
    }
    nextViajero = 0;

    //pintar la tira del color de fondo predominante
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = colorFondo;
    }

    // Mostrar resultados
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
  static int predominante = -1; // guardamos el último dominio detectado

  if (ahora - ultimoDeteccionColor >= intervaloAsistente && !esperandoAsistentes) {
    ultimoDeteccionColor = ahora;
    esperandoAsistentes = true;

    int contador[3] = {0, 0, 0};
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
        asistentes[i].colorIndex = coloresGrupoAsistente[random(0, 2)]; // random entre los dos colores del grupo
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

