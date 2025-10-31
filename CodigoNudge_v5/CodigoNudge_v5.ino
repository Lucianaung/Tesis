//NO FUNCIONA BIEN EL VOLVER DE LA ANIMACIÓN DE RESPIRACIÓN

#include <FastLED.h>

#define TIRA_PIN 18
#define BOTON_PIN 21
#define INDICADOR_PIN 19
#define POT_PIN 34

#define NUM_LEDS 398
#define COLOR_ORDEN BRG
#define LED_TYPE WS2811

#define NUM_INDICADOR 1

#define MAX_LEDS 398
#define SIZE_VIAJERO 8
#define MAX_VIAJEROS 200

CRGB leds[NUM_LEDS];
CRGB indicador[NUM_INDICADOR];
CRGB colorFondo = CRGB::Black;

struct Viajero {
  int pos;
  // CRGB color;
  int colorIndex;
  bool activo;
};

Viajero viajeros[MAX_VIAJEROS];

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

CHSV coloresPredefinidos[12] = {
  CHSV(13, 250, 255),   // naranja 0
  CHSV(158, 255, 222),  // azul 1
  CHSV(227, 255, 245),  // rosa 2

  CHSV(5, 250, 255),  // naranjarojo 3
  CHSV(5, 153, 255),  // naranja pastel 4

  CHSV(139, 255, 224),  // celeste 5
  CHSV(144, 143, 224),  // celeste pastel 6

  CHSV(206, 255, 245),  // rosavioleta 7
  CHSV(206, 127, 245),  // rosapastel 8

  CHSV(13, 150, maxBrillo),   // naranjablanco 9
  CHSV(158, 30, maxBrillo),   // azulblanco 10
  CHSV(227, 150, maxBrillo),  // rosablanco 11
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

  randomSeed(analogRead(0));
  ultimaActividad = millis();
  tiempoInactividadRandom = random(minRandom, maxRandom);
}

void loop() {
  // Leer potenciómetro
  static int valorPotFiltrado = 0;
  int valorPot = analogRead(POT_PIN);
  valorPotFiltrado = (valorPotFiltrado * 0.9) + (valorPot * 0.1);

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
  if (abs(valorPotFiltrado - ultimoValorPot) > 40) {
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
  EVERY_N_MILLISECONDS(150) {
    if (!respiracionActiva) {
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = colorFondo;
      for (int i = 0; i < MAX_VIAJEROS; i++) {
        if (viajeros[i].activo) {
          viajeros[i].pos = (viajeros[i].pos + 1) % (NUM_LEDS + 1);
          for (int j = 0; j < SIZE_VIAJERO; j++) {
            int ledIndex = (viajeros[i].pos + j) % (NUM_LEDS + 1);
            leds[ledIndex] = coloresPredefinidos[viajeros[i].colorIndex];
          }
        }
      }
    }
  }

  FastLED.show();
  Serial.println(valorPotFiltrado);
}


//-----------------------FUNCIONES AUXILIARES-----------------------------------------

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
}