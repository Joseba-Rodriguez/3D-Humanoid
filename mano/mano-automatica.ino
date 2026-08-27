/*
  mano-automatica.ino
  Control automático de la mano animatrónica (humanoide 3D) - Arduino Mega 2560.
  5 servos de dedos que van formando poses ("formas") de la mano de manera
  automática y cíclica, con movimiento suave entre cada pose.

  Tabla de conexionado:
  Motor -> Pin digital -> Función
  1     -> 30           -> Pulgar, flexión (recto/extendido <-> flexionado hacia la palma)
  2     -> 31            -> Índice
  3     -> 32            -> Corazón
  4     -> 33            -> Anular + meñique (juntos, un solo servo mueve ambos dedos)
  5     -> 34            -> Pulgar, eje vertical / oposición (debe ir sincronizado con el motor 1)
  6     -> 35            -> Muñeca, giro izquierda/derecha
  16    -> 45            -> Codo, movimiento arriba/abajo

  CONVENCIÓN DE ÁNGULOS (0-180°, igual que en flex-mano/mano-felx.ino):
  - Dedos (índice, corazón, anular+meñique): 0° = dedo cerrado/flexionado, 180° = dedo abierto/recto.
  - Pulgar flexión (motor 1): 0° = pulgar flexionado hacia la palma, 180° = pulgar recto/extendido.
  - Pulgar vertical/oposición (motor 5): 0° = pulgar plano, pegado al plano de la mano,
    180° = pulgar levantado en posición vertical (separado, como en un "OK" o un "like").
  - Muñeca (motor 6): 0° = girada del todo a la izquierda, 180° = girada del todo a la derecha.
  - Codo (motor 16): 0° = codo totalmente abajo (brazo extendido), 180° = codo totalmente arriba (flexionado).

  IMPORTANTE: Estos ángulos son de partida. Ajusta las poses en POSES[] según lo que
  veas físicamente en tu mano/brazo una vez montada (evita forzar los servos contra el tope mecánico).

  No modifica ni depende de la carpeta flex-mano (esa mano se controla por sensores flex).
*/

#include <Servo.h>

// ---------- Pines ----------
const uint8_t PIN_PULGAR_FLEXION = 30;
const uint8_t PIN_INDICE         = 31;
const uint8_t PIN_CORAZON        = 32;
const uint8_t PIN_ANULAR_MENIQUE = 33;
const uint8_t PIN_PULGAR_VERTICAL = 34;
const uint8_t PIN_MUNECA          = 35;
const uint8_t PIN_CODO            = 45;

const uint8_t NUM_SERVOS = 7;

// ---------- Objetos Servo ----------
Servo servoPulgarFlexion;
Servo servoIndice;
Servo servoCorazon;
Servo servoAnularMenique;
Servo servoPulgarVertical;
Servo servoMuneca;
Servo servoCodo;

// ---------- Ángulos de referencia ----------
const int DEDO_CERRADO = 0;
const int DEDO_ABIERTO = 180;
const int DEDO_MEDIO   = 90;

const int PULGAR_FLEX_CERRADO = 0;
const int PULGAR_FLEX_ABIERTO = 180;
const int PULGAR_FLEX_MEDIO   = 90;

const int PULGAR_VERT_PLANO   = 0;
const int PULGAR_VERT_ARRIBA  = 180;
const int PULGAR_VERT_MEDIO   = 90;

const int MUNECA_IZQUIERDA = 0;
const int MUNECA_CENTRO    = 90;
const int MUNECA_DERECHA   = 180;

const int CODO_ABAJO = 0;
const int CODO_MEDIO = 90;
const int CODO_ARRIBA = 180;

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

// Cada pose define el ángulo de los 7 servos (dedos + muñeca + codo). Añade o ajusta poses libremente.
const PoseMano POSES[] = {
  { "Mano relajada",     PULGAR_FLEX_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    PULGAR_VERT_MEDIO,  MUNECA_CENTRO,    CODO_MEDIO   },
  { "Puno cerrado",      PULGAR_FLEX_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_CENTRO,    CODO_ARRIBA  },
  { "Mano abierta",      PULGAR_FLEX_ABIERTO,  DEDO_ABIERTO,  DEDO_ABIERTO,  DEDO_ABIERTO,  PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ABAJO   },
  { "Senal de paz",      PULGAR_FLEX_CERRADO,  DEDO_ABIERTO,  DEDO_ABIERTO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_DERECHA,   CODO_MEDIO   },
  { "Pulgar arriba",     PULGAR_FLEX_ABIERTO,  DEDO_CERRADO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_ARRIBA, MUNECA_CENTRO,    CODO_ARRIBA  },
  { "Senalar",           PULGAR_FLEX_CERRADO,  DEDO_ABIERTO,  DEDO_CERRADO,  DEDO_CERRADO,  PULGAR_VERT_PLANO,  MUNECA_IZQUIERDA, CODO_ABAJO   },
  { "OK",                PULGAR_FLEX_MEDIO,    DEDO_MEDIO,    DEDO_ABIERTO,  DEDO_ABIERTO,  PULGAR_VERT_MEDIO,  MUNECA_CENTRO,    CODO_MEDIO   },
  { "Garra",             PULGAR_FLEX_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    DEDO_MEDIO,    PULGAR_VERT_MEDIO,  MUNECA_DERECHA,   CODO_ARRIBA  },
};
const uint8_t NUM_POSES = sizeof(POSES) / sizeof(POSES[0]);

