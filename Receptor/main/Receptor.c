#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"

// Configuración de la UART: pines TX/RX, puerto y tamaño de buffer
#define TX_PIN 17
#define RX_PIN 18
#define UART_PORT UART_NUM_2
#define BUF_SIZE 256

// Definiciones de teclas HID básicas
#define HID_KEY_ENTER       0x28
#define HID_KEY_ESCAPE      0x29
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_TAB         0x2B
#define HID_KEY_CAPS_LOCK   0x39

// Combinaciones Ctrl + C/V/X/Z
#define CMD_CTRL_C 0xA1
#define CMD_CTRL_V 0xA2
#define CMD_CTRL_X 0xA3
#define CMD_CTRL_Z 0xA4

// Comandos especiales no ASCII
#define CMD_ESC   0x1B
#define CMD_TAB   0x09
#define CMD_CAPS  0x14
#define CMD_GUI   0x90

// Flechas de dirección
#define CMD_ARROW_UP    0xA5
#define CMD_ARROW_DOWN  0xA6
#define CMD_ARROW_LEFT  0xA7
#define CMD_ARROW_RIGHT 0xA8

// Teclado numérico (numpad)
#define HID_KEY_KP_DIV     0x54
#define HID_KEY_KP_MULT    0x55
#define HID_KEY_KP_MINUS   0x56
#define HID_KEY_KP_PLUS    0x57
#define HID_KEY_KP_ENTER   0x58
#define HID_KEY_KP_1       0x59
#define HID_KEY_KP_2       0x5A
#define HID_KEY_KP_3       0x5B
#define HID_KEY_KP_4       0x5C
#define HID_KEY_KP_5       0x5D
#define HID_KEY_KP_6       0x5E
#define HID_KEY_KP_7       0x5F
#define HID_KEY_KP_8       0x60
#define HID_KEY_KP_9       0x61
#define HID_KEY_KP_0       0x62
#define HID_KEY_KP_DOT     0x63
#define HID_KEY_KP_EQUAL   0x67
#define HID_KEY_NUM_LOCK   0x53

// Teclas de función F1–F12
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

// Cola para mantener el orden de los caracteres enviados por UART
static QueueHandle_t uart_queue;

// Estructura de mapeo de caracteres AltGr
typedef struct {
    uint8_t keycode;
    char symbol;
} altgr_map_t;

// Tabla de AltGr
static const altgr_map_t altgr_table[] = {
    {HID_KEY_Q, '@'},
    // Se pueden agregar más símbolos según el layout
};
static const int altgr_table_size = sizeof(altgr_table) / sizeof(altgr_map_t);

// Tabla básica de mapeo HID a ASCII (sin AltGr)
static const uint8_t keycode2ascii[57][2] = {
    {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {'a','A'},{'b','B'},{'c','C'},{'d','D'},{'e','E'},{'f','F'},
    {'g','G'},{'h','H'},{'i','I'},{'j','J'},{'k','K'},{'l','L'},
    {'m','M'},{'n','N'},{'o','O'},{'p','P'},{'q','Q'},{'r','R'},
    {'s','S'},{'t','T'},{'u','U'},{'v','V'},{'w','W'},{'x','X'},
    {'y','Y'},{'z','Z'},
    {'1','!'},{'2','"'},{'3','#'},{'4','$'},{'5','%'},{'6','&'},
    {'7','/'},{'8','('},{'9',')'},{'0','='},
    {'\n','\n'},{CMD_ESC,CMD_ESC},{'\b','\b'},{CMD_TAB,CMD_TAB},{' ',' '},
    {'-','_'},{'=','+'},{'[','{'},{']','}'},
    {'\\','|'},{'<','>'},{';',':'},{'\'','"'},
    {'`','~'},{',','<'},{'.','>'},{'/','?'}
};

// ===== Inicialización UART =====
static void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Crear cola para mantener orden en envíos UART
    uart_queue = xQueueCreate(64, sizeof(char));
}

