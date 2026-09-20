/*
  humanoide-completo.ino
  Programa unificado del humanoide 3D - Arduino Mega 2560.
  Une en un solo sketch los tres módulos que antes iban por separado:
    - CUELLO  (3 servos: inclinación izq/der + rotación)
    - OJOS    (6 servos: 4 párpados + movimiento vertical/horizontal)
    - MANO    (7 servos: dedos + muñeca + codo)

  Cada módulo conserva exactamente su misma lógica y sus mismos valores/ángulos
  que tenía por separado. Lo único que cambia es que algunos nombres de
  variables/funciones que se repetían entre módulos (POSES, moverAPoseSuave,
  imprimirPose, etc.) se han renombrado añadiendo el sufijo Cuello/Ojos/Mano
  para que puedan convivir en el mismo archivo sin chocar.

  Tabla de conexionado completa:
  Módulo  Motor -> Pin -> Función
  CUELLO  7     -> 36  -> Inclinación de cuello hacia la izquierda
  CUELLO  8     -> 37  -> Inclinación de cuello hacia la derecha
  CUELLO  9     -> 38  -> Giro rotatorio del cuello (izquierda/derecha)
  OJOS    10    -> 39  -> Párpado arriba izquierdo
  OJOS    11    -> 40  -> Párpado arriba derecho
  OJOS    12    -> 41  -> Párpado abajo izquierdo
  OJOS    13    -> 42  -> Párpado abajo derecho
  OJOS    14    -> 43  -> Movimiento vertical de ojos (ambos)
  OJOS    15    -> 44  -> Movimiento horizontal de ojos (ambos)
  MANO    1     -> 30  -> Pulgar, flexión
  MANO    2     -> 31  -> Índice
  MANO    3     -> 32  -> Corazón
  MANO    4     -> 33  -> Anular + meñique
  MANO    5     -> 34  -> Pulgar, eje vertical / oposición
  MANO    6     -> 35  -> Muñeca, giro izquierda/derecha
  MANO    16    -> 45  -> Codo, movimiento arriba/abajo

  MODOS DE FUNCIONAMIENTO DE LA MANO (Monitor Serie a 9600 baudios):
  Escribe un número y pulsa Enter para cambiar de modo en cualquier momento:
    0 -> Modo por defecto: poses aleatorias.
    1 -> Modo "Piedra, papel o tijera".
  (El cuello y los ojos se mueven siempre solos, en paralelo, sin necesidad de comandos).
*/

#include <Servo.h>

// ======================================================================
// =========================== CUELLO ==================================
// ======================================================================

// ---------- Pines (cuello) ----------
const uint8_t PIN_INCLINACION_IZQUIERDA = 36;
const uint8_t PIN_INCLINACION_DERECHA   = 37;
const uint8_t PIN_ROTACION              = 38;

// ---------- Objetos Servo (cuello) ----------
Servo servoInclinacionIzquierda;
Servo servoInclinacionDerecha;
Servo servoRotacion;

// ---------- Posición neutra y amplitud de movimiento (cuello) ----------
const int NEUTRO = 90;

const int AMPLITUD_INCLINACION = 25;
const int AMPLITUD_ROTACION    = 35;

const int INCLINACION_IZQ_ACTIVA   = NEUTRO + AMPLITUD_INCLINACION;
const int INCLINACION_IZQ_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION / 2);
const int INCLINACION_DER_ACTIVA   = NEUTRO + AMPLITUD_INCLINACION;
const int INCLINACION_DER_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION / 2);

const int ROTACION_IZQUIERDA = NEUTRO - AMPLITUD_ROTACION;
const int ROTACION_DERECHA   = NEUTRO + AMPLITUD_ROTACION;

// ---------- Definición de una pose de cuello ----------
struct PoseCuello {
  const char* nombre;
  int inclinacionIzquierda;
  int inclinacionDerecha;
  int rotacion;
};

