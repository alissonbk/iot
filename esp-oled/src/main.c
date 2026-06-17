#include "oled.h"
#include "bitmaps.h"

static const char *TAG = "oled";

static void show_hello_world(void)
{
    const char *msg = "Hello World";
    int text_width  = (int)strlen(msg) * 6 - 1;
    int start_x     = (OLED_WIDTH - text_width) / 2;
    int start_page  = 3;

    fb_draw_string(start_x, start_page, msg);
    oled_flush();

    ESP_LOGI(TAG, "\"Hello World\" displayed!");
}

static void draw_skull(void) {

    fb_draw_fullscreen(skull_bitmap);
    oled_flush();
}

void app_main(void)
{
    oled_init();
    // show_hello_world();
    // draw_skull();
    fb_draw_bitmap(5, 5, 28, 28, temperature_bitmap);
    oled_flush();
}