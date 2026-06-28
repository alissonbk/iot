#include "oled.h"
#include "bitmaps.h"
#include "ds18b20.h"

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
    static int temp_str_size = 32 + 8;
    char temp_str[temp_str_size];
    
    ds18b20_init();
    oled_init();
    // show_hello_world();
    // draw_skull();    
    fb_draw_bitmap(5, 5, 28, 28, temperature_bitmap);    
    float temp;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_err_t err = ds18b20_read_temperature(&temp);
        if (err != ESP_OK) {
            printf("failed to read temperature: %d", err);
        }
        ESP_LOGI(TAG, "temperature: %f", temp);    
        snprintf(temp_str, temp_str_size, "%.2f Celsius", temp);
        fb_draw_string(40, 4, temp_str);
        oled_flush();
    }
}