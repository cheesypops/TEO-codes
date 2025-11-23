## Tests de codegen

Esta carpeta contiene programas de prueba centrados en la generación de bytecode
(`.vmcode`).

- `01_programa_completo.txt`: prueba un programa que combina:
  - declaraciones globales,
  - expresiones aritméticas y lógicas,
  - `if` / `else`,
  - bucles `while`, `for` y `do-while`,
  - uso de todas las funciones reservadas relevantes:
    - `mover(int)`, `girarIzq(int)`, `girarDer(int)`, `reversa(int)`,
    - `leerSensor()`, `parar()`.

### Cómo ejecutar estos tests

Desde la raíz del proyecto:

```bash
make
./mi_compilador tests/codegen/01_programa_completo.txt
cat program.vmcode
```

Cada ejecución sobrescribe `program.vmcode` con el bytecode lineal generado para
el programa correspondiente.
