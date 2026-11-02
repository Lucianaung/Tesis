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

#define MAX_ASISTENTES 100
#define SIZE_ASISTENTE 6

// Destello al nacer un viajero
#define DESTELLO_MS 250
#define DESTELLO_R 110
#define DESTELLO_G 60
#define DESTELLO_B 60    // menos azul para compensar la tira
#define DESTELLO_LEDS 3   // cuántos LEDs del viajero llevan el destello (12 = todo el cuerpo)

// Tiempo (en ms) durante el cual un viajero recién nacido tiene prioridad sobre los asistentes.
// Los asistentes van a la mitad de velocidad, así que en ~1 s el viajero ya los dejó atrás.
// Si cambiás la velocidad (EVERY_N_MILLISECONDS), ajustá este valor en proporción.
#define PRIORIDAD_NACIMIENTO_MS 1200

// Antirrebote del botón (en ms)
#define DEBOUNCE_MS 40

CRGB leds[NUM_LEDS];
CRGB indicador[NUM_INDICADOR];
CHSV colorFondo = CHSV(0, 0, 0);

struct Viajero {
  int pos;
  int colorIndex;
  bool activo;
  unsigned long nacio;  // momento de nacimiento (destello y prioridad sobre asistentes)
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
unsigned long ultimoBoton = 0;
unsigned long ultimaActividad = 0;
int ultimoValorPot = 0;
int nextViajero = 0;    // buffer circular de viajeros: siguiente slot a escribir (= el más viejo cuando está lleno)
int nextAsistente = 0;  // buffer circular de asistentes: igual que el de viajeros
bool fondoDefinido = false;
bool respiracionActiva = false;

// VARIABLES LIENZO BLANCO
unsigned long tiempoInactividadRandom = 0;
int minRandom = 10000;   // 10 segundos
int maxRandom = 35000;   // 35 segundos

bool lienzoInicializado = false;
unsigned long inicioTransicion = 0;
bool transicionLienzoActiva = false;
float brilloLienzo = 0.0;

// TRANSICIÓN AL LIENZO BLANCO (2 fases)
//   Fase 1: lo que se ve se apaga hasta negro.
//   Fase 2: el efecto del lienzo ya está corriendo y va subiendo de brillo desde negro.
const unsigned long duracionApagado = 1000;    // ms de la fase 1 (apagado) - ajustable
const unsigned long duracionEncendido = 1500;  // ms de la fase 2 (encendido con el efecto) - ajustable (mínimo 1)
int faseTransicion = 0;                        // 0 = ninguna, 1 = apagado, 2 = encendido

// VARIABLES VIAJEROS ASISTENTES
unsigned long ultimoDeteccionColor = 0;
bool esperandoAsistentes = false;
unsigned long proximoAsistente = 0;
int asistentesPendientes = 0;

const unsigned long intervaloAsistente = 20000;  // 20s
int randomMinAsistentes = 1;
int randomMaxAsistentes = 6;
int proximoAsistenteMINMs = 1000;
int proximoAsistenteMAXMs = 6000;

int indiceFondoGlobal;
int coloresGrupoAsistente[2];


float brilloMax = 255;
float brilloMed = 200;
float brilloMin = 180;

CHSV coloresPredefinidos[12] = {
  CHSV(7, 255, brilloMax),    // naranja 0
  CHSV(160, 255, brilloMax),  // azul 1
  CHSV(243, 255, brilloMax),  // rosa 2
  CHSV(255, 255, brilloMed),  // naranjarojo 3
  CHSV(7, 220, brilloMax),    // naranjarojo pastel 4
  CHSV(140, 255, brilloMed),  // celeste agua 5
  CHSV(100, 120, brilloMax),  // agua mar 6
  CHSV(230, 255, brilloMed),  // rosavioleta 7
  CHSV(224, 180, brilloMax),  // violeta pastel 8
  CHSV(8, 140, brilloMin),    // naranjablanco 9
  CHSV(160, 30, brilloMin),   // azulblanco 10
  CHSV(243, 120, brilloMin)   // rosablanco 11
};

void lienzoBlanco();
void animarLienzoBlanco(uint8_t escala = 255);
void enviarViajero();
void asistente();
void viajerosAsistentes();
void reiniciarAsistentes();
void dibujarEscena(bool avanzar);
void dibujarViajero(int i, unsigned long ahora);

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

