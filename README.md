# 3D-Humanoid

Este proyecto implementa el control de una mano robótica utilizando servomotores para simular movimientos naturales de los dedos. El código permite realizar gestos predefinidos y movimientos aleatorios, proporcionando una base para aplicaciones en robótica y animatrónica.

## Características

- **Control individual de los dedos:** Cada dedo está controlado por un servomotor, con la excepción del anular y meñique, que comparten un servo.
- **Gestos predefinidos:** Incluye funciones para abrir la mano, cerrarla, hacer un gesto de "OK", y flexionar los dedos.
- **Movimientos aleatorios:** Genera movimientos aleatorios para simular comportamientos más naturales.
- **Configuración de intervalos:** Los movimientos se realizan en intervalos de tiempo aleatorios para mayor realismo.

## Requisitos

- **Hardware:**
    - 4 servomotores (uno para cada dedo o grupo de dedos).
    - Microcontrolador compatible con Arduino.
    - Fuente de alimentación adecuada para los servos.
- **Software:**
    - Arduino IDE.
    - Biblioteca `Servo.h` (incluida por defecto en el IDE de Arduino).

## Funciones principales

### `abrirMano()`
Abre la mano lentamente moviendo todos los dedos a su posición máxima.

### `cerrarMano()`
Cierra la mano lentamente moviendo todos los dedos a su posición mínima.

### `hacerGestoOK()`
Realiza un gesto de "OK" moviendo el pulgar y el índice a una posición específica mientras los demás dedos permanecen relajados.

### `flexionarDedos()`
Simula la flexión de los dedos de forma individual, con movimientos aleatorios para cada uno.

### `hacerGestosAleatorios()`
Genera un gesto aleatorio moviendo cada dedo a un ángulo aleatorio dentro de un rango predefinido.

## Configuración inicial

1. Conecta los servomotores a los pines digitales del microcontrolador:
     - Pulgar: Pin 6.
     - Índice: Pin 7.
     - Medio: Pin 10.
     - Anular + Meñique: Pin 9.
2. Sube el código al microcontrolador utilizando el Arduino IDE.
3. Alimenta los servos y el microcontrolador.
4. Observa los movimientos y ajusta los parámetros si es necesario.

## Personalización

- **Ángulos de movimiento:** Puedes ajustar los rangos de los ángulos en las funciones `getRandomAngle()` y en las funciones de los gestos.
- **Intervalos de tiempo:** Modifica los valores en `getRandomDelay()` y `intervalo` para cambiar la duración de los movimientos y los tiempos de espera.
- **Nuevos gestos:** Agrega nuevas funciones para implementar gestos personalizados.

## Notas

- Asegúrate de que los servos estén correctamente calibrados para evitar daños mecánicos.
- Si los movimientos no son suaves, verifica la fuente de alimentación y los valores de retardo en el código.

Este proyecto es ideal para aprender sobre el control de servomotores y la programación de movimientos robóticos.