/*
  humanoide-automatico.ino
  PROGRAMA MAESTRO - Arduino Mega 2560.
  Controla TODO el humanoide de forma autónoma y simultánea: ojos, mano (con muñeca
  y codo) y cuello. Cada parte se mueve de forma independiente y no bloqueante, así
  que todas funcionan a la vez sin que unas tengan que esperar a otras (por ejemplo,
  el cuello puede estar girando muy despacio mientras la mano cambia de pose y los
  ojos parpadean, todo al mismo tiempo).

  NO incluye la carpeta flex-mano (esa mano usa sensores flex y es un programa aparte).

  ============================================================
  TABLA COMPLETA DE CONEXIONADO (16 servos en total)
  ============================================================
  OJOS:
  Motor -> Pin -> Función
  10    -> 39  -> Párpado arriba izquierdo
  11    -> 40  -> Párpado arriba derecho
  12    -> 41  -> Párpado abajo izquierdo
  13    -> 42  -> Párpado abajo derecho
  14    -> 43  -> Movimiento vertical de ojos (ambos)
  15    -> 44  -> Movimiento horizontal de ojos (ambos)

  MANO + MUÑECA + CODO:
  Motor -> Pin -> Función
  1     -> 30  -> Pulgar, flexión (recto <-> hacia la palma)
  2     -> 31  -> Índice
  3     -> 32  -> Corazón
  4     -> 33  -> Anular + meñique (juntos)
  5     -> 34  -> Pulgar, eje vertical/oposición (sincronizado con motor 1)
  6     -> 35  -> Muñeca, giro izquierda/derecha
  16    -> 45  -> Codo, arriba/abajo

  CUELLO (movimiento lento y pequeño, la cabeza pesa):
  Motor -> Pin -> Función
  7     -> 36  -> Inclinación de cuello hacia la izquierda
  8     -> 37  -> Inclinación de cuello hacia la derecha
  9     -> 38  -> Giro rotatorio del cuello

  ============================================================
  MODOS DE LA MANO (Monitor Serie a 9600 baudios, escribe un número y Enter):
    0 -> Poses aleatorias de mano (por defecto)
    1 -> La mano juega sola a "Piedra, papel o tijera"
  Los ojos y el cuello siempre funcionan en modo automático, en paralelo.

  IMPORTANTE: Todos los ángulos de partida (párpados, mirada, dedos, muñeca, codo,
  cuello) son valores conservadores de ejemplo. Ajusta las constantes de cada bloque
  según lo que veas físicamente en tu humanoide montado.
*/

#include <Servo.h>

// ================================================================
// PINES
// ================================================================
// Ojos
const uint8_t PIN_PARPADO_SUP_IZQ = 39;
const uint8_t PIN_PARPADO_SUP_DER = 40;
const uint8_t PIN_PARPADO_INF_IZQ = 41;
const uint8_t PIN_PARPADO_INF_DER = 42;
const uint8_t PIN_OJOS_VERTICAL   = 43;
const uint8_t PIN_OJOS_HORIZONTAL = 44;

// Mano + muñeca + codo
const uint8_t PIN_PULGAR_FLEXION  = 30;
const uint8_t PIN_INDICE          = 31;
const uint8_t PIN_CORAZON         = 32;
const uint8_t PIN_ANULAR_MENIQUE  = 33;
const uint8_t PIN_PULGAR_VERTICAL = 34;
const uint8_t PIN_MUNECA          = 35;
const uint8_t PIN_CODO            = 45;

// Cuello
const uint8_t PIN_CUELLO_INCLINACION_IZQ = 36;
const uint8_t PIN_CUELLO_INCLINACION_DER = 37;
const uint8_t PIN_CUELLO_ROTACION        = 38;

// ================================================================
// OBJETOS SERVO
// ================================================================
Servo servoParpadoSupIzq, servoParpadoSupDer, servoParpadoInfIzq, servoParpadoInfDer;
Servo servoOjosVertical, servoOjosHorizontal;