  // --- BOTÓN: cada pulsación hace nacer un viajero nuevo en el LED 0, por encima de todo ---
  if (botonPrevio == HIGH && botonActual == LOW && millis() - ultimoBoton > DEBOUNCE_MS) {
    ultimoBoton = millis();
    ultimaActividad = millis();
    tiempoInactividadRandom = random(minRandom, maxRandom);

    if (respiracionActiva) {
      respiracionActiva = false;
      fondoDefinido = false;
      transicionLienzoActiva = false;  // si estaba en plena transición, la cancelo
      lienzoInicializado = false;
      faseTransicion = 0;
      reiniciarAsistentes();           // limpia asistentes y tandas pendientes: nada del color anterior se filtra
      FastLED.setBrightness(180);
    }

    enviarViajero();
    dibujarEscena(false);  // se ve al instante, sin esperar el próximo paso de 45 ms
    FastLED.show();
  }
  botonPrevio = botonActual;

  if (respiracionActiva) {
    if (transicionLienzoActiva) {
      lienzoBlanco();  // avanza la transición
    } else {
      animarLienzoBlanco();
    }
  } else {
    EVERY_N_MILLISECONDS(45) {
      dibujarEscena(true);
    }
  }

  FastLED.show();

  // Mientras dura el lienzo blanco no se lanzan asistentes
  if (!respiracionActiva) viajerosAsistentes();
}


//-----------------------DIBUJO DE LA ESCENA (viajeros + asistentes)-----------------------------------------
// Orden de dibujo (de abajo hacia arriba):
//   1) fondo
//   2) viajeros, del más viejo al más nuevo
//   3) asistentes, del más viejo al más nuevo  -> se ven por encima de los viajeros durante su recorrido
//   4) viajeros recién nacidos (menos de PRIORIDAD_NACIMIENTO_MS) -> si nacen justo donde pasa un asistente,
//      el viajero manda; y el recién nacido queda siempre por encima de todo.

void dibujarViajero(int i, unsigned long ahora) {
  CHSV c = coloresPredefinidos[viajeros[i].colorIndex];
  bool destello = (DESTELLO_MS > 0 && (ahora - viajeros[i].nacio) < DESTELLO_MS);

  for (int j = 0; j < SIZE_VIAJERO; j++) {
    int ledIndex = ((int)viajeros[i].pos + j) % NUM_LEDS;
    if (destello && j < DESTELLO_LEDS) {
      leds[ledIndex] = CRGB(DESTELLO_R, DESTELLO_G, DESTELLO_B);
    } else {
      leds[ledIndex] = c;
    }
  }
}

void dibujarEscena(bool avanzar) {
  unsigned long ahora = millis();
  fill_solid(leds, NUM_LEDS, colorFondo);

  // 2) Viajeros (de más viejo a más nuevo: empezamos en nextViajero)
  for (int k = 0; k < MAX_VIAJEROS; k++) {
    int i = (nextViajero + k) % MAX_VIAJEROS;
    if (!viajeros[i].activo) continue;
    if (avanzar) viajeros[i].pos = (viajeros[i].pos + 1) % NUM_LEDS;
    dibujarViajero(i, ahora);
  }

  // 3) Asistentes por encima (de más viejo a más nuevo: empezamos en nextAsistente)
  for (int k = 0; k < MAX_ASISTENTES; k++) {
    int i = (nextAsistente + k) % MAX_ASISTENTES;
    if (!asistentes[i].activo) continue;
    if (avanzar) asistentes[i].pos = fmod(asistentes[i].pos + 0.5, NUM_LEDS);  //0.4
    for (int j = 0; j < SIZE_ASISTENTE; j++) {
      int ledIndex = ((int)asistentes[i].pos + j) % NUM_LEDS;
      leds[ledIndex] = coloresPredefinidos[asistentes[i].colorIndex];
    }
  }

  // 4) Viajeros recién nacidos: se vuelven a dibujar encima de los asistentes
  for (int k = 0; k < MAX_VIAJEROS; k++) {
    int i = (nextViajero + k) % MAX_VIAJEROS;
    if (!viajeros[i].activo) continue;
    if ((ahora - viajeros[i].nacio) < PRIORIDAD_NACIMIENTO_MS) {
      dibujarViajero(i, ahora);
    }
  }
}


