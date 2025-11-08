
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
    for (int i = 0; i < MAX_ASISTENTES; i++) {
      if (!asistentes[i].activo) {
        asistentes[i].pos = 0;
        asistentes[i].colorIndex = coloresGrupoAsistente[random(0, 2)];
        asistentes[i].activo = true;
        Serial.print("Asistente lanzado con color ");
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