const PoseCuello POSES_CUELLO[] = {
  { "Centro",                       NEUTRO,                   NEUTRO,                   NEUTRO           },
  { "Inclinar poco a la izquierda", INCLINACION_IZQ_ACTIVA,   INCLINACION_DER_RELAJADA, NEUTRO           },
  { "Inclinar poco a la derecha",   INCLINACION_IZQ_RELAJADA, INCLINACION_DER_ACTIVA,   NEUTRO           },
  { "Girar poco a la izquierda",    NEUTRO,                   NEUTRO,                   ROTACION_IZQUIERDA },
  { "Girar poco a la derecha",      NEUTRO,                   NEUTRO,                   ROTACION_DERECHA   },
};
const uint8_t NUM_POSES_CUELLO = sizeof(POSES_CUELLO) / sizeof(POSES_CUELLO[0]);

// ---------- Temporización (cuello) ----------
const uint16_t TIEMPO_MIN_ENTRE_MOVIMIENTOS_MS = 3000;
const uint16_t TIEMPO_MAX_ENTRE_MOVIMIENTOS_MS = 7000;
const uint16_t PASOS_MOVIMIENTO_SUAVE_CUELLO = 85;
const uint8_t  RETARDO_PASO_MS_CUELLO = 40;

// ---------- Estado (cuello) ----------
PoseCuello poseActualCuello = POSES_CUELLO[0];
unsigned long proximoMovimientoCuello = 0;
uint8_t indicePoseAnteriorCuello = 0;

// ======================================================================
// ============================ OJOS ====================================
// ======================================================================

// ---------- Pines (ojos) ----------
const uint8_t PIN_PARPADO_SUP_IZQ = 39;
const uint8_t PIN_PARPADO_SUP_DER = 40;
const uint8_t PIN_PARPADO_INF_IZQ = 41;
const uint8_t PIN_PARPADO_INF_DER = 42;
const uint8_t PIN_OJOS_VERTICAL   = 43;
const uint8_t PIN_OJOS_HORIZONTAL = 44;

// ---------- Objetos Servo (ojos) ----------
Servo servoParpadoSupIzq;
Servo servoParpadoSupDer;
Servo servoParpadoInfIzq;
Servo servoParpadoInfDer;
Servo servoOjosVertical;
Servo servoOjosHorizontal;

// ---------- Ángulos de los párpados ----------
const int PARPADO_SUP_IZQ_ABIERTO = 130;
const int PARPADO_SUP_IZQ_CERRADO = 90;
const int PARPADO_SUP_DER_ABIERTO = 50;
const int PARPADO_SUP_DER_CERRADO = 90;

const int PARPADO_INF_IZQ_ABIERTO = 50;
const int PARPADO_INF_IZQ_CERRADO = 90;
const int PARPADO_INF_DER_ABIERTO = 130;
const int PARPADO_INF_DER_CERRADO = 90;

// ---------- Ángulos de movimiento ocular ----------
const int OJOS_VERTICAL_CENTRO   = 90;
const int OJOS_VERTICAL_ARRIBA   = 70;
const int OJOS_VERTICAL_ABAJO    = 110;

const int OJOS_HORIZONTAL_CENTRO   = 90;
const int OJOS_HORIZONTAL_IZQUIERDA = 70;
const int OJOS_HORIZONTAL_DERECHA   = 100;

// ---------- Temporización (ojos) ----------
const uint16_t PARPADEO_MIN_MS = 2000;
const uint16_t PARPADEO_MAX_MS = 6000;
const uint16_t PARPADEO_DURACION_MS = 120;
const uint8_t  PROBABILIDAD_DOBLE_PARPADEO = 20;

const uint16_t MIRADA_MIN_MS = 1500;
const uint16_t MIRADA_MAX_MS = 4500;
const uint8_t  PASOS_MOVIMIENTO_SUAVE_OJOS = 20;
const uint8_t  RETARDO_PASO_MS_OJOS = 15;

// ---------- Estado (ojos) ----------
unsigned long proximoParpadeo = 0;
unsigned long proximaMirada = 0;
int verticalActual = OJOS_VERTICAL_CENTRO;
int horizontalActual = OJOS_HORIZONTAL_CENTRO;

// ======================================================================
// ============================ MANO ====================================
// ======================================================================

