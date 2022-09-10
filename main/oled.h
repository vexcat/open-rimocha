#pragma once
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "driver/i2c.h"

typedef struct oled_display {
  struct {
    int scl, sda, rst;
    uint8_t address;
    i2c_port_t port;
  } i2c;
  SemaphoreHandle_t mutex;
} oled_display;

esp_err_t oled_init(oled_display* display);
void oled_send_text(oled_display* display, const char* format, ...);
void oled_send(oled_display* display, bool isData, uint8_t* data, size_t data_len);
void oled_send_command(oled_display* display, uint8_t command);
void oled_send_data(oled_display* display, uint8_t* data, size_t data_len);
void oled_goto(oled_display* display, int page, int col);
void oled_clear_page(oled_display* display, int page);