Servo servoPulgarFlexion, servoIndice, servoCorazon, servoAnularMenique;
Servo servoPulgarVertical, servoMuneca, servoCodo;

Servo servoCuelloInclinacionIzq, servoCuelloInclinacionDer, servoCuelloRotacion;

// ================================================================
// OJOS - constantes y estado
// ================================================================
const int PARPADO_SUP_IZQ_ABIERTO = 90, PARPADO_SUP_IZQ_CERRADO = 130;
const int PARPADO_SUP_DER_ABIERTO = 90, PARPADO_SUP_DER_CERRADO = 50;
const int PARPADO_INF_IZQ_ABIERTO = 90, PARPADO_INF_IZQ_CERRADO = 50;
const int PARPADO_INF_DER_ABIERTO = 90, PARPADO_INF_DER_CERRADO = 130;

const int OJOS_VERTICAL_CENTRO = 90, OJOS_VERTICAL_ARRIBA = 70, OJOS_VERTICAL_ABAJO = 110;
const int OJOS_HORIZONTAL_CENTRO = 90, OJOS_HORIZONTAL_IZQUIERDA = 60, OJOS_HORIZONTAL_DERECHA = 120;

const uint16_t PARPADEO_MIN_MS = 2000, PARPADEO_MAX_MS = 6000;
const uint16_t PARPADEO_DURACION_MS = 120;
const uint8_t  PROBABILIDAD_DOBLE_PARPADEO = 20; // %

const uint16_t MIRADA_MIN_MS = 1500, MIRADA_MAX_MS = 4500;
const uint8_t  GAZE_TOTAL_PASOS = 20;
const uint8_t  GAZE_RETARDO_PASO_MS = 15;

const uint8_t PARPADEO_ESPERANDO = 0, PARPADEO_CERRADO = 1;
uint8_t estadoParpadeo = PARPADEO_ESPERANDO;
unsigned long proximoParpadeo = 0, tiempoReaperturaParpados = 0;
bool dobleParpadeoPendiente = false;

bool gazeEnMovimiento = false;
unsigned long proximaMirada = 0, gazeUltimoPasoMillis = 0;
uint8_t gazePasoActual = 0;
int gazeVerticalActual = OJOS_VERTICAL_CENTRO, gazeHorizontalActual = OJOS_HORIZONTAL_CENTRO;
int gazeVerticalOrigen, gazeHorizontalOrigen, gazeVerticalDestino, gazeHorizontalDestino;

// ================================================================
// MANO + MUÑECA + CODO - constantes y estado
// ================================================================
const int DEDO_CERRADO = 0, DEDO_ABIERTO = 180, DEDO_MEDIO = 90;
const int PULGAR_FLEX_CERRADO = 0, PULGAR_FLEX_ABIERTO = 180, PULGAR_FLEX_MEDIO = 90;
const int PULGAR_VERT_PLANO = 0, PULGAR_VERT_ARRIBA = 180, PULGAR_VERT_MEDIO = 90;
const int MUNECA_IZQUIERDA = 0, MUNECA_CENTRO = 90, MUNECA_DERECHA = 180;
const int CODO_ABAJO = 0, CODO_MEDIO = 90, CODO_ARRIBA = 180;

struct PoseMano {
  const char* nombre;
  int pulgarFlexion, indice, corazon, anularMenique, pulgarVertical, muneca, codo;
};

