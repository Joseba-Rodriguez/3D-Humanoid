#include <Servo.h>

// Servos de la mano
Servo pulgar;        // Pulgar
Servo indice;        // Índice
Servo medio;         // Medio
Servo anularMenique; // Anular + Meñique

// Servos del brazo
Servo brazo;   // Brazo (parte media del brazo robótico)
Servo muneca;  // Muñeca (final del brazo robótico)

// Posiciones actuales
int pulgarPos = 90;
int indicePos = 90;
int medioPos = 90;
int anularMeniquePos = 90;
int brazoPos = 90;

// Tiempo y control aleatorio
unsigned long tiempoAnterior = 0;
unsigned long intervalo = 2000;

int getRandomAngle() {
  return random(45, 135);
}

int getRandomDelay() {
  return random(500, 1500);
}

void setup() {
  // Conectar servos de la mano
  pulgar.attach(6);
  indice.attach(7);
  medio.attach(10);
  anularMenique.attach(9);

  // Conectar servos del brazo
  brazo.attach(8);     // Supón que lo conectaste al pin 8
  muneca.attach(11);   // Supón que lo conectaste al pin 11

  // Inicialización
  Serial.begin(9600);
  abrirMano();
  moverBrazo(90);  // Inicia el brazo y muñeca en centro
  delay(1000);
}

void loop() {
  unsigned long tiempoActual = millis();

  if (tiempoActual - tiempoAnterior >= intervalo) {
    tiempoAnterior = tiempoActual;
    
    int accion = random(1, 6);
    
    switch (accion) {
      case 1:
        abrirMano();
        break;
      case 2:
        cerrarMano();
        break;
      case 3:
        hacerGestoPersonalizado(160, 160, 90, 90); // Gesto OK
        break;
      case 4:
        flexionarDedos();
        break;
      case 5:
        moverBrazo(random(60, 120));  // Mueve brazo y muñeca coordinadamente
        break;
    }

    intervalo = random(1000, 3000);
  }
}

// Sincroniza movimiento del brazo y la muñeca
void moverBrazo(int nuevoAnguloBrazo) {
  Serial.print("Moviendo brazo a ");
  Serial.print(nuevoAnguloBrazo);
  Serial.println("°");

  int anguloActual = brazoPos;
  int paso = (nuevoAnguloBrazo > anguloActual) ? 1 : -1;

  for (int i = anguloActual; i != nuevoAnguloBrazo; i += paso) {
    brazo.write(i);

    // Sincronización: la muñeca acompaña, invertida o compensada
    int anguloMuneca = constrain(map(i, 45, 135, 135, 45), 45, 135);
    muneca.write(anguloMuneca);

    delay(15);
  }

  brazoPos = nuevoAnguloBrazo;
  delay(getRandomDelay());
}

void abrirMano() {
  Serial.println("Abriendo la mano...");
  for (int i = 45; i <= 135; i++) {
    pulgar.write(i);
    indice.write(i);
    medio.write(i);
    anularMenique.write(i);
    delay(random(10, 20));
  }
  delay(getRandomDelay());
}

void cerrarMano() {
  Serial.println("Cerrando la mano...");
  for (int i = 135; i >= 45; i--) {
    pulgar.write(i);
    indice.write(i);
    medio.write(i);
    anularMenique.write(i);
    delay(random(10, 20));
  }
  delay(getRandomDelay());
}

void flexionarDedos() {
  Serial.println("Flexionando los dedos...");
  pulgar.write(getRandomAngle()); delay(random(500, 1000));
  indice.write(getRandomAngle()); delay(random(500, 1000));
  medio.write(getRandomAngle()); delay(random(500, 1000));
  anularMenique.write(getRandomAngle()); delay(random(500, 1000));
  abrirMano();
}

void hacerGestoPersonalizado(int angPulgar, int angIndice, int angMedio, int angAnularMenique) {
  Serial.println("Haciendo gesto personalizado...");
  pulgar.write(angPulgar);
  indice.write(angIndice);
  medio.write(angMedio);
  anularMenique.write(angAnularMenique);
  delay(getRandomDelay());
}