// ---------- Pines (mano) ----------
const uint8_t PIN_PULGAR_FLEXION = 30;
const uint8_t PIN_INDICE         = 31;
const uint8_t PIN_CORAZON        = 32;
const uint8_t PIN_ANULAR_MENIQUE = 33;
//const uint8_t PIN_PULGAR_VERTICAL = 34;
const uint8_t PIN_MUNECA          = 35;
const uint8_t PIN_CODO            = 45;

const uint8_t NUM_SERVOS = 7;

// ---------- Objetos Servo (mano) ----------
Servo servoPulgarFlexion;
Servo servoIndice;
Servo servoCorazon;
Servo servoAnularMenique;
Servo servoPulgarVertical;
Servo servoMuneca;
Servo servoCodo;

// ---------- Ángulos de referencia (mano) ----------
const int DEDO_CERRADO = 0;
const int DEDO_ABIERTO = 180;
const int DEDO_MEDIO   = 90;

const int PULGAR_FLEX_CERRADO = 0;
const int PULGAR_FLEX_ABIERTO = 180;
const int PULGAR_FLEX_MEDIO   = 90;
const int PULGAR_FLEX_OK      = 25;

const int PULGAR_VERT_PLANO   = 0;
const int PULGAR_VERT_ARRIBA  = 180;
const int PULGAR_VERT_MEDIO   = 90;
const int PULGAR_VERT_OK      = 25;

const int MUNECA_IZQUIERDA = 0;
const int MUNECA_CENTRO    = 90;
const int MUNECA_DERECHA   = 180;

const int CODO_ABAJO = 0;
const int CODO_MEDIO = 90;
const int CODO_ARRIBA = 180;

// ---------- Límites de seguridad de los servos (mano) ----------
const int LIM_MIN_PULGAR_FLEX = 5;
const int LIM_MAX_PULGAR_FLEX = 175;

const int LIM_MIN_INDICE = 5;
const int LIM_MAX_INDICE = 175;

const int LIM_MIN_CORAZON = 5;
const int LIM_MAX_CORAZON = 175;

const int LIM_MIN_ANULAR_MENIQUE = 5;
const int LIM_MAX_ANULAR_MENIQUE = 175;

const int LIM_MIN_PULGAR_VERTICAL = 20;
const int LIM_MAX_PULGAR_VERTICAL = 160;

const int LIM_MIN_MUNECA = 5;
const int LIM_MAX_MUNECA = 175;

const int LIM_MIN_CODO = 5;
const int LIM_MAX_CODO = 175;

// ---------- Definición de una pose de mano ----------
struct PoseMano {
  const char* nombre;
  int pulgarFlexion;
  int indice;
  int corazon;
  int anularMenique;
  int pulgarVertical;
  int muneca;
  int codo;
};

const PoseMano POSES_MANO[] = {
  { "Mano relajada",     PULGAR_FLEX_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    PULGAR_VERT_MEDIO,  MUNECA_CENTRO,    CODO_MEDIO   },
  { "Puno cerrado",      PULGAR_FLEX_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_CENTRO,    CODO_ARRIBA  },
  { "Mano abierta",      PULGAR_FLEX_ABIERTO,  DEDO_ABIERTO,  DEDO_ABIERTO,  DEDO_ABIERTO,  PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ABAJO   },
  { "Senal de paz",      PULGAR_FLEX_CERRADO,  DEDO_ABIERTO,  DEDO_ABIERTO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_DERECHA,   CODO_MEDIO   },
  { "Pulgar arriba",     PULGAR_FLEX_ABIERTO,  DEDO_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ARRIBA  },
  { "Senalar",           PULGAR_FLEX_CERRADO,  DEDO_ABIERTO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_IZQUIERDA, CODO_ABAJO   },
  { "OK",                PULGAR_FLEX_OK,       DEDO_MEDIO,    DEDO_ABIERTO,  DEDO_ABIERTO,  PULGAR_VERT_OK,     MUNECA_CENTRO,    CODO_MEDIO   },
  { "Garra",             PULGAR_FLEX_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    PULGAR_VERT_MEDIO,  MUNECA_DERECHA,   CODO_ARRIBA  },
};
const uint8_t NUM_POSES_MANO = sizeof(POSES_MANO) / sizeof(POSES_MANO[0]);