const PoseMano POSES_MANO[] = {
  { "Mano relajada",   PULGAR_FLEX_MEDIO,   DEDO_MEDIO,   DEDO_MEDIO,   DEDO_MEDIO,   PULGAR_VERT_MEDIO,  MUNECA_CENTRO,    CODO_MEDIO  },
  { "Puno cerrado",    PULGAR_FLEX_CERRADO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_CENTRO,    CODO_ARRIBA },
  { "Mano abierta",    PULGAR_FLEX_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ABAJO  },
  { "Senal de paz",    PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_DERECHA,   CODO_MEDIO  },
  { "Pulgar arriba",   PULGAR_FLEX_ABIERTO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ARRIBA },
  { "Senalar",         PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_IZQUIERDA, CODO_ABAJO  },
  { "OK",              PULGAR_FLEX_MEDIO,   DEDO_MEDIO,   DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_MEDIO,  MUNECA_CENTRO,    CODO_MEDIO  },
  { "Garra",           PULGAR_FLEX_MEDIO,   DEDO_MEDIO,   DEDO_MEDIO,   DEDO_MEDIO,   PULGAR_VERT_MEDIO,  MUNECA_DERECHA,   CODO_ARRIBA },
};
const uint8_t NUM_POSES_MANO = sizeof(POSES_MANO) / sizeof(POSES_MANO[0]);

const PoseMano JUGADAS_RPS[] = {
  { "Piedra", PULGAR_FLEX_CERRADO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_CENTRO, CODO_ARRIBA },
  { "Papel",  PULGAR_FLEX_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO, CODO_MEDIO  },
  { "Tijera", PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_CERRADO, PULGAR_VERT_PLANO,  MUNECA_CENTRO, CODO_MEDIO  },
};
const uint8_t NUM_JUGADAS_RPS = sizeof(JUGADAS_RPS) / sizeof(JUGADAS_RPS[0]);

const uint8_t MODO_POSES_ALEATORIAS = 0; // modo por defecto
const uint8_t MODO_PIEDRA_PAPEL_TIJERA = 1;
uint8_t modoMano = MODO_POSES_ALEATORIAS;

const uint16_t POSE_MANO_MIN_MS = 3000, POSE_MANO_MAX_MS = 7000;
const uint16_t RPS_MIN_MS = 4000, RPS_MAX_MS = 8000;
const uint8_t  MANO_TOTAL_PASOS = 30;
const uint8_t  MANO_RETARDO_PASO_MS = 20;

// Sacudida del brazo (arriba/abajo) antes de revelar la jugada de piedra, papel o tijera
const uint8_t  SACUDIDAS_CODO = 3;       // nº de veces que sube y baja el brazo
const uint8_t  SACUDIDA_PASOS = 10;      // pasos de interpolacion de cada subida/bajada
const uint8_t  SACUDIDA_RETARDO_MS = 18; // retardo entre pasos de la sacudida

PoseMano manoPoseActual = POSES_MANO[0], manoOrigen, manoDestino;
bool manoEnMovimiento = false;
unsigned long manoProximoEvento = 0, manoUltimoPasoMillis = 0;
uint8_t manoPasoActual = 0, manoIndiceAnterior = 0;

bool manoSacudiendo = false;
int manoSacudidaOrigen = 0, manoSacudidaDestino = 0;
uint8_t manoSacudidaPasoActual = 0, manoSacudidaContador = 0, manoIdxJugadaPendiente = 0;
unsigned long manoUltimoPasoSacudidaMillis = 0;

// ================================================================
// CUELLO - constantes y estado (movimiento lento y pequeño)
// ================================================================
const int NEUTRO = 90;
const int AMPLITUD_INCLINACION = 12;
const int AMPLITUD_ROTACION = 15;
const int INCLINACION_IZQ_ACTIVA = NEUTRO + AMPLITUD_INCLINACION;
const int INCLINACION_IZQ_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION / 2);
const int INCLINACION_DER_ACTIVA = NEUTRO + AMPLITUD_INCLINACION;
const int INCLINACION_DER_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION / 2);
const int ROTACION_IZQUIERDA = NEUTRO - AMPLITUD_ROTACION;
const int ROTACION_DERECHA = NEUTRO + AMPLITUD_ROTACION;

struct PoseCuello {
  const char* nombre;
  int inclinacionIzquierda, inclinacionDerecha, rotacion;
};

