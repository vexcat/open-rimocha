#include "oled.h"
#include "font.h"

static const char *TAG = "oled";

esp_err_t oled_init(oled_display* display) {
  display->mutex = xSemaphoreCreateMutex();

  gpio_config_t rst_conf = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = 1ULL << display->i2c.rst,
    .pull_down_en = 0,
    .pull_up_en = 0
  };
  gpio_config(&rst_conf);
  gpio_set_level(display->i2c.rst, 0);
  vTaskDelay(50);
  gpio_set_level(display->i2c.rst, 1);
  vTaskDelay(10);

  i2c_config_t config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = display->i2c.sda,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = display->i2c.scl,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 700000,
    .clk_flags = 0
  };
  ESP_ERROR_CHECK(i2c_param_config(display->i2c.port, &config));
  ESP_ERROR_CHECK(i2c_driver_install(display->i2c.port, I2C_MODE_MASTER, 0, 0, 0));

  oled_send_command(display, 0xAE); // Display Off
  oled_send_command(display, 0xA8); // Set MUX ratio
  oled_send_command(display, 0x3F); //   to 64
  oled_send_command(display, 0xD3); // Set Display Offset
  oled_send_command(display, 0x00); //   to 0
  oled_send_command(display, 0x40); // Set Display Start Line to 0
  oled_send_command(display, 0xA0); // Segment Remap
  oled_send_command(display, 0xC0); // COM Output Scan Direction
  oled_send_command(display, 0xDA); // Set COM Pins Config
  oled_send_command(display, 0x12); //   Alternative COM configuration
  oled_send_command(display, 0x81); // Set Contrast
  oled_send_command(display, 0x7F); //   to 127
  oled_send_command(display, 0xA4); // Disable Entire Display On
  oled_send_command(display, 0xA6); // Set Normal Display
  oled_send_command(display, 0xD5); // Set Osc Frequency
  oled_send_command(display, 0x80); //   Default frequency, 0 divide
  oled_send_command(display, 0x8D); // Charge Pump
  oled_send_command(display, 0x14); //   Enable Charge Pump
  oled_send_command(display, 0xAF); // Display On

  unsigned char empty[128] = { 0 };

  for(int i = 0; i < 8; i++) {
    oled_goto(display, i, 0);
    oled_send_data(display, empty, 128);
  }

  oled_goto(display, 0, 0);

  return ESP_OK;
}

void oled_send(oled_display* display, bool isData, uint8_t* data, size_t data_len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (display->i2c.address << 1) | I2C_MASTER_WRITE, true));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, isData ? 0x40 : 0x80, true));
  if(data_len > 0)
    ESP_ERROR_CHECK(i2c_master_write(cmd, data, data_len, true));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));
  ESP_ERROR_CHECK(i2c_master_cmd_begin(display->i2c.port, cmd, 10));
  i2c_cmd_link_delete(cmd);
}

void oled_send_command(oled_display* display, uint8_t command) {
  oled_send(display, false, &command, 1);
}

void oled_send_data(oled_display* display, uint8_t* data, size_t data_len) {
  xSemaphoreTake(display->mutex, portMAX_DELAY);
  oled_send(display, true, data, data_len);
  xSemaphoreGive(display->mutex);
}

void oled_goto(oled_display* display, int page, int col) {
  xSemaphoreTake(display->mutex, portMAX_DELAY);
  oled_send_command(display, 0x00 |  (col & 0x0f));
  oled_send_command(display, 0x10 | ((col & 0xf0) >> 4));
  oled_send_command(display, 0xB0 |  (page & 7));
  xSemaphoreGive(display->mutex);
}

void oled_clear_page(oled_display* display, int page) {
  oled_goto(display, page, 0);
  unsigned char empty[128] = { 0 };
  oled_send_data(display, empty, 128);
  oled_goto(display, page, 0);
}

void oled_send_text(oled_display* display, const char* format, ...) {
  char formatted[80];
  va_list(args);

  va_start(args, format);
  vsnprintf(formatted, 80, format, args);

  for(int i = 0; formatted[i]; i++) {
    oled_send_data(display, font_6x8[(unsigned char)formatted[i]], 6);
  }
}
