/**
 * Hello World on SSD1306 OLED using ESP-IDF new I2C master driver (v5.x+)
 *
 * Wiring (default):
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * SSD1306 I2C address: 0x3C (most common) or 0x3D
 */

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
#define OLED_I2C_ADDR          0x3C   /* change to 0x3D if yours has SA0 high */
#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_PAGES             (OLED_HEIGHT / 8)  /* 8 pages of 8 rows each */

static const char *TAG = "oled";

/* ── Globals ────────────────────────────────────────────────────────────── */
static i2c_master_bus_handle_t  bus_handle;
static i2c_master_dev_handle_t  oled_handle;

/* Frame-buffer: 128 × 8 pages = 1024 bytes */
static uint8_t fb[OLED_WIDTH * OLED_PAGES];

/* ── 5×7 font (printable ASCII 0x20–0x7E) ──────────────────────────────── */
/* Each character is 5 bytes wide; height is 7 pixels (within an 8-px page) */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* ''' */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x30,0x30,0x00,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' */
    {0x00,0x08,0x14,0x22,0x41}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x41,0x22,0x14,0x08,0x00}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x41,0x51,0x32}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x03,0x04,0x78,0x04,0x03}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x00,0x7F,0x41,0x41}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' */
    {0x41,0x41,0x7F,0x00,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x08,0x14,0x54,0x54,0x3C}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x00,0x7F,0x10,0x28,0x44}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' */
    {0x08,0x08,0x2A,0x1C,0x08}, /* '~' */
};

/* ════════════════════════════════════════════════════════════════════════════
   Low-level helpers
   ════════════════════════════════════════════════════════════════════════════ */

/** Send one or more command bytes to the SSD1306. */
static void oled_send_cmd(const uint8_t *cmds, size_t len)
{
    /* SSD1306 I2C frame: [control byte 0x00] [cmd] [cmd] ... */
    uint8_t buf[64];
    buf[0] = 0x00; /* Co=0, D/C#=0 → command stream */
    memcpy(&buf[1], cmds, len);
    ESP_ERROR_CHECK(i2c_master_transmit(oled_handle, buf, len + 1, -1));
}

/** Push the frame-buffer to the OLED GDDRAM. */
static void oled_flush(void)
{
    for (uint8_t page = 0; page < 8; page++) {

        uint8_t cmds[] = {
            (uint8_t)(0xB0 + page),
            0x02,   // lower column offset (important for SH1106)
            0x10    // higher column
        };

        oled_send_cmd(cmds, sizeof(cmds));

        uint8_t buf[129];
        buf[0] = 0x40;
        memcpy(&buf[1], &fb[page * OLED_WIDTH], OLED_WIDTH);

        ESP_ERROR_CHECK(i2c_master_transmit(oled_handle, buf, 129, -1));
    }
}

/* ════════════════════════════════════════════════════════════════════════════
   Frame-buffer drawing helpers
   ════════════════════════════════════════════════════════════════════════════ */

static void fb_clear(void) { memset(fb, 0, sizeof(fb)); }

/**
 * Draw one character at pixel column x, page row (0-7).
 * Returns the x position after the character (x + 6).
 */
static int fb_draw_char(int x, int page, char c)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *glyph = font5x7[c - 0x20];

    for (int col = 0; col < 5; col++) {
        int px = x + col;
        if (px >= OLED_WIDTH) break;
        fb[page * OLED_WIDTH + px] = glyph[col];
    }
    /* 1-pixel gap after character */
    if (x + 5 < OLED_WIDTH)
        fb[page * OLED_WIDTH + x + 5] = 0x00;

    return x + 6; /* advance 6 pixels (5 glyph + 1 gap) */
}

/** Draw a null-terminated string; wraps to next page if text overflows. */
static void fb_draw_string(int x, int page, const char *str)
{
    while (*str && page < OLED_PAGES) {
        if (x + 6 > OLED_WIDTH) { x = 0; page++; }
        x = fb_draw_char(x, page, *str++);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
   SSD1306 initialisation sequence
   ════════════════════════════════════════════════════════════════════════════ */

static void oled_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,       /* display off */
        0xD5, 0x80, /* set display clock divide ratio / oscillator freq */
        0xA8, 0x3F, /* set multiplex ratio (63 → 64 MUX) */
        0xD3, 0x00, /* set display offset = 0 */
        0x40,       /* set display start line = 0 */
        0x8D, 0x14, /* charge pump: enable */
        0x20, 0x00, /* memory addressing mode: horizontal */
        0xA1,       /* segment re-map: col 127 → SEG0 */
        0xC8,       /* COM output scan direction: remapped */
        0xDA, 0x12, /* COM pins hardware config */
        0x81, 0xCF, /* set contrast */
        0xD9, 0xF1, /* set pre-charge period */
        0xDB, 0x40, /* set Vcomh deselect level */
        0xA4,       /* entire display ON (follow GDDRAM) */
        0xA6,       /* normal display (not inverted) */
        0xAF,       /* display on */
    };
    oled_send_cmd(init_seq, sizeof(init_seq));
    fb_clear();
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(20));
}

/* ════════════════════════════════════════════════════════════════════════════
   I2C master init  (your original code, unchanged)
   ════════════════════════════════════════════════════════════════════════════ */

static void i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port              = I2C_NUM_0,
        .sda_io_num            = I2C_MASTER_SDA_IO,
        .scl_io_num            = I2C_MASTER_SCL_IO,
        .clk_source            = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt     = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    /* Add the SSD1306 as a device on the bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = OLED_I2C_ADDR,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &oled_handle));
}

/* ════════════════════════════════════════════════════════════════════════════
   app_main
   ════════════════════════════════════════════════════════════════════════════ */

void app_main(void)
{
    ESP_LOGI(TAG, "Initialising I2C …");
    i2c_master_init();

    ESP_LOGI(TAG, "Initialising SSD1306 OLED …");
    oled_init();
    

    /* ── Draw "Hello World" centred on the display ── */
    fb_clear();

    /* "Hello World" is 11 chars × 6 px = 66 px wide; centre on 128-px display */
    const char *msg = "Hello World";
    int text_width  = (int)strlen(msg) * 6 - 1; /* last char has no trailing gap */
    int start_x     = (OLED_WIDTH - text_width) / 2;
    int start_page  = 3; /* page 3 of 8 ≈ vertical centre */

    fb_draw_string(start_x, start_page, msg);
    oled_flush();

    ESP_LOGI(TAG, "\"Hello World\" displayed!");
}