const PoseCuello POSES_CUELLO[] = {
  { "Centro",                       NEUTRO,                   NEUTRO,                   NEUTRO             },
  { "Inclinar poco a la izquierda", INCLINACION_IZQ_ACTIVA,   INCLINACION_DER_RELAJADA, NEUTRO             },
  { "Inclinar poco a la derecha",   INCLINACION_IZQ_RELAJADA, INCLINACION_DER_ACTIVA,   NEUTRO             },
  { "Girar poco a la izquierda",    NEUTRO,                   NEUTRO,                   ROTACION_IZQUIERDA },
  { "Girar poco a la derecha",      NEUTRO,                   NEUTRO,                   ROTACION_DERECHA   },
};
const uint8_t NUM_POSES_CUELLO = sizeof(POSES_CUELLO) / sizeof(POSES_CUELLO[0]);

const uint16_t CUELLO_MIN_MS = 5000, CUELLO_MAX_MS = 10000;
const uint16_t CUELLO_TOTAL_PASOS = 120;
const uint8_t  CUELLO_RETARDO_PASO_MS = 40;

PoseCuello cuelloPoseActual = POSES_CUELLO[0], cuelloOrigen, cuelloDestino;
bool cuelloEnMovimiento = false;
unsigned long cuelloProximoEvento = 0, cuelloUltimoPasoMillis = 0;
uint16_t cuelloPasoActual = 0;
uint8_t cuelloIndiceAnterior = 0;

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(9600);

  servoParpadoSupIzq.attach(PIN_PARPADO_SUP_IZQ);
  servoParpadoSupDer.attach(PIN_PARPADO_SUP_DER);
  servoParpadoInfIzq.attach(PIN_PARPADO_INF_IZQ);
  servoParpadoInfDer.attach(PIN_PARPADO_INF_DER);
  servoOjosVertical.attach(PIN_OJOS_VERTICAL);
  servoOjosHorizontal.attach(PIN_OJOS_HORIZONTAL);

  servoPulgarFlexion.attach(PIN_PULGAR_FLEXION);
  servoIndice.attach(PIN_INDICE);
  servoCorazon.attach(PIN_CORAZON);
  servoAnularMenique.attach(PIN_ANULAR_MENIQUE);
  servoPulgarVertical.attach(PIN_PULGAR_VERTICAL);
  servoMuneca.attach(PIN_MUNECA);
  servoCodo.attach(PIN_CODO);

  servoCuelloInclinacionIzq.attach(PIN_CUELLO_INCLINACION_IZQ);
  servoCuelloInclinacionDer.attach(PIN_CUELLO_INCLINACION_DER);
  servoCuelloRotacion.attach(PIN_CUELLO_ROTACION);

  randomSeed(analogRead(A0));

  Serial.println(F("=== Humanoide automatico iniciado (ojos + mano + cuello) ==="));
  Serial.println(F("Modo mano: escribe 0 (poses aleatorias) o 1 (piedra, papel o tijera)"));

  // Posiciones iniciales instantaneas
  abrirParpados();
  servoOjosVertical.write(OJOS_VERTICAL_CENTRO);
  servoOjosHorizontal.write(OJOS_HORIZONTAL_CENTRO);

  aplicarPoseManoInstantanea(POSES_MANO[0]);
  manoPoseActual = POSES_MANO[0];

  aplicarPoseCuelloInstantanea(POSES_CUELLO[0]);
  cuelloPoseActual = POSES_CUELLO[0];

  unsigned long ahora = millis();
  proximoParpadeo = ahora + random(PARPADEO_MIN_MS, PARPADEO_MAX_MS);
  proximaMirada = ahora + random(MIRADA_MIN_MS, MIRADA_MAX_MS);
  manoProximoEvento = ahora + random(POSE_MANO_MIN_MS, POSE_MANO_MAX_MS);
  cuelloProximoEvento = ahora + random(CUELLO_MIN_MS, CUELLO_MAX_MS);
}

