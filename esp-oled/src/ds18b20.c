#include "ds18b20.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bitmaps.h"

static const char *TAG = "ds18b20";

/*
 * OneWire bit-banging helpers
 * All timing values assume a 1-Wire bus speed of ~16 kbps (standard mode).
 */

static void ow_delay_us(uint32_t us)
{
    ets_delay_us(us);
}

static void ow_set_output(void)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT_OD);
}

static void ow_set_input(void)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
}

static void ow_write_low(void)
{
    gpio_set_level(DS18B20_GPIO, 0);
}

static void ow_write_high(void)
{
    gpio_set_level(DS18B20_GPIO, 1);
}

static int ow_read_pin(void)
{
    return gpio_get_level(DS18B20_GPIO);
}

/*
 * OneWire reset & presence pulse
 * Returns 1 if a device is present, 0 otherwise.
 */
static int ow_reset(void)
{
    int presence;

    ow_set_output();
    ow_write_low();
    ow_delay_us(480);
    ow_set_input();
    ow_delay_us(70);
    presence = (ow_read_pin() == 0) ? 1 : 0;
    ow_delay_us(410);

    return presence;
}

static void ow_write_bit(int bit)
{
    ow_set_output();
    ow_write_low();
    ow_delay_us(2);
    if (bit)
        ow_write_high();
    ow_delay_us(58);
    ow_write_high();
    ow_delay_us(1);
    ow_set_input();
}

static int ow_read_bit(void)
{
    int bit;

    ow_set_output();
    ow_write_low();
    ow_delay_us(2);
    ow_set_input();
    ow_delay_us(10);
    bit = ow_read_pin();
    ow_delay_us(55);

    return bit;
}

static void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ow_read_bit())
            byte |= 0x80;
    }

    return byte;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

esp_err_t ds18b20_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DS18B20_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* wait for sensor to power up */
    vTaskDelay(pdMS_TO_TICKS(10));

    int attempts = 3;
    while (attempts--) {
        if (ow_reset()) {
            ESP_LOGI(TAG, "DS18B20 found on GPIO %d", DS18B20_GPIO);
            return ESP_OK;
        }
        ESP_LOGW(TAG, "no presence pulse, retrying...");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGE(TAG, "DS18B20 not found on GPIO %d", DS18B20_GPIO);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ds18b20_read_temperature(float *celsius)
{   
    if (!celsius)
        return ESP_ERR_INVALID_ARG;

    if (!ow_reset())
        return ESP_ERR_NOT_FOUND;


    /* Skip ROM (single sensor on bus) + Start Conversion */
    ow_write_byte(0xCC);
    ow_write_byte(0x44);

    /* wait for conversion (max 750 ms at 12-bit resolution) */
    vTaskDelay(pdMS_TO_TICKS(750));

    if (!ow_reset())
        return ESP_ERR_NOT_FOUND;

    /* Skip ROM + Read Scratchpad */
    ow_write_byte(0xCC);
    ow_write_byte(0xBE);

    uint8_t scratchpad[9];
    for (int i = 0; i < 9; i++)
        scratchpad[i] = ow_read_byte();

    int16_t raw = (int16_t)(scratchpad[1] << 8 | scratchpad[0]);
    *celsius = raw * 0.0625f;

    ESP_LOGD(TAG, "temperature: %.2f C", *celsius);
    return ESP_OK;
}

void ds18b20_deinit(void)
{
    gpio_reset_pin(DS18B20_GPIO);
    ESP_LOGI(TAG, "DS18B20 deinitialized");
}