// ===== Tarea de envío UART =====
static void uart_task(void *arg) {
    char c;
    while (1) {
        if (xQueueReceive(uart_queue, &c, portMAX_DELAY)) {
            uart_write_bytes(UART_PORT, &c, 1); // Enviar caracter
        }
    }
}

// ===== Encolar caracter para UART =====
static inline void send_char(char c) {
    if (uart_queue) {
        xQueueSend(uart_queue, &c, portMAX_DELAY);
    }
}

// ===== Procesar tecla individual =====
static void handle_key(uint8_t modifier, uint8_t key_code) {

    bool ctrl = (modifier & (HID_LEFT_CONTROL | HID_RIGHT_CONTROL)) !=0;

    // Combinaciones Ctrl
    if(ctrl){
        switch (key_code){
            case HID_KEY_C: send_char(CMD_CTRL_C); return;
            case HID_KEY_V: send_char(CMD_CTRL_V); return;
            case HID_KEY_X: send_char(CMD_CTRL_X); return;
            case HID_KEY_Z: send_char(CMD_CTRL_Z); return;
        }
    }

    // Flechas de dirección
    if (key_code == HID_KEY_UP)    { send_char(CMD_ARROW_UP); return; }
    if (key_code == HID_KEY_DOWN)  { send_char(CMD_ARROW_DOWN); return; }
    if (key_code == HID_KEY_LEFT)  { send_char(CMD_ARROW_LEFT); return; }
    if (key_code == HID_KEY_RIGHT) { send_char(CMD_ARROW_RIGHT); return; }

    // Teclas especiales normales
    switch(key_code) {
        case HID_KEY_ESCAPE:    send_char(CMD_ESC); return;
        case HID_KEY_TAB:       send_char(CMD_TAB); return;
        case HID_KEY_CAPS_LOCK: send_char(CMD_CAPS); return;
        case HID_KEY_BACKSPACE: send_char('\b');    return;
        case HID_KEY_ENTER:     send_char('\n');    return;
    }

    // ===== Teclado numérico =====
    switch (key_code) {
        case HID_KEY_KP_0: send_char('0'); return;
        case HID_KEY_KP_1: send_char('1'); return;
        case HID_KEY_KP_2: send_char('2'); return;
        case HID_KEY_KP_3: send_char('3'); return;
        case HID_KEY_KP_4: send_char('4'); return;
        case HID_KEY_KP_5: send_char('5'); return;
        case HID_KEY_KP_6: send_char('6'); return;
        case HID_KEY_KP_7: send_char('7'); return;
        case HID_KEY_KP_8: send_char('8'); return;
        case HID_KEY_KP_9: send_char('9'); return;

        case HID_KEY_KP_DIV:   send_char('/'); return;
        case HID_KEY_KP_MULT:  send_char('*'); return;
        case HID_KEY_KP_MINUS: send_char('-'); return;
        case HID_KEY_KP_PLUS:  send_char('+'); return;
        case HID_KEY_KP_DOT:   send_char('.'); return;
        case HID_KEY_KP_EQUAL: send_char('='); return;
        case HID_KEY_KP_ENTER: send_char('\n'); return;
        case HID_KEY_NUM_LOCK: return; // No hace nada
    }

    // F1 – F12
    switch(key_code) {
        case HID_KEY_F1:  send_char(CMD_F1);  return;
        case HID_KEY_F2:  send_char(CMD_F2);  return;
        case HID_KEY_F3:  send_char(CMD_F3);  return;
        case HID_KEY_F4:  send_char(CMD_F4);  return;
        case HID_KEY_F5:  send_char(CMD_F5);  return;
        case HID_KEY_F6:  send_char(CMD_F6);  return;
        case HID_KEY_F7:  send_char(CMD_F7);  return;
        case HID_KEY_F8:  send_char(CMD_F8);  return;
        case HID_KEY_F9:  send_char(CMD_F9);  return;
        case HID_KEY_F10: send_char(CMD_F10); return;
        case HID_KEY_F11: send_char(CMD_F11); return;
        case HID_KEY_F12: send_char(CMD_F12); return;
    }

    // AltGr
    if ((modifier & HID_RIGHT_ALT) != 0) {
        for (int i = 0; i < altgr_table_size; i++) {
            if (key_code == altgr_table[i].keycode) {
                send_char(altgr_table[i].symbol); // Enviar símbolo AltGr
                return;
            }
        }
    }

    // ASCII normal
    if (key_code < 57) {
        bool shift = (modifier & (HID_LEFT_SHIFT | HID_RIGHT_SHIFT)) != 0;
        char c = keycode2ascii[key_code][shift ? 1 : 0];
        if (c) send_char(c); // Enviar caracter normal
    }
}