// ================================================================
// LOOP PRINCIPAL - todo no bloqueante, todo funciona a la vez
// ================================================================
void loop() {
  leerCambioModoMano();

  actualizarParpadeo();
  actualizarMirada();
  actualizarMano();
  actualizarCuello();
}

// ================================================================
// OJOS
// ================================================================
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

// Parpadeo con maquina de estados (no bloqueante), con doble parpadeo ocasional
void actualizarParpadeo() {
  unsigned long ahora = millis();

  if (estadoParpadeo == PARPADEO_ESPERANDO) {
    if (ahora >= proximoParpadeo) {
      cerrarParpados();
      Serial.println(F("Ojos: parpadeo"));
      estadoParpadeo = PARPADEO_CERRADO;
      tiempoReaperturaParpados = ahora + PARPADEO_DURACION_MS;
    }
    return;
  }

  // estadoParpadeo == PARPADEO_CERRADO
  if (ahora >= tiempoReaperturaParpados) {
    abrirParpados();
    estadoParpadeo = PARPADEO_ESPERANDO;

    if (!dobleParpadeoPendiente && random(100) < PROBABILIDAD_DOBLE_PARPADEO) {
      dobleParpadeoPendiente = true;
      proximoParpadeo = ahora + 150; // segundo parpadeo casi inmediato
    } else {
      dobleParpadeoPendiente = false;
      proximoParpadeo = ahora + random(PARPADEO_MIN_MS, PARPADEO_MAX_MS);
    }
  }
}

// Movimiento de mirada interpolado y no bloqueante
void actualizarMirada() {
  unsigned long ahora = millis();

  if (!gazeEnMovimiento) {
    if (ahora >= proximaMirada) {
      gazeVerticalOrigen = gazeVerticalActual;
      gazeHorizontalOrigen = gazeHorizontalActual;
      gazeVerticalDestino = random(OJOS_VERTICAL_ARRIBA, OJOS_VERTICAL_ABAJO + 1);
      gazeHorizontalDestino = random(OJOS_HORIZONTAL_IZQUIERDA, OJOS_HORIZONTAL_DERECHA + 1);
      gazePasoActual = 0;
      gazeEnMovimiento = true;
      gazeUltimoPasoMillis = ahora;
    }
    return;
  }

  if (ahora - gazeUltimoPasoMillis >= GAZE_RETARDO_PASO_MS) {
    gazeUltimoPasoMillis = ahora;
    gazePasoActual++;
    servoOjosVertical.write(map(gazePasoActual, 0, GAZE_TOTAL_PASOS, gazeVerticalOrigen, gazeVerticalDestino));
    servoOjosHorizontal.write(map(gazePasoActual, 0, GAZE_TOTAL_PASOS, gazeHorizontalOrigen, gazeHorizontalDestino));

    if (gazePasoActual >= GAZE_TOTAL_PASOS) {
      gazeVerticalActual = gazeVerticalDestino;
      gazeHorizontalActual = gazeHorizontalDestino;
      gazeEnMovimiento = false;
      proximaMirada = ahora + random(MIRADA_MIN_MS, MIRADA_MAX_MS);
    }
  }
}

// ================================================================
// MANO + MUÑECA + CODO
// ================================================================
void aplicarPoseManoInstantanea(const PoseMano &pose) {
  servoPulgarFlexion.write(pose.pulgarFlexion);
  servoIndice.write(pose.indice);
  servoCorazon.write(pose.corazon);
  servoAnularMenique.write(pose.anularMenique);
  servoPulgarVertical.write(pose.pulgarVertical);
  servoMuneca.write(pose.muneca);
  servoCodo.write(pose.codo);
}

