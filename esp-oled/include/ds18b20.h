#ifndef DS18B20_H
#define DS18B20_H

#include "driver/gpio.h"
#include "esp_err.h"

#define DS18B20_GPIO  GPIO_NUM_4

esp_err_t ds18b20_init(void);
esp_err_t ds18b20_read_temperature(float *celsius);
void      ds18b20_deinit(void);

#endif
