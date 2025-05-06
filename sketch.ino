#include <Servo.h>

// Definir los servos para cada dedo
Servo pulgar;        // Pulgar
Servo indice;        // Índice
Servo medio;         // Medio
Servo anularMenique; // Anular + Meñique (compartido)

// Variables de ángulos
int pulgarPos = 90;
int indicePos = 90;
int medioPos = 90;
int anularMeniquePos = 90;

// Configuración de la duración de los movimientos (en milisegundos)
unsigned long tiempoAnterior = 0;
unsigned long intervalo = 2000; // Inicialmente en 2 segundos (variará)

// Funciones para generar movimientos aleatorios
int getRandomAngle() {
  return random(45, 135);  // Rango de 45° a 135° para movimientos naturales
}

int getRandomDelay() {
  return random(500, 1500); // Tiempo entre 0.5s y 1.5s para los movimientos
}

void setup() {
  // Conectar los servos a los pines correspondientes
  pulgar.attach(6);
  indice.attach(7);
  medio.attach(10);
  anularMenique.attach(9);
  
  // Inicializamos el puerto serial para monitorear
  Serial.begin(9600);
  
  // Asegurar que la mano comienza en una posición neutral
  abrirMano();
  delay(1000);
}

void loop() {
  unsigned long tiempoActual = millis();

  // Revisamos si ha pasado el tiempo para el siguiente movimiento aleatorio
  if (tiempoActual - tiempoAnterior >= intervalo) {
    // Actualizamos el tiempo
    tiempoAnterior = tiempoActual;
    
    // Elegimos aleatoriamente una acción
    int accion = random(1, 6);
    
    switch (accion) {
      case 1:
        abrirMano();
        break;
        
      case 2:
        cerrarMano();
        break;
        
      case 3:
        hacerGestoOK();
        break;
        
      case 4:
        flexionarDedos();
        break;
        
      case 5:
        hacerGestosAleatorios();
        break;
    }
    
    // Establecer un nuevo intervalo aleatorio para el siguiente movimiento
    intervalo = random(1000, 3000); // Cambiar el tiempo de espera entre acciones
  }
}

// Función para abrir la mano lentamente
void abrirMano() {
  Serial.println("Abriendo la mano...");
  for (int i = pulgarPos; i <= 180; i++) {
    pulgar.write(i);
    indice.write(i);
    medio.write(i);
    anularMenique.write(i);
    delay(random(10, 30)); // Retardo aleatorio entre cada incremento
  }
  delay(getRandomDelay()); // Esperar un tiempo aleatorio antes de continuar
}

// Función para cerrar la mano lentamente
void cerrarMano() {
  Serial.println("Cerrando la mano...");
  for (int i = pulgarPos; i >= 45; i--) {
    pulgar.write(i);
    indice.write(i);
    medio.write(i);
    anularMenique.write(i);
    delay(random(10, 30)); // Retardo aleatorio entre cada incremento
  }
  delay(getRandomDelay()); // Esperar un tiempo aleatorio antes de continuar
}

// Función para hacer un gesto de "OK"
void hacerGestoOK() {
  Serial.println("Haciendo gesto OK...");
  pulgar.write(160);
  indice.write(160);
  medio.write(90);
  anularMenique.write(90);
  delay(getRandomDelay());
  
  abrirMano(); // Volver a abrir la mano
}

// Función para simular la flexión de los dedos
void flexionarDedos() {
  Serial.println("Flexionando los dedos...");
  
  // Flexionar el pulgar
  pulgar.write(getRandomAngle());
  delay(random(500, 1000));
  
  // Flexionar el índice
  indice.write(getRandomAngle());
  delay(random(500, 1000));
  
  // Flexionar el medio
  medio.write(getRandomAngle());
  delay(random(500, 1000));
  
  // Flexionar el anular + meñique
  anularMenique.write(getRandomAngle());
  delay(random(500, 1000));
  
  // Volver a abrir la mano después de flexionar
  abrirMano();
}

// Función para hacer un gesto aleatorio con la mano
void hacerGestosAleatorios() {
  Serial.println("Haciendo gesto aleatorio...");
  
  // Gesto aleatorio para cada dedo
  pulgar.write(getRandomAngle());
  indice.write(getRandomAngle());
  medio.write(getRandomAngle());
  anularMenique.write(getRandomAngle());
  
  delay(getRandomDelay());  // Esperar un tiempo aleatorio
}
