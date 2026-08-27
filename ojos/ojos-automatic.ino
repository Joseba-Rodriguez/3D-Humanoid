/*
  ojos-automatic.ino
  Control automático de ojos animatrónicos (humanoide 3D) - Arduino Mega 2560
  6 servos: 2 párpados superiores, 2 párpados inferiores, 1 eje vertical (arriba/abajo),
  1 eje horizontal (izquierda/derecha).

  Tabla de conexionado:
  Servo -> Pin digital -> Función
  10    -> 39           -> Párpado arriba izquierdo
  11    -> 40           -> Párpado arriba derecho
  12    -> 41           -> Párpado abajo izquierdo
  13    -> 42           -> Párpado abajo derecho
  14    -> 43           -> Movimiento vertical de ojos (ambos)
  15    -> 44           -> Movimiento horizontal de ojos (ambos)

  IMPORTANTE: Los ángulos definidos abajo (ABIERTO/CERRADO, CENTRO/LIMITES) son valores
  de partida. Ajusta las constantes según el rango físico real de tus servos y mecanismos
  antes de dejarlo funcionando sin supervisión.
*/

#include <Servo.h>

// ---------- Pines ----------
const uint8_t PIN_PARPADO_SUP_IZQ = 39;
const uint8_t PIN_PARPADO_SUP_DER = 40;
const uint8_t PIN_PARPADO_INF_IZQ = 41;
const uint8_t PIN_PARPADO_INF_DER = 42;
const uint8_t PIN_OJOS_VERTICAL   = 43;
const uint8_t PIN_OJOS_HORIZONTAL = 44;

// ---------- Objetos Servo ----------
Servo servoParpadoSupIzq;
Servo servoParpadoSupDer;
Servo servoParpadoInfIzq;
Servo servoParpadoInfDer;
Servo servoOjosVertical;
Servo servoOjosHorizontal;

// ---------- Ángulos de los párpados (AJUSTAR según montaje) ----------
const int PARPADO_SUP_IZQ_ABIERTO = 90;
const int PARPADO_SUP_IZQ_CERRADO = 130;
const int PARPADO_SUP_DER_ABIERTO = 90;
const int PARPADO_SUP_DER_CERRADO = 50;

const int PARPADO_INF_IZQ_ABIERTO = 90;
const int PARPADO_INF_IZQ_CERRADO = 50;
const int PARPADO_INF_DER_ABIERTO = 90;
const int PARPADO_INF_DER_CERRADO = 130;

// ---------- Ángulos de movimiento ocular (AJUSTAR según montaje) ----------
const int OJOS_VERTICAL_CENTRO   = 90;
const int OJOS_VERTICAL_ARRIBA   = 70;
const int OJOS_VERTICAL_ABAJO    = 110;

const int OJOS_HORIZONTAL_CENTRO   = 90;
const int OJOS_HORIZONTAL_IZQUIERDA = 60;
const int OJOS_HORIZONTAL_DERECHA   = 120;

// ---------- Temporización ----------
const uint16_t PARPADEO_MIN_MS = 2000;   // tiempo mínimo entre parpadeos
const uint16_t PARPADEO_MAX_MS = 6000;   // tiempo máximo entre parpadeos
const uint16_t PARPADEO_DURACION_MS = 120; // duración de cerrado/abierto de párpado
const uint8_t  PROBABILIDAD_DOBLE_PARPADEO = 20; // % de veces que parpadea dos veces seguidas

const uint16_t MIRADA_MIN_MS = 1500;  // tiempo mínimo entre cambios de mirada
const uint16_t MIRADA_MAX_MS = 4500;  // tiempo máximo entre cambios de mirada
const uint8_t  PASOS_MOVIMIENTO_SUAVE = 20; // nº de pasos para interpolar el movimiento
const uint8_t  RETARDO_PASO_MS = 15;        // retardo entre pasos de interpolación

// ---------- Estado ----------
unsigned long proximoParpadeo = 0;
unsigned long proximaMirada = 0;
int verticalActual = OJOS_VERTICAL_CENTRO;
int horizontalActual = OJOS_HORIZONTAL_CENTRO;

