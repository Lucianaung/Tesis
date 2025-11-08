
//-----------------------LIENZO BLANCO-----------------------------------------

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

  for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
  for (int i = 0; i < MAX_ASISTENTES; i++) asistentes[i].activo = false;
  nextViajero = 0;

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

//-----------------------ANIMACIÓN DE LIENZO BLANCO-----------------------------------------

void animarLienzoBlanco() {
  static bool inicializado = false;
  static float fases[NUM_LEDS];
  static float velocidades[NUM_LEDS];
  static unsigned long ultimoUpdate = 0;
  const unsigned long intervalo = 16;  // ~60 FPS sin delay

  // Inicialización única (una sola vez)
  if (!inicializado) {
    for (int i = 0; i < NUM_LEDS; i++) {
      fases[i] = random(0, 628) / 100.0;           // entre 0 y ~2π
      velocidades[i] = 0.03 + random(20) / 200.0;  // variación sutil
    }
    inicializado = true;
  }

  unsigned long ahora = millis();
  if (ahora - ultimoUpdate < intervalo) return;
  ultimoUpdate = ahora;

  // --- Color base según el fondo dominante detectado ---
  CHSV fondoHSV = coloresPredefinidos[indiceFondoGlobal];
  // switch (indiceFondoGlobal) {
  //   case 9:   fondoHSV = CHSV(32, 60, 60);  break;   // Naranja-blanco
  //   case 10:  fondoHSV = CHSV(158, 30, 50); break;   // Azul-blanco
  //   case 11:  fondoHSV = CHSV(220, 50, 60); break;   // Rosa-blanco
  //   default:  fondoHSV = CHSV(0, 0, 60);    break;   // Blanco neutro
  // }

  // --- Parámetros del efecto ---
  uint8_t brilloBase = fondoHSV.v;
  uint8_t amplitud = brilloBase * 0.3;
  uint8_t satBase = fondoHSV.s;
  uint8_t hue = fondoHSV.h;

  // --- Actualización de cada LED ---
  for (int i = 0; i < NUM_LEDS; i++) {
    fases[i] += velocidades[i];
    if (fases[i] > 6.2831) fases[i] -= 6.2831;

    float onda = (sin(fases[i]) + 1.0) * 0.5;  // rango 0–1
    uint8_t brillo = brilloBase + (onda - 0.5) * amplitud;
    // brillo = constrain(brillo, 0, 180);

    leds[i] = CHSV(hue, satBase, brillo);
  }

  // ⚠️ No se llama FastLED.show() aquí
}