//-----------------------LIENZO BLANCO (TRANSICIÓN)-----------------------------------------
// Fase 1: la escena que se ve en el momento de la inactividad se apaga hasta negro.
// Fase 2: arranca el efecto del lienzo (ya corriendo) y sube de brillo desde negro hasta el valor normal.

void lienzoBlanco() {
  static int contador[3];
  static int indiceFondo;
  static CRGB previo[NUM_LEDS];  // "foto" de la escena justo antes de apagarse

  unsigned long ahora = millis();

  if (!lienzoInicializado) {
    // reset contadores
    contador[0] = contador[1] = contador[2] = 0;

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
      // no iniciar transición; salimos y dejemos a quien llamó decidir
      return;
    }

    int predominante = 0;
    for (int i = 1; i < 3; i++) {
      if (contador[i] > contador[predominante]) predominante = i;
    }

    if (predominante == 0) indiceFondo = 9;
    else if (predominante == 1) indiceFondo = 10;
    else indiceFondo = 11;

    indiceFondoGlobal = indiceFondo;
    colorFondo = coloresPredefinidos[indiceFondo];
    fondoDefinido = true;

    // guardo lo que se está viendo ahora para apagarlo suavemente
    memcpy(previo, leds, sizeof(previo));

    // apago viajeros (lógica) y reinicio su puntero
    for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
    nextViajero = 0;

    // apago asistentes y cancelo cualquier tanda pendiente
    reiniciarAsistentes();

    // iniciar transición: fase 1 (apagado)
    transicionLienzoActiva = true;
    faseTransicion = 1;
    inicioTransicion = ahora;
    lienzoInicializado = true;

    Serial.println("-----Lienzo Blanco (inicio transición)-----");
    Serial.print("NARANJA: "); Serial.print((contador[0] * 100) / total); Serial.println("%");
    Serial.print("AZUL: ");    Serial.print((contador[1] * 100) / total); Serial.println("%");
    Serial.print("ROSA: ");    Serial.print((contador[2] * 100) / total); Serial.println("%");
    return;
  }

  if (transicionLienzoActiva) {

    // ---------- FASE 1: APAGADO ----------
    if (faseTransicion == 1) {
      float progreso = constrain((float)(ahora - inicioTransicion) / (float)duracionApagado, 0.0, 1.0);
      float curva = 0.5 - 0.5 * cos(progreso * PI);
      uint8_t escala = (uint8_t)(255 * (1.0 - curva));  // 255 -> 0

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = previo[i];
        leds[i].nscale8(escala);
      }

      if (progreso >= 1.0f) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        faseTransicion = 2;
        inicioTransicion = ahora;
        Serial.println("Apagado completo - inicia el lienzo con el efecto.");
      }
      return;
    }

    // ---------- FASE 2: ENCENDIDO CON EL EFECTO YA CORRIENDO ----------
    if (faseTransicion == 2) {
      float progreso = constrain((float)(ahora - inicioTransicion) / (float)max(duracionEncendido, 1UL), 0.0, 1.0);
      float curva = 0.5 - 0.5 * cos(progreso * PI);
      uint8_t escala = (uint8_t)(255 * curva);  // 0 -> 255

      animarLienzoBlanco(escala);  // el efecto corre desde el primer instante, escalado en brillo

      if (progreso >= 1.0f) {
        transicionLienzoActiva = false;
        lienzoInicializado = false;
        faseTransicion = 0;
        Serial.println("Transición de lienzo blanco completada.");
      }
      return;
    }
  }
}

//-----------------------ANIMACIÓN DE LIENZO BLANCO-----------------------------------------
// escala: 255 = brillo normal del efecto; valores menores lo atenúan (se usa durante la transición).

