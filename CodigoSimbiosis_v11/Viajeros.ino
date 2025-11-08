
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