/*
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

  // Desactiva los viajeros y asistentes
  for (int i = 0; i < MAX_VIAJEROS; i++) viajeros[i].activo = false;
  for (int i = 0; i < MAX_ASISTENTES; i++) asistentes[i].activo = false;
  nextViajero = 0;

  // Inicia la transición
  transicionLienzoActiva = true;
  inicioTransicion = millis();
  progresoTransicion = 0.0;

  Serial.println("-----Lienzo Blanco (transición iniciada)-----");
}


*/