void imprimirPoseMano(const PoseMano &pose) {
  Serial.print(F("Mano - pose aplicada: "));
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

void iniciarMovimientoMano(const PoseMano &destino) {
  manoOrigen = manoPoseActual;
  manoDestino = destino;
  manoPasoActual = 0;
  manoEnMovimiento = true;
  manoUltimoPasoMillis = millis();
}

// Lee del Monitor Serie el modo de la mano (0 poses aleatorias, 1 piedra/papel/tijera)
void leerCambioModoMano() {
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

  modoMano = valor;
  Serial.print(F("Modo de mano cambiado a: "));
  Serial.println(modoMano == MODO_PIEDRA_PAPEL_TIJERA ? F("Piedra, papel o tijera") : F("Poses aleatorias"));

  unsigned long ahora = millis();
  manoProximoEvento = ahora + random(POSE_MANO_MIN_MS, POSE_MANO_MAX_MS);
}

void actualizarMano() {
  unsigned long ahora = millis();

  if (manoSacudiendo) {
    actualizarSacudidaBrazo();
    return;
  }

  if (!manoEnMovimiento) {
    if (ahora >= manoProximoEvento) {
      if (modoMano == MODO_PIEDRA_PAPEL_TIJERA) {
        uint8_t idx;
        do {
          idx = random(0, NUM_JUGADAS_RPS);
        } while (idx == manoIndiceAnterior && NUM_JUGADAS_RPS > 1);

        Serial.println(F("Mano preparando jugada..."));
        manoIdxJugadaPendiente = idx;
        manoSacudiendo = true;
        manoSacudidaContador = 0;
        manoSacudidaPasoActual = 0;
        manoSacudidaOrigen = manoPoseActual.codo;
        manoSacudidaDestino = CODO_ARRIBA;
        manoUltimoPasoSacudidaMillis = ahora;
      } else {
        uint8_t idx;
        do {
          idx = random(0, NUM_POSES_MANO);
        } while (idx == manoIndiceAnterior && NUM_POSES_MANO > 1);

        Serial.print(F("Mano - nueva pose seleccionada -> "));
        Serial.println(POSES_MANO[idx].nombre);
        iniciarMovimientoMano(POSES_MANO[idx]);
        manoIndiceAnterior = idx;
      }
    }
    return;
  }

  if (ahora - manoUltimoPasoMillis >= MANO_RETARDO_PASO_MS) {
    manoUltimoPasoMillis = ahora;
    manoPasoActual++;

    servoPulgarFlexion.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.pulgarFlexion, manoDestino.pulgarFlexion));
    servoIndice.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.indice, manoDestino.indice));
    servoCorazon.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.corazon, manoDestino.corazon));
    servoAnularMenique.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.anularMenique, manoDestino.anularMenique));
    servoPulgarVertical.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.pulgarVertical, manoDestino.pulgarVertical));
    servoMuneca.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.muneca, manoDestino.muneca));
    servoCodo.write(map(manoPasoActual, 0, MANO_TOTAL_PASOS, manoOrigen.codo, manoDestino.codo));

    if (manoPasoActual >= MANO_TOTAL_PASOS) {
      manoPoseActual = manoDestino;
      manoEnMovimiento = false;
      imprimirPoseMano(manoDestino);

      unsigned long espera = (modoMano == MODO_PIEDRA_PAPEL_TIJERA)
        ? random(RPS_MIN_MS, RPS_MAX_MS)
        : random(POSE_MANO_MIN_MS, POSE_MANO_MAX_MS);
      manoProximoEvento = ahora + espera;
    }
  }
}

