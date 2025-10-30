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
CRGB colorFondo = CRGB::Black;

struct Viajero {
  int pos;
  // CRGB color;
  int colorIndex;
  bool activo;
};

Viajero viajeros[MAX_VIAJEROS];

// CRGB colorSeleccionado;  // color definido por potenciómetro
int colorSeleccionadoIndex = 0;  // índice del color seleccionado
bool botonPrevio = HIGH;
unsigned long ultimaActividad = 0;
int ultimoValorPot = 0;
bool ultimoEstadoBoton = HIGH;
bool controlInactivo = false;
int nextViajero = 0;
bool fondoDefinido = false;


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

  CHSV(13, 76, 255),   // naranjablanco 9
  CHSV(158, 30, 222),  // azulblanco 10
  CHSV(227, 60, 255),  // rosablanco 11
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
  enum EstadoControl { ACTIVO,
                       INACTIVO };
  static EstadoControl estadoActual = ACTIVO;

  // Leer potenciómetro
  static int valorPotFiltrado = 0;
  int valorPot = analogRead(POT_PIN);
  valorPotFiltrado = (valorPotFiltrado * 0.9) + (valorPot * 0.1);

  int rangoColor;
  if (valorPotFiltrado < 1365) {
    rangoColor = 0;
  } else if (valorPotFiltrado < 2730) {
    rangoColor = 1;
  } else {
    rangoColor = 2;
  }

  colorSeleccionadoIndex = rangoColor;
  indicador[0] = coloresPredefinidos[colorSeleccionadoIndex];

  // Detectar actividad en potenciómetro o botón
  bool botonActual = digitalRead(BOTON_PIN);
  if (abs(valorPotFiltrado - ultimoValorPot) > 40 || botonActual != ultimoEstadoBoton) {
    ultimaActividad = millis();  //hubo actividad se empieza a contar
    controlInactivo = false;
    ultimoValorPot = valorPotFiltrado;
    ultimoEstadoBoton = botonActual;
  }

  // Verificar inactividad despúes de 5 segundos
  if (millis() - ultimaActividad > 5000) {
    controlInactivo = true;
  }

  // Enviar viajero si presiono el boton
  if (botonPrevio == HIGH && botonActual == LOW) {
    bool enviado = false;

    // Buscar un viajero libre
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (!viajeros[i].activo) {
        viajeros[i].pos = 0;
        viajeros[i].colorIndex = colorSeleccionadoIndex;
        viajeros[i].activo = true;
        enviado = true;
        break;
      }
    }

    // Si no hay viajeros libres, sobrescribir el siguiente
    if (!enviado) {
      viajeros[nextViajero].pos = 0;
      viajeros[nextViajero].colorIndex = colorSeleccionadoIndex;
      viajeros[nextViajero].activo = true;
      nextViajero++;
      if (nextViajero >= MAX_VIAJEROS) nextViajero = 0;
    }
  }
  botonPrevio = botonActual;

  // Modo LienzoBlanco
  if (controlInactivo) {
    if (estadoActual != INACTIVO) {
      Serial.println("SISTEMA INACTIVO");
      lienzoBlanco();
      estadoActual = INACTIVO;
    }
  } else {
    if (estadoActual != ACTIVO) {
      Serial.println("ACTIVIDAD DETECTADA");
      estadoActual = ACTIVO;
    }
  }
  Serial.println(valorPotFiltrado);

  // --- Animación de viajeros ---
  EVERY_N_MILLISECONDS(150) {
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = colorFondo;
    for (int i = 0; i < MAX_VIAJEROS; i++) {
      if (viajeros[i].activo) {
        viajeros[i].pos = (viajeros[i].pos + 1) % (MAX_LEDS + 1);
        for (int j = 0; j < SIZE_VIAJERO; j++) {
          int ledIndex = (viajeros[i].pos + j) % (MAX_LEDS + 1);
          leds[ledIndex] = coloresPredefinidos[viajeros[i].colorIndex];  // sobrescribe colores anteriores
        }
      }
    }
  }
  FastLED.show();  // Mostrar indicador siempre
}


//-----------------------MONITOREOS INTELIGENTES-----------------------------------------

void lienzoBlanco() {
  int contador[3] = { 0, 0, 0 };

  //Contar colores de los viajeros activos
  for (int i = 0; i < MAX_VIAJEROS; i++) {
    if (viajeros[i].activo) {
      int index = viajeros[i].colorIndex;
      if (index >= 0 && index <= 2) {
        contador[index]++;
      }
    }
  }

  // Calcular total y existencia de viajeros
  int total = contador[0] + contador[1] + contador[2];
  if (total == 0) {
    Serial.println("No hay viajeros activos");
    return;
  }

  //Determinar color predominante
  int predominante = 0;
  for (int i = 1; i < 3; i++) {
    if (contador[i] > contador[predominante]) {
      predominante = i;
    }
  }

  // Mostrar resultados
  Serial.println("-----Lienzo Blanco-----");
  Serial.print("MAGENTA: ");
  Serial.print((contador[0] * 100) / total);
  Serial.println("%");
  Serial.print("AZUL: ");
  Serial.print((contador[1] * 100) / total);
  Serial.println("%");
  Serial.print("NARANJA: ");
  Serial.print((contador[2] * 100) / total);
  Serial.println("%");
  Serial.print("Color predominante: ");
  switch (predominante) {
    case 0: Serial.println("MAGENTA"); break;
    case 1: Serial.println("AZUL"); break;
    case 2: Serial.println("NARANJA"); break;
  }
  Serial.println("-----------------------");

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

  for (int i = 0; i < MAX_VIAJEROS; i++) {
    viajeros[i].activo = false;
  }
  nextViajero = 0;

  for (int i = 0; i < NUM_LEDS; i++) { //pintar la tira del color de fondo predominante
    leds[i] = colorFondo;
  }

  FastLED.show();
}