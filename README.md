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
   idf.py add-dependency "espressif/usb_host_hid"
4. Conecta tu ESP32-S3 (modelo con soporte USB OTG) y compila/fluye el firmware:
   ```bash
   idf.py build
   idf.py flash

### 2) Emisor (Arduino IDE)
1. Abre la carpeta `emisor/` en Arduino IDE.
2. Selecciona la placa “ESP32-S3 Dev Module” o el modelo exacto que uses. 
3. Configura el puerto correcto y sube el sketch directamente al dispositivo.

### 3) Colector (Python)
1. Asegurate de tener Python 3.8 o superior instalado.
2. Abre la carpeta `Colector/`.
   ```bash
   cd colector
4. Crea y activa tu entorno virtual:
   ```bash
   python3 -m venv env
   source env/bin/activate
5. Instala las dependencias:
   ```bash
   pip install -r requirements.txt
6. Ejecuta el colector:
   ```bash
   python3 keylogger.py

---

## Aviso importante - Uso responsable
Este proyecto es **exclusivamente** para fines educativos, pruebas en laboratorio y auditoria interna.  
NO debe usarse para espiar, monitorizar o capturar datos en equipos o redes sin consentimiento expreso del propietario. El autor no se responsabiliza por el uso indebido. Antes de ejecutar cualquier codigo, asegurate de tener permiso y de cumplir la ley local y las politicas de tu organizacion.

---
