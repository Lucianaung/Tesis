
//-----------------------LIENZO BLANCO-----------------------------------------

void lienzoBlanco() {
  static int contador[3];
  static int indiceFondo;
  static CHSV fondoHSV;

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

    // apago viajeros y asistentes
    for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
    for (int i = 0; i < MAX_ASISTENTES; i++) asistentes[i].activo = false;
    nextViajero = 0;

    // iniciar parámetros de transición
    transicionLienzoActiva = true;
    inicioTransicion = ahora;
    lienzoInicializado = true;

    Serial.println("-----Lienzo Blanco (inicio transición)-----");
    Serial.print("NARANJA: "); Serial.print((contador[0] * 100) / total); Serial.println("%");
    Serial.print("AZUL: ");    Serial.print((contador[1] * 100) / total); Serial.println("%");
    Serial.print("ROSA: ");    Serial.print((contador[2] * 100) / total); Serial.println("%");
    return;
  }

  if (transicionLienzoActiva) {
    float progreso = constrain((float)(ahora - inicioTransicion) / (float)duracionTransicion, 0.0, 1.0);
    float curva = 0.5 - 0.5 * cos(progreso * PI);

    fondoHSV = coloresPredefinidos[indiceFondoGlobal];

    uint8_t escalaPrevio = (uint8_t)(255 * (1.0 - curva)); // 255 -> 0
    uint8_t vNuevo = (uint8_t)(fondoHSV.v * curva);       // 0 -> vBase

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].nscale8(escalaPrevio);
    }

    CRGB tmp;
    hsv2rgb_rainbow(CHSV(fondoHSV.h, fondoHSV.s, vNuevo), tmp);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] += tmp;
    }

    if (progreso >= 1.0f) {
      fill_solid(leds, NUM_LEDS, fondoHSV);

      transicionLienzoActiva = false;
      lienzoInicializado = false; 
      Serial.println("Transición de lienzo blanco completada.");
    }

    return;
  }

  reiniciarAsistentes();
}

//-----------------------ANIMACIÓN DE LIENZO BLANCO-----------------------------------------

void animarLienzoBlanco() {
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

    leds[i] = CHSV(hue, satBase, brillo);
  }
}
