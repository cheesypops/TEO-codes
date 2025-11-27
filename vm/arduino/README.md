# Carro Evita-Líneas (PlatformIO ESP32)

Proyecto de un carro evita-líneas para ESP32 usando L298N y sensores IR.

Estructura:

- `src/main.ino` : código principal
- `platformio.ini` : configuración PlatformIO

Compilar y subir (PlatformIO):

```bash
platformio run --environment esp32doit-devkit-v1
platformio run --target upload --environment esp32doit-devkit-v1 --upload-port /dev/cu.usbserial-XXXX
```

Notas:

- Asegúrate de conectar correctamente ENA/ENB e IN1..IN4 al L298N.
- Ajusta `velocidadBase` y `velocidadGiro` en `src/main.ino` según tu chasis.
