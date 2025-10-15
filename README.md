# esp32-keylogger

> Proyecto educativo y de laboratorio: demostrador de captura y reenvio de eventos de teclado usando 2 ESP32 S3 y un servicio colector en Python.  
> Estructura pensada para aprendizaje, pruebas controladas y auditoria interna.

---

## Compilar y ejecutar

### 1) Receptor (ESP-IDF)
1. Instala ESP-IDF y sigue la guia oficial para configurar el entorno.  
2. Copia la carpeta `receptor/` a tu workspace IDF.  
3. Agrega la dependencia HID (desde la raiz del proyecto receptor):
   ```bash
   cd receptor
   idf.py add-dependency "espressif/usb_host_hid"
---

## Aviso importante - Uso responsable
Este proyecto es **exclusivamente** para fines educativos, pruebas en laboratorio y auditoria interna.  
NO debe usarse para espiar, monitorizar o capturar datos en equipos o redes sin consentimiento expreso del propietario. El autor no se responsabiliza por el uso indebido. Antes de ejecutar cualquier codigo, asegurate de tener permiso y de cumplir la ley local y las politicas de tu organizacion.

---

## Estructura del repositorio
- `receptor/`  -> codigo en ESP-IDF (host USB HID que lee el teclado y transmite por UART).
- `emisor/`    -> codigo en Arduino IDE (emula el teclado hacia la PC y reenvia copias al colector).
- `colector/`  -> servidor en Python (Flask). Recibe /key POST y registra las teclas.

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