// Poses del juego "Piedra, papel o tijera"
const PoseMano JUGADAS_RPS[] = {
  { "Piedra",  PULGAR_FLEX_CERRADO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_CENTRO, CODO_ARRIBA },
  { "Papel",   PULGAR_FLEX_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO, CODO_MEDIO  },
  { "Tijera",  PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_CENTRO, CODO_MEDIO  },
};
const uint8_t NUM_JUGADAS_RPS = sizeof(JUGADAS_RPS) / sizeof(JUGADAS_RPS[0]);

// ---------- Modos de funcionamiento (mano) ----------
const uint8_t MODO_POSES_ALEATORIAS = 0;
const uint8_t MODO_PIEDRA_PAPEL_TIJERA = 1;
uint8_t modoActual = MODO_POSES_ALEATORIAS;

// ---------- Temporización (mano) ----------
const uint16_t TIEMPO_MIN_ENTRE_POSES_MS = 3000;
const uint16_t TIEMPO_MAX_ENTRE_POSES_MS = 7000;
const uint16_t TIEMPO_MIN_ENTRE_JUGADAS_MS = 4000;
const uint16_t TIEMPO_MAX_ENTRE_JUGADAS_MS = 8000;
const uint8_t  PASOS_MOVIMIENTO_SUAVE_MANO = 30;
const uint8_t  RETARDO_PASO_MS_MANO = 20;

// ---------- Estado (mano) ----------
PoseMano poseActualMano = POSES_MANO[0];
unsigned long proximoCambioPose = 0;
unsigned long proximaJugadaRPS = 0;
uint8_t indicePoseAnteriorMano = 0;
uint8_t indiceJugadaAnterior = 0;

// ======================================================================
// =========================== SETUP / LOOP =============================
// ======================================================================

void setup() {
  Serial.begin(9600);

  // --- Cuello: enganchar servos ---
  servoInclinacionIzquierda.attach(PIN_INCLINACION_IZQUIERDA);
  servoInclinacionDerecha.attach(PIN_INCLINACION_DERECHA);
  servoRotacion.attach(PIN_ROTACION);

  // --- Ojos: enganchar servos ---
  servoParpadoSupIzq.attach(PIN_PARPADO_SUP_IZQ);
  servoParpadoSupDer.attach(PIN_PARPADO_SUP_DER);
  servoParpadoInfIzq.attach(PIN_PARPADO_INF_IZQ);
  servoParpadoInfDer.attach(PIN_PARPADO_INF_DER);
  servoOjosVertical.attach(PIN_OJOS_VERTICAL);
  servoOjosHorizontal.attach(PIN_OJOS_HORIZONTAL);

  // --- Mano: enganchar servos ---
  servoPulgarFlexion.attach(PIN_PULGAR_FLEXION);
  servoIndice.attach(PIN_INDICE);
  servoCorazon.attach(PIN_CORAZON);
  servoAnularMenique.attach(PIN_ANULAR_MENIQUE);
//  servoPulgarVertical.attach(PIN_PULGAR_VERTICAL);
  servoMuneca.attach(PIN_MUNECA);
  servoCodo.attach(PIN_CODO);

  randomSeed(analogRead(A0));

  Serial.println(F("=== Humanoide iniciado: cuello + ojos + mano ==="));

  // --- Cuello: posición inicial ---
  aplicarPoseInstantaneaCuello(POSES_CUELLO[0]);
  poseActualCuello = POSES_CUELLO[0];
  imprimirPoseCuello(POSES_CUELLO[0]);
  proximoMovimientoCuello = millis() + random(TIEMPO_MIN_ENTRE_MOVIMIENTOS_MS, TIEMPO_MAX_ENTRE_MOVIMIENTOS_MS);

  // --- Ojos: posición inicial (abiertos, mirando al centro) ---
  abrirParpados();
  servoOjosVertical.write(OJOS_VERTICAL_CENTRO);
  servoOjosHorizontal.write(OJOS_HORIZONTAL_CENTRO);
  verticalActual = OJOS_VERTICAL_CENTRO;
  horizontalActual = OJOS_HORIZONTAL_CENTRO;
  proximoParpadeo = millis() + random(PARPADEO_MIN_MS, PARPADEO_MAX_MS);
  proximaMirada = millis() + random(MIRADA_MIN_MS, MIRADA_MAX_MS);

  // --- Mano: posición inicial ---
  mostrarMenuModos();
  aplicarPoseInstantaneaMano(POSES_MANO[0]);
  poseActualMano = POSES_MANO[0];
  imprimirPoseMano(POSES_MANO[0]);
  proximoCambioPose = millis() + random(TIEMPO_MIN_ENTRE_POSES_MS, TIEMPO_MAX_ENTRE_POSES_MS);
  proximaJugadaRPS = millis() + random(TIEMPO_MIN_ENTRE_JUGADAS_MS, TIEMPO_MAX_ENTRE_JUGADAS_MS);
}