// Sube y baja el codo (brazo) varias veces, sin bloquear el resto del humanoide, antes
// de revelar la jugada de piedra, papel o tijera (como al contar "...ya!")
void actualizarSacudidaBrazo() {
  unsigned long ahora = millis();
  if (ahora - manoUltimoPasoSacudidaMillis < SACUDIDA_RETARDO_MS) {
    return;
  }
  manoUltimoPasoSacudidaMillis = ahora;
  manoSacudidaPasoActual++;

  servoCodo.write(map(manoSacudidaPasoActual, 0, SACUDIDA_PASOS, manoSacudidaOrigen, manoSacudidaDestino));

  if (manoSacudidaPasoActual >= SACUDIDA_PASOS) {
    manoPoseActual.codo = manoSacudidaDestino;
    manoSacudidaPasoActual = 0;

    if (manoSacudidaDestino == CODO_ARRIBA) {
      manoSacudidaOrigen = CODO_ARRIBA;
      manoSacudidaDestino = CODO_MEDIO;
    } else {
      manoSacudidaContador++;
      if (manoSacudidaContador >= SACUDIDAS_CODO) {
        manoSacudiendo = false;
        Serial.print(F("Mano jugando... -> "));
        Serial.println(JUGADAS_RPS[manoIdxJugadaPendiente].nombre);
        iniciarMovimientoMano(JUGADAS_RPS[manoIdxJugadaPendiente]);
        manoIndiceAnterior = manoIdxJugadaPendiente;
      } else {
        manoSacudidaOrigen = CODO_MEDIO;
        manoSacudidaDestino = CODO_ARRIBA;
      }
    }
  }
}

// ================================================================
// CUELLO
// ================================================================
void aplicarPoseCuelloInstantanea(const PoseCuello &pose) {
  servoCuelloInclinacionIzq.write(pose.inclinacionIzquierda);
  servoCuelloInclinacionDer.write(pose.inclinacionDerecha);
  servoCuelloRotacion.write(pose.rotacion);
}

void imprimirPoseCuello(const PoseCuello &pose) {
  Serial.print(F("Cuello - movimiento aplicado: "));
  Serial.println(pose.nombre);
  Serial.print(F("  Inclinacion izquierda: ")); Serial.println(pose.inclinacionIzquierda);
  Serial.print(F("  Inclinacion derecha: ")); Serial.println(pose.inclinacionDerecha);
  Serial.print(F("  Rotacion: ")); Serial.println(pose.rotacion);
  Serial.println(F("----------------------------------"));
}

void actualizarCuello() {
  unsigned long ahora = millis();

  if (!cuelloEnMovimiento) {
    if (ahora >= cuelloProximoEvento) {
      uint8_t idx;
      do {
        idx = random(0, NUM_POSES_CUELLO);
      } while (idx == cuelloIndiceAnterior && NUM_POSES_CUELLO > 1);

      Serial.print(F("Cuello - nuevo movimiento -> "));
      Serial.println(POSES_CUELLO[idx].nombre);

      cuelloOrigen = cuelloPoseActual;
      cuelloDestino = POSES_CUELLO[idx];
      cuelloPasoActual = 0;
      cuelloEnMovimiento = true;
      cuelloUltimoPasoMillis = ahora;
      cuelloIndiceAnterior = idx;
    }
    return;
  }

  if (ahora - cuelloUltimoPasoMillis >= CUELLO_RETARDO_PASO_MS) {
    cuelloUltimoPasoMillis = ahora;
    cuelloPasoActual++;

    servoCuelloInclinacionIzq.write(map(cuelloPasoActual, 0, CUELLO_TOTAL_PASOS, cuelloOrigen.inclinacionIzquierda, cuelloDestino.inclinacionIzquierda));
    servoCuelloInclinacionDer.write(map(cuelloPasoActual, 0, CUELLO_TOTAL_PASOS, cuelloOrigen.inclinacionDerecha, cuelloDestino.inclinacionDerecha));
    servoCuelloRotacion.write(map(cuelloPasoActual, 0, CUELLO_TOTAL_PASOS, cuelloOrigen.rotacion, cuelloDestino.rotacion));

    if (cuelloPasoActual >= CUELLO_TOTAL_PASOS) {
      cuelloPoseActual = cuelloDestino;
      cuelloEnMovimiento = false;
      imprimirPoseCuello(cuelloDestino);
      cuelloProximoEvento = ahora + random(CUELLO_MIN_MS, CUELLO_MAX_MS);
    }
  }
}