// ---------- Temporización ----------
const uint16_t TIEMPO_MIN_ENTRE_POSES_MS = 3000;
const uint16_t TIEMPO_MAX_ENTRE_POSES_MS = 7000;
const uint8_t  PASOS_MOVIMIENTO_SUAVE = 30; // nº de pasos para interpolar entre poses
const uint8_t  RETARDO_PASO_MS = 20;        // retardo entre pasos de interpolación

// ---------- Estado ----------
PoseMano poseActual = POSES[0];
unsigned long proximoCambioPose = 0;
uint8_t indicePoseAnterior = 0;

void setup() {
  Serial.begin(9600);

  servoPulgarFlexion.attach(PIN_PULGAR_FLEXION);
  servoIndice.attach(PIN_INDICE);
  servoCorazon.attach(PIN_CORAZON);
  servoAnularMenique.attach(PIN_ANULAR_MENIQUE);
  servoPulgarVertical.attach(PIN_PULGAR_VERTICAL);
  servoMuneca.attach(PIN_MUNECA);
  servoCodo.attach(PIN_CODO);

  randomSeed(analogRead(A0));

  Serial.println(F("=== Mano automatica iniciada ==="));
  aplicarPoseInstantanea(POSES[0]);
  poseActual = POSES[0];
  imprimirPose(POSES[0]);

  proximoCambioPose = millis() + random(TIEMPO_MIN_ENTRE_POSES_MS, TIEMPO_MAX_ENTRE_POSES_MS);
}

void loop() {
  if (millis() >= proximoCambioPose) {
    uint8_t indiceNuevaPose = elegirPoseDistinta(indicePoseAnterior);
    Serial.print(F("Nueva pose seleccionada -> "));
    Serial.println(POSES[indiceNuevaPose].nombre);

    moverAPoseSuave(POSES[indiceNuevaPose]);
    poseActual = POSES[indiceNuevaPose];
    indicePoseAnterior = indiceNuevaPose;

    imprimirPose(POSES[indiceNuevaPose]);

    proximoCambioPose = millis() + random(TIEMPO_MIN_ENTRE_POSES_MS, TIEMPO_MAX_ENTRE_POSES_MS);
  }
}

// Imprime por el Monitor Serie el nombre de la pose y el angulo aplicado a cada servo
void imprimirPose(const PoseMano &pose) {
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

// Elige un índice de pose aleatorio distinto al anterior, para no repetir la misma dos veces seguidas
uint8_t elegirPoseDistinta(uint8_t indiceAnterior) {
  uint8_t indiceNuevo;
  do {
    indiceNuevo = random(0, NUM_POSES);
  } while (indiceNuevo == indiceAnterior && NUM_POSES > 1);
  return indiceNuevo;
}

// Aplica una pose directamente, sin interpolación (uso solo en el arranque)
void aplicarPoseInstantanea(const PoseMano &pose) {
  servoPulgarFlexion.write(pose.pulgarFlexion);
  servoIndice.write(pose.indice);
  servoCorazon.write(pose.corazon);
  servoAnularMenique.write(pose.anularMenique);
  servoPulgarVertical.write(pose.pulgarVertical);
  servoMuneca.write(pose.muneca);
  servoCodo.write(pose.codo);
}

// Interpola los 7 servos desde la pose actual hasta la pose destino, moviéndolos
// todos a la vez (pulgar, muñeca y codo quedan sincronizados con el resto de dedos)
void moverAPoseSuave(const PoseMano &destino) {
  PoseMano origen = poseActual;

  for (uint8_t paso = 1; paso <= PASOS_MOVIMIENTO_SUAVE; paso++) {
    servoPulgarFlexion.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.pulgarFlexion, destino.pulgarFlexion));
    servoIndice.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.indice, destino.indice));
    servoCorazon.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.corazon, destino.corazon));
    servoAnularMenique.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.anularMenique, destino.anularMenique));
    servoPulgarVertical.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.pulgarVertical, destino.pulgarVertical));
    servoMuneca.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.muneca, destino.muneca));
    servoCodo.write(map(paso, 0, PASOS_MOVIMIENTO_SUAVE, origen.codo, destino.codo));
    delay(RETARDO_PASO_MS);
  }
}