void loop() {
  // --- Mano: lee cambios de modo por Serial y ejecuta su lógica ---
  leerCambioDeModo();
  if (modoActual == MODO_PIEDRA_PAPEL_TIJERA) {
    loopPiedraPapelTijera();
  } else {
    loopPosesAleatorias();
  }

  // --- Ojos: parpadeo y mirada ---
  loopOjos();

  // --- Cuello: movimiento lento de cabeza ---
  loopCuello();
}

// ======================================================================
// ======================= FUNCIONES: CUELLO =============================
// ======================================================================

void loopCuello() {
  if (millis() >= proximoMovimientoCuello) {
    uint8_t indiceNuevaPose = elegirPoseDistintaCuello(indicePoseAnteriorCuello);
    Serial.print(F("Nuevo movimiento de cuello -> "));
    Serial.println(POSES_CUELLO[indiceNuevaPose].nombre);

    moverAPoseSuaveCuello(POSES_CUELLO[indiceNuevaPose]);
    poseActualCuello = POSES_CUELLO[indiceNuevaPose];
    indicePoseAnteriorCuello = indiceNuevaPose;

    imprimirPoseCuello(POSES_CUELLO[indiceNuevaPose]);

    proximoMovimientoCuello = millis() + random(TIEMPO_MIN_ENTRE_MOVIMIENTOS_MS, TIEMPO_MAX_ENTRE_MOVIMIENTOS_MS);
  }
}

void imprimirPoseCuello(const PoseCuello &pose) {
  Serial.print(F("Pose de cuello aplicada: "));
  Serial.println(pose.nombre);
  Serial.print(F("  Inclinacion izquierda: ")); Serial.println(pose.inclinacionIzquierda);
  Serial.print(F("  Inclinacion derecha: ")); Serial.println(pose.inclinacionDerecha);
  Serial.print(F("  Rotacion: ")); Serial.println(pose.rotacion);
  Serial.println(F("----------------------------------"));
}

uint8_t elegirPoseDistintaCuello(uint8_t indiceAnterior) {
  uint8_t indiceNuevo;
  do {
    indiceNuevo = random(0, NUM_POSES_CUELLO);
  } while (indiceNuevo == indiceAnterior && NUM_POSES_CUELLO > 1);
  return indiceNuevo;
}

void aplicarPoseInstantaneaCuello(const PoseCuello &pose) {
  servoInclinacionIzquierda.write(pose.inclinacionIzquierda);
  servoInclinacionDerecha.write(pose.inclinacionDerecha);
  servoRotacion.write(pose.rotacion);
}

void moverAPoseSuaveCuello(const PoseCuello &destino) {
  PoseCuello origen = poseActualCuello;

  for (uint16_t paso = 1; paso <= PASOS_MOVIMIENTO_SUAVE_CUELLO; paso++) {
    servoInclinacionIzquierda.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE_CUELLO, origen.inclinacionIzquierda, destino.inclinacionIzquierda));
    servoInclinacionDerecha.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE_CUELLO, origen.inclinacionDerecha, destino.inclinacionDerecha));
    servoRotacion.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE_CUELLO, origen.rotacion, destino.rotacion));
    delay(RETARDO_PASO_MS_CUELLO);
  }
}

// ======================================================================
// ======================== FUNCIONES: OJOS ===============================
// ======================================================================