void setup() {
  servoParpadoSupIzq.attach(PIN_PARPADO_SUP_IZQ);
  servoParpadoSupDer.attach(PIN_PARPADO_SUP_DER);
  servoParpadoInfIzq.attach(PIN_PARPADO_INF_IZQ);
  servoParpadoInfDer.attach(PIN_PARPADO_INF_DER);
  servoOjosVertical.attach(PIN_OJOS_VERTICAL);
  servoOjosHorizontal.attach(PIN_OJOS_HORIZONTAL);

  randomSeed(analogRead(A0));

  // Posición inicial: ojos abiertos y mirando al centro
  abrirParpados();
  servoOjosVertical.write(OJOS_VERTICAL_CENTRO);
  servoOjosHorizontal.write(OJOS_HORIZONTAL_CENTRO);
  verticalActual = OJOS_VERTICAL_CENTRO;
  horizontalActual = OJOS_HORIZONTAL_CENTRO;

  proximoParpadeo = millis() + random(PARPADEO_MIN_MS, PARPADEO_MAX_MS);
  proximaMirada = millis() + random(MIRADA_MIN_MS, MIRADA_MAX_MS);
}

void loop() {
  unsigned long ahora = millis();

  if (ahora >= proximoParpadeo) {
    parpadear();
    if (random(100) < PROBABILIDAD_DOBLE_PARPADEO) {
      delay(150);
      parpadear();
    }
    proximoParpadeo = millis() + random(PARPADEO_MIN_MS, PARPADEO_MAX_MS);
  }

  if (ahora >= proximaMirada) {
    moverMiradaAleatoria();
    proximaMirada = millis() + random(MIRADA_MIN_MS, MIRADA_MAX_MS);
  }
}

// Cierra y vuelve a abrir los cuatro párpados de forma sincronizada
void parpadear() {
  cerrarParpados();
  delay(PARPADEO_DURACION_MS);
  abrirParpados();
}

void cerrarParpados() {
  servoParpadoSupIzq.write(PARPADO_SUP_IZQ_CERRADO);
  servoParpadoSupDer.write(PARPADO_SUP_DER_CERRADO);
  servoParpadoInfIzq.write(PARPADO_INF_IZQ_CERRADO);
  servoParpadoInfDer.write(PARPADO_INF_DER_CERRADO);
}

void abrirParpados() {
  servoParpadoSupIzq.write(PARPADO_SUP_IZQ_ABIERTO);
  servoParpadoSupDer.write(PARPADO_SUP_DER_ABIERTO);
  servoParpadoInfIzq.write(PARPADO_INF_IZQ_ABIERTO);
  servoParpadoInfDer.write(PARPADO_INF_DER_ABIERTO);
}

// Elige una nueva dirección de mirada al azar y se mueve suavemente hasta ella
void moverMiradaAleatoria() {
  int nuevoVertical = random(OJOS_VERTICAL_ARRIBA, OJOS_VERTICAL_ABAJO + 1);
  int nuevoHorizontal = random(OJOS_HORIZONTAL_IZQUIERDA, OJOS_HORIZONTAL_DERECHA + 1);
  moverMiradaSuave(nuevoVertical, nuevoHorizontal);
}

// Interpola el movimiento de los dos servos oculares para que sea gradual, no brusco
void moverMiradaSuave(int destinoVertical, int destinoHorizontal) {
  int origenVertical = verticalActual;
  int origenHorizontal = horizontalActual;

  for (uint8_t paso = 1; paso <= PASOS_MOVIMIENTO_SUAVE; paso++) {
    int v = map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origenVertical, destinoVertical);
    int h = map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origenHorizontal, destinoHorizontal);
    servoOjosVertical.write(v);
    servoOjosHorizontal.write(h);
    delay(RETARDO_PASO_MS);
  }

  verticalActual = destinoVertical;
  horizontalActual = destinoHorizontal;
}
