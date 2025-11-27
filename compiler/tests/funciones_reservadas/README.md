## Tests de funciones reservadas con argumentos

Esta carpeta contiene pruebas específicas para las funciones reservadas del
robot que ahora aceptan argumentos opcionales desde el lenguaje fuente.

- `ok_mover_girar_reversa.txt`: casos válidos de `mover`, `girarIzq`, `girarDer`
  y `reversa` con 0, 1 y 2 argumentos numéricos.
- `ok_esperar_y_leerSensor.txt`: casos válidos de `esperar(ms)` y
  `leerSensor()`, integrados en un pequeño bucle.
- `error_mover_girar_reversa.txt`: pruebas negativas de cantidad y tipo de
  argumentos para las funciones de movimiento.
- `error_esperar_y_leerSensor.txt`: pruebas negativas de cantidad y tipo de
  argumentos para `esperar` y de uso incorrecto de `leerSensor`.

### Cómo ejecutar los tests

1. Compilar el compilador (si no está ya compilado) de acuerdo al flujo habitual
   del proyecto (por ejemplo, usando `make` en la carpeta `src`).
2. Desde la raíz del proyecto, ejecutar el compilador sobre cada archivo:

```bash
cd compiler/tests/funciones_reservadas
../../build/bin/mi_compilador ok_mover_girar_reversa.txt
../../build/bin/mi_compilador ok_esperar_y_leerSensor.txt
../../build/bin/mi_compilador error_mover_girar_reversa.txt
../../build/bin/mi_compilador error_esperar_y_leerSensor.txt
```

- En los archivos `ok_*` **no** deberían aparecer errores semánticos y se
  debería generar un archivo `codigo.txt` válido.
- En los archivos `error_*` **sí** deberían aparecer mensajes de error semántico
  descriptivos en la salida de error estándar y no debería generarse código
  útil.

Estos tests permiten verificar de forma manual y exhaustiva que la validación
semántica, la generación de bytecode y la convención de paso de argumentos a la
máquina virtual se comportan como se espera.
