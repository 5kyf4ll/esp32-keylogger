#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"

#define TX_PIN 17
#define RX_PIN 18
#define UART_PORT UART_NUM_2
#define BUF_SIZE 256

// HID key definitions
#define HID_KEY_ENTER       0x28
#define HID_KEY_ESCAPE      0x29
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_TAB         0x2B
#define HID_KEY_CAPS_LOCK   0x39

// Custom command codes (solo los que no tienen ASCII)
#define CMD_ESC   0x1B
#define CMD_TAB   0x09
#define CMD_CAPS  0x14
#define CMD_GUI   0x90

// AltGr special character mapping
typedef struct {
    uint8_t keycode;
    char symbol;
} altgr_map_t;

static const altgr_map_t altgr_table[] = {
    {HID_KEY_Q, '@'},
    // Agrega aquí más símbolos según tu layout
};
static const int altgr_table_size = sizeof(altgr_table) / sizeof(altgr_map_t);

// Basic HID to ASCII mapping (without AltGr)
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
}

static void send_char(char c) {
    uart_write_bytes(UART_PORT, &c, 1);
}

static void handle_key(uint8_t modifier, uint8_t key_code) {
    switch(key_code) {
        case HID_KEY_ESCAPE:    send_char(CMD_ESC); return;
        case HID_KEY_TAB:       send_char(CMD_TAB); return;
        case HID_KEY_CAPS_LOCK: send_char(CMD_CAPS); return;
        case HID_KEY_BACKSPACE: send_char('\b');    return; // ASCII backspace
        case HID_KEY_ENTER:     send_char('\n');    return; // ASCII newline
    }

    // AltGr handling
    if ((modifier & HID_RIGHT_ALT) != 0) {
        for (int i = 0; i < altgr_table_size; i++) {
            if (key_code == altgr_table[i].keycode) {
                char c = altgr_table[i].symbol;
                send_char(c);
                return;
            }
        }
    }

    // Normal key handling
    if(key_code < 57) {
        bool shift = (modifier & (HID_LEFT_SHIFT | HID_RIGHT_SHIFT)) != 0;
        char c = keycode2ascii[key_code][shift ? 1 : 0];
        if(c) send_char(c);
    }
}

static void process_keys(hid_keyboard_input_report_boot_t *report) {
    static hid_keyboard_input_report_boot_t prev_report = {0};
    static bool gui_pressed = false;

    uint8_t mods = report->modifier.val;

    // Detect GUI (Windows key)
    if ((mods & (HID_LEFT_GUI | HID_RIGHT_GUI)) && !gui_pressed) {
        send_char(CMD_GUI);
        gui_pressed = true;
    } else if (!(mods & (HID_LEFT_GUI | HID_RIGHT_GUI))) {
        gui_pressed = false;
    }

    // Process normal keys (keydown detection)
    for(int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {
        uint8_t key = report->key[i];
        if(key > HID_KEY_ERROR_UNDEFINED) {
            bool already_pressed = false;
            for (int j = 0; j < HID_KEYBOARD_KEY_MAX; j++) {
                if (key == prev_report.key[j]) {
                    already_pressed = true;
                    break;
                }
            }
            if (!already_pressed) {
                handle_key(mods, key);
            }
        }
    }

    prev_report = *report;
}

static void hid_host_interface_callback(hid_host_device_handle_t dev_handle,
                                        const hid_host_interface_event_t event,
                                        void *arg) {

    if(event == HID_HOST_INTERFACE_EVENT_INPUT_REPORT) {
        uint8_t data[64] = {0};
        size_t len = 0;
        hid_host_device_get_raw_input_report_data(dev_handle, data, sizeof(data), &len);
        if(len < sizeof(hid_keyboard_input_report_boot_t)) return;

        process_keys((hid_keyboard_input_report_boot_t *)data);
    }
}

void hid_host_device_event(hid_host_device_handle_t hid_dev,
                           const hid_host_driver_event_t event,
                           void *arg) {

    if(event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };
        hid_host_device_open(hid_dev, &dev_config);
        hid_host_device_start(hid_dev);
    }
}

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

    while(1) {
        uint32_t events;
        usb_host_lib_handle_events(portMAX_DELAY, &events);
    }
}

void app_main(void) {
    uart_init();
    printf("UART initialized on pins TX:%d, RX:%d\n", TX_PIN, RX_PIN);
    xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL);
    while(1) vTaskDelay(pdMS_TO_TICKS(1000));
}