// ===== Procesar reporte completo de teclado =====
static void process_keys(hid_keyboard_input_report_boot_t *report) {
    static hid_keyboard_input_report_boot_t prev_report = {0};
    static bool gui_pressed = false;

    uint8_t mods = report->modifier.val;

    // Detectar tecla GUI (Windows)
    if ((mods & (HID_LEFT_GUI | HID_RIGHT_GUI)) && !gui_pressed) {
        send_char(CMD_GUI);
        gui_pressed = true;
    } else if (!(mods & (HID_LEFT_GUI | HID_RIGHT_GUI))) {
        gui_pressed = false;
    }

    // Procesar teclas presionadas (keydown)
    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {
        uint8_t key = report->key[i];
        if (key > HID_KEY_ERROR_UNDEFINED) {
            bool already_pressed = false;
            for (int j = 0; j < HID_KEYBOARD_KEY_MAX; j++) {
                if (key == prev_report.key[j]) {
                    already_pressed = true;
                    break;
                }
            }
            if (!already_pressed) handle_key(mods, key);
        }
    }

    prev_report = *report;
}

// ===== Callbacks del host USB =====
static void hid_host_interface_callback(hid_host_device_handle_t dev_handle,
                                        const hid_host_interface_event_t event,
                                        void *arg) {
    if (event == HID_HOST_INTERFACE_EVENT_INPUT_REPORT) {
        uint8_t data[64] = {0};
        size_t len = 0;
        hid_host_device_get_raw_input_report_data(dev_handle, data, sizeof(data), &len);
        if (len >= sizeof(hid_keyboard_input_report_boot_t))
            process_keys((hid_keyboard_input_report_boot_t *)data); // Procesar entrada
    }
}

// ===== Evento de dispositivo USB =====
void hid_host_device_event(hid_host_device_handle_t hid_dev,
                           const hid_host_driver_event_t event,
                           void *arg) {
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };
        hid_host_device_open(hid_dev, &dev_config); // Abrir dispositivo
        hid_host_device_start(hid_dev);             // Iniciar dispositivo
    }
}

// ===== Tarea principal USB =====
static void usb_task(void *arg) {
    const usb_host_config_t host_config = {.skip_phy_setup = false, .intr_flags = ESP_INTR_FLAG_LEVEL1};
    usb_host_install(&host_config);

    hid_host_driver_config_t hid_cfg = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_event,
        .callback_arg = NULL
    };
    hid_host_install(&hid_cfg);

    // Manejar eventos USB continuamente
    while (1) {
        uint32_t events;
        usb_host_lib_handle_events(portMAX_DELAY, &events);
    }
}

// ===== Función principal =====
void app_main(void) {
    uart_init(); // Inicializar UART
    printf("UART initialized on TX:%d, RX:%d\n", TX_PIN, RX_PIN);
    xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL);  // Tarea USB
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 4, NULL);  // Tarea UART
    while (1) vTaskDelay(pdMS_TO_TICKS(1000)); // Mantener main vivo
}
