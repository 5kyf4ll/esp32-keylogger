# esp32-keylogger

> Proyecto educativo y de laboratorio: demostrador de captura y reenvio de eventos de teclado usando 2 ESP32 y un servicio colector en Python.  
> Estructura pensada para aprendizaje, pruebas controladas y auditoria interna.

---

## Estructura del repositorio
- `receptor/`  -> codigo en ESP-IDF. Este dispositivo actua como host USB HID, recibe las pulsaciones del teclado y las envia por serial/WiFi al emisor.
- `emisor/`    -> codigo en Arduino IDE. Recibe las pulsaciones desde el receptor y las emula hacia la computadora objetivo; tambien reenvia una copia al colector.
- `colector/`  -> servidor en Python (Flask). Recibe y registra las teclas, muestra reconstruccion en consola y guarda en archivo rotativo.

---

## Propósito
Este repo es un proyecto **educativo** para:
- entender como funcionan HID y USB host en ESP32,
- practicar comunicacion entre microcontroladores,
- aprender buenas practicas de logging y privacidad en proyectos de seguridad.

**NO** es para espiar ni para uso en equipos ajenos sin consentimiento. Lee la seccion "uso responsable" antes de ejecutar nada.

---

## Componentes y notas rapidas

### Receptor (ESP-IDF)
- Framework: **ESP-IDF** (version recomendada en tu entorno).
- Dependencia importante: `usb_host_hid` (comando para agregar en tu proyecto IDF):
  ```bash
  idf.py add-dependency "espressif/usb_host_hid"