void animarLienzoBlanco(uint8_t escala) {
  static bool inicializado = false;
  static float fases[NUM_LEDS];
  static float velocidades[NUM_LEDS];
  static unsigned long ultimoUpdate = 0;
  const unsigned long intervalo = 16;  // ~60 FPS sin delay

  if (!inicializado) {
    for (int i = 0; i < NUM_LEDS; i++) {
      fases[i] = random(0, 628) / 100.0;           // entre 0 y ~2π
      velocidades[i] = 0.03 + random(30) / 200.0;  // variación sutil puede ir entre 0.03 y 0.13
    }
    inicializado = true;
  }

  unsigned long ahora = millis();
  if (ahora - ultimoUpdate < intervalo) return;
  ultimoUpdate = ahora;

  CHSV fondoHSV = coloresPredefinidos[indiceFondoGlobal];

  uint8_t brilloBase = fondoHSV.v;
  uint8_t amplitud = brilloBase * 0.4; //0.3=30%
  uint8_t satBase = fondoHSV.s;
  uint8_t hue = fondoHSV.h;

  // --- Actualización de cada LED ---
  for (int i = 0; i < NUM_LEDS; i++) {
    fases[i] += velocidades[i];
    if (fases[i] > 6.2831) fases[i] -= 6.2831;

    float onda = (sin(fases[i]) + 1.0) * 0.5;  // rango 0–1
    uint8_t brillo = brilloBase + (onda - 0.5) * amplitud;
    brillo = scale8(brillo, escala);           // atenúa durante la transición (255 = sin cambio)

    leds[i] = CHSV(hue, satBase, brillo);
  }
}


//-----------------------FUNCION ENVIAR VIAJERO-----------------------------------------
// Buffer circular: siempre se escribe en nextViajero. Si está lleno, reemplaza al más viejo.
// Así el orden de dibujo (desde nextViajero) siempre va de más viejo a más nuevo.

void enviarViajero() {
  viajeros[nextViajero].pos = 0;
  viajeros[nextViajero].colorIndex = colorSeleccionadoIndex;
  viajeros[nextViajero].activo = true;
  viajeros[nextViajero].nacio = millis();
  nextViajero = (nextViajero + 1) % MAX_VIAJEROS;

  asistente();
}

//-----------------------FUNCION ASISTENTE-----------------------------------------
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

//-----------------------FUNCION VIAJEROS ASISTENTE-----------------------------------------

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

    int total = contador[0] + contador[1] + contador[2];
    if (total == 0) {
      esperandoAsistentes = false;
      return;
    }

    predominante = 0;
    for (int i = 1; i < 3; i++) if (contador[i] > contador[predominante]) predominante = i;

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

  if (esperandoAsistentes && ahora >= proximoAsistente && asistentesPendientes > 0) {
    // Buffer circular: si están todos los lugares ocupados, el nuevo reemplaza al más viejo
    asistentes[nextAsistente].pos = 0;
    asistentes[nextAsistente].colorIndex = coloresGrupoAsistente[random(0, 2)];
    asistentes[nextAsistente].activo = true;
    Serial.print("Asistente lanzado con color ");
    Serial.println(asistentes[nextAsistente].colorIndex);
    nextAsistente = (nextAsistente + 1) % MAX_ASISTENTES;

    asistentesPendientes--;
    if (asistentesPendientes > 0)
      proximoAsistente = ahora + random(proximoAsistenteMINMs, proximoAsistenteMAXMs);
    else
      esperandoAsistentes = false;
  }
}

//--------------------------- REINICIO DE ASISTENTES (al entrar y al salir del lienzo blanco)--------------
// Apaga todos los asistentes, reinicia su puntero y cancela cualquier tanda a medio lanzar.
void reiniciarAsistentes() {
  for (int i = 0; i < MAX_ASISTENTES; i++) asistentes[i].activo = false;
  nextAsistente = 0;
  esperandoAsistentes = false;
  asistentesPendientes = 0;
  ultimoDeteccionColor = millis();
  proximoAsistente = 0;
}
