#ifndef OLED_H
#define OLED_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

/* ── Pin / bus config ───────────────────────────────────────────────────── */
#define I2C_MASTER_SCL_IO      22
#define I2C_MASTER_SDA_IO      21
#define I2C_MASTER_FREQ_HZ     400000

/* ── SSD1306 config ─────────────────────────────────────────────────────── */
#define OLED_I2C_ADDR          0x3C
#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_PAGES             (OLED_HEIGHT / 8)

/* ── Public API ─────────────────────────────────────────────────────────── */
void i2c_master_init(void);
void oled_init(void);
void fb_clear(void);
void fb_draw_string(int x, int page, const char *str);
void fb_draw_bitmap(int x, int y, int w, int h, const uint8_t *bitmap);
void fb_draw_sprite(int x, int y, int w, int h, const uint8_t *bitmap);
void fb_draw_fullscreen(const uint8_t *bitmap);
void oled_flush(void);

#endif /* OLED_H */