void loopOjos() {
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

void moverMiradaAleatoria() {
  int nuevoVertical = random(OJOS_VERTICAL_ARRIBA, OJOS_VERTICAL_ABAJO + 1);
  int nuevoHorizontal = random(OJOS_HORIZONTAL_IZQUIERDA, OJOS_HORIZONTAL_DERECHA + 1);
  moverMiradaSuave(nuevoVertical, nuevoHorizontal);
}

void moverMiradaSuave(int destinoVertical, int destinoHorizontal) {
  int origenVertical = verticalActual;
  int origenHorizontal = horizontalActual;

  for (uint8_t paso = 1; paso <= PASOS_MOVIMIENTO_SUAVE_OJOS; paso++) {
    int v = map(paso, 0, PASOS_MOVIMIENTO_SUAVE_OJOS, origenVertical, destinoVertical);
    int h = map(paso, 0, PASOS_MOVIMIENTO_SUAVE_OJOS, origenHorizontal, destinoHorizontal);
    servoOjosVertical.write(v);
    servoOjosHorizontal.write(h);
    delay(RETARDO_PASO_MS_OJOS);
  }

  verticalActual = destinoVertical;
  horizontalActual = destinoHorizontal;
}

// ======================================================================
// ======================== FUNCIONES: MANO ===============================
// ======================================================================

void mostrarMenuModos() {
  Serial.println(F("Escribe un numero y pulsa Enter para cambiar de modo:"));
  Serial.println(F("  0 -> Poses aleatorias (por defecto)"));
  Serial.println(F("  1 -> Piedra, papel o tijera"));
}

void leerCambioDeModo() {
  if (Serial.available() == 0) {
    return;
  }

  int valor = Serial.parseInt();
  while (Serial.available() > 0) {
    Serial.read();
  }

  if (valor != MODO_POSES_ALEATORIAS && valor != MODO_PIEDRA_PAPEL_TIJERA) {
    Serial.println(F("Modo invalido. Usa 0 (poses aleatorias) o 1 (piedra, papel o tijera)."));
    return;
  }

  modoActual = valor;
  Serial.print(F("Modo cambiado a: "));
  Serial.println(modoActual == MODO_PIEDRA_PAPEL_TIJERA ? F("Piedra, papel o tijera") : F("Poses aleatorias"));

  proximoCambioPose = millis() + random(TIEMPO_MIN_ENTRE_POSES_MS, TIEMPO_MAX_ENTRE_POSES_MS);
  proximaJugadaRPS = millis() + random(TIEMPO_MIN_ENTRE_JUGADAS_MS, TIEMPO_MAX_ENTRE_JUGADAS_MS);
}

void loopPosesAleatorias() {
  if (millis() >= proximoCambioPose) {
    uint8_t indiceNuevaPose = elegirPoseDistintaMano(indicePoseAnteriorMano);
    Serial.print(F("Nueva pose seleccionada -> "));
    Serial.println(POSES_MANO[indiceNuevaPose].nombre);

    moverAPoseSuaveMano(POSES_MANO[indiceNuevaPose]);
    poseActualMano = POSES_MANO[indiceNuevaPose];
    indicePoseAnteriorMano = indiceNuevaPose;

    imprimirPoseMano(POSES_MANO[indiceNuevaPose]);

    proximoCambioPose = millis() + random(TIEMPO_MIN_ENTRE_POSES_MS, TIEMPO_MAX_ENTRE_POSES_MS);
  }
}

void loopPiedraPapelTijera() {
  if (millis() >= proximaJugadaRPS) {
    uint8_t indiceNuevaJugada;
    do {
      indiceNuevaJugada = random(0, NUM_JUGADAS_RPS);
    } while (indiceNuevaJugada == indiceJugadaAnterior && NUM_JUGADAS_RPS > 1);

    Serial.print(F("Jugando... -> "));
    Serial.println(JUGADAS_RPS[indiceNuevaJugada].nombre);

    moverAPoseSuaveMano(JUGADAS_RPS[indiceNuevaJugada]);
    poseActualMano = JUGADAS_RPS[indiceNuevaJugada];
    indiceJugadaAnterior = indiceNuevaJugada;

    imprimirPoseMano(JUGADAS_RPS[indiceNuevaJugada]);

    proximaJugadaRPS = millis() + random(TIEMPO_MIN_ENTRE_JUGADAS_MS, TIEMPO_MAX_ENTRE_JUGADAS_MS);
  }
}

void imprimirPoseMano(const PoseMano &pose) {
  Serial.print(F("Pose aplicada: "));
  Serial.println(pose.nombre);
  Serial.print(F("  Pulgar flexion: ")); Serial.println(pose.pulgarFlexion);
  Serial.print(F("  Indice: ")); Serial.println(pose.indice);
  Serial.print(F("  Corazon: ")); Serial.println(pose.corazon);
  Serial.print(F("  Anular+Menique: ")); Serial.println(pose.anularMenique);
  Serial.print(F("  Pulgar vertical: ")); Serial.println(pose.pulgarVertical);
  Serial.print(F("  Muneca: ")); Serial.println(pose.muneca);
  Serial.print(F("  Codo: ")); Serial.println(pose.codo);
  Serial.println(F("----------------------------------"));
}

uint8_t elegirPoseDistintaMano(uint8_t indiceAnterior) {
  uint8_t indiceNuevo;
  do {
    indiceNuevo = random(0, NUM_POSES_MANO);
  } while (indiceNuevo == indiceAnterior && NUM_POSES_MANO > 1);
  return indiceNuevo;
}

void escribirServoSeguro(Servo &servo, int angulo, int limMin, int limMax) {
  int anguloSeguro = constrain(angulo, limMin, limMax);
  servo.write(anguloSeguro);
}

void escribirMuneca(int anguloLogico) {
  int anguloFisico = 180 - anguloLogico;
  escribirServoSeguro(servoMuneca, anguloFisico, LIM_MIN_MUNECA, LIM_MAX_MUNECA);
}

void aplicarPoseInstantaneaMano(const PoseMano &pose) {
  escribirServoSeguro(servoPulgarFlexion, pose.pulgarFlexion, LIM_MIN_PULGAR_FLEX, LIM_MAX_PULGAR_FLEX);
  escribirServoSeguro(servoIndice, pose.indice, LIM_MIN_INDICE, LIM_MAX_INDICE);
  escribirServoSeguro(servoCorazon, pose.corazon, LIM_MIN_CORAZON, LIM_MAX_CORAZON);
  escribirServoSeguro(servoAnularMenique, pose.anularMenique, LIM_MIN_ANULAR_MENIQUE, LIM_MAX_ANULAR_MENIQUE);
  escribirServoSeguro(servoPulgarVertical, pose.pulgarVertical, LIM_MIN_PULGAR_VERTICAL, LIM_MAX_PULGAR_VERTICAL);
  escribirMuneca(pose.muneca);
  escribirServoSeguro(servoCodo, pose.codo, LIM_MIN_CODO, LIM_MAX_CODO);
}

void moverAPoseSuaveMano(const PoseMano &destino) {
  PoseMano origen = poseActualMano;

  for (uint8_t paso = 1; paso <= PASOS_MOVIMIENTO_SUAVE_MANO; paso++) {
    escribirServoSeguro(servoPulgarFlexion, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.pulgarFlexion, destino.pulgarFlexion), LIM_MIN_PULGAR_FLEX, LIM_MAX_PULGAR_FLEX);
    escribirServoSeguro(servoIndice, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.indice, destino.indice), LIM_MIN_INDICE, LIM_MAX_INDICE);
    escribirServoSeguro(servoCorazon, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.corazon, destino.corazon), LIM_MIN_CORAZON, LIM_MAX_CORAZON);
    escribirServoSeguro(servoAnularMenique, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.anularMenique, destino.anularMenique), LIM_MIN_ANULAR_MENIQUE, LIM_MAX_ANULAR_MENIQUE);
    escribirServoSeguro(servoPulgarVertical, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.pulgarVertical, destino.pulgarVertical), LIM_MIN_PULGAR_VERTICAL, LIM_MAX_PULGAR_VERTICAL);
    escribirMuneca(map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.muneca, destino.muneca));
    escribirServoSeguro(servoCodo, map(paso, 0, PASOS_MOVIMIENTO_SUAVE_MANO, origen.codo, destino.codo), LIM_MIN_CODO, LIM_MAX_CODO);
    delay(RETARDO_PASO_MS_MANO);
  }
}
