# esp32-keylogger

> Proyecto educativo y de laboratorio: demostrador de captura y reenvio de eventos de teclado usando 2 ESP32-S3 y un bot de Telegram.  
> Diseño pensado para aprendizaje, pruebas controladas y auditoría interna en entornos autorizados.
<p align="center">
  <img src="https://github.com/user-attachments/assets/8517655d-2b2a-4c60-b36f-e852813cddd9" width="400">
</p>

---

## Arquitectura del proyecto

El sistema captura los eventos HID del teclado real mediante un **ESP32-S3 receptor**, los reenvía por serial cruzado al **ESP32-S3 emisor**, y este finalmente envía las pulsaciones a un **bot de Telegram**, sin necesidad de servidor propio.

**Componentes del sistema:**
- ESP32-S3 Receptor → Captura HID por USB-OTG.  
- ESP32-S3 Emisor → Recibe los datos por UART y los envía a Telegram.  
- Bot de Telegram → Recibe los logs en tiempo real.

---

## Diagrama de conexión
![mmmm](https://github.com/user-attachments/assets/7c2f380b-b254-454c-b580-6182f637d58e)

---

## Diagrama de cableado entre ESP32-S3

Para la comunicación entre el **receptor** y el **emisor**, se usa una conexión serial cruzada entre los GPIO:

| Receptor (ESP32-S3) | Emisor (ESP32-S3) |
|----------------------|-------------------|
| GPIO17 (TX)          | GPIO18 (RX)       |
| GPIO18 (RX)          | GPIO17 (TX)       |
| GND                  | GND               |
<img width="1107" height="815" alt="image" src="https://github.com/user-attachments/assets/128efb56-557a-468a-b991-7b15de718025" />

---

## Configuración del bot de Telegram

1. Abre Telegram y busca **@BotFather**.  
2. Ejecuta `/newbot` y sigue los pasos.  
3. Copia el **token del bot**.  
4. En el ESP32-S3 *emisor*, configura:
   - Token del bot  
   - ID de chat (obténlo hablando con `@userinfobot`)

El emisor enviará cada tecla capturada al chat configurado.

---

## Compilar y ejecutar

### 1) Receptor (ESP-IDF)
1. Instala ESP-IDF y sigue la guia oficial para configurar el entorno.  
2. Copia la carpeta `receptor/` a tu workspace IDF.  
3. Agrega la dependencia HID (desde la raiz del proyecto receptor):
   ```bash
   cd receptor
   idf.py add-dependency "espressif/usb_host_hid"
4. Conecta tu ESP32-S3 (modelo con soporte USB OTG) y compila/fluye el firmware:
   ```bash
   idf.py build
   idf.py flash

### 2) Emisor (Arduino IDE)
1. Abre la carpeta `emisor/` en Arduino IDE.
2. Selecciona la placa “ESP32-S3 Dev Module” o el modelo exacto que uses. 
3. Configura el puerto correcto y sube el sketch directamente al dispositivo.
   - **SSID** y **password** de tu red WiFi  
   - **Token del bot** de Telegram  
   - **Chat ID** donde recibirás los mensajes
4. Sube el sketch al dispositivo.
---

## Video demostrativo
<p align="center">
  <a href="https://www.youtube.com/watch?v=8ONeW38sM1c">
    <img src="https://img.youtube.com/vi/8ONeW38sM1c/0.jpg" width="600">
  </a>
</p>
---

## Aviso importante - Uso responsable
Este proyecto es **exclusivamente** para fines educativos, pruebas en laboratorio y auditoría interna.
**No debe usarse** para espiar, monitorizar o capturar datos en equipos o redes sin consentimiento expreso del propietario.
El autor **no se responsabiliza** por el uso indebido.
Antes de ejecutar cualquier código, asegúrate de tener permiso y de cumplir la ley local y las políticas de tu organización.
