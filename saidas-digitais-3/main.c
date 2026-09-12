#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED0_GPIO GPIO_NUM_4
#define LED1_GPIO GPIO_NUM_5
#define LED2_GPIO GPIO_NUM_6
#define LED3_GPIO GPIO_NUM_7

#define DELAY_TIME_MS 500

void setup(void)
{
    gpio_reset_pin(LED0_GPIO);
    esp_rom_gpio_pad_select_gpio(LED0_GPIO);
    gpio_set_direction(LED0_GPIO, GPIO_MODE_OUTPUT);
   
    gpio_reset_pin(LED1_GPIO);
    esp_rom_gpio_pad_select_gpio(LED1_GPIO);
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);


    gpio_reset_pin(LED2_GPIO);
    esp_rom_gpio_pad_select_gpio(LED2_GPIO);
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);


    gpio_reset_pin(LED3_GPIO);
    esp_rom_gpio_pad_select_gpio(LED3_GPIO);
    gpio_set_direction(LED3_GPIO, GPIO_MODE_OUTPUT);
}

void n1(void)
{
  uint8_t current_state = 0;
  uint8_t max_state = 16;
  for (int i = 0; i < 16; i++)
  {
      gpio_set_level(LED0_GPIO, (current_state >> 0) & 1);
      gpio_set_level(LED1_GPIO, (current_state >> 1) & 1);
      gpio_set_level(LED2_GPIO, (current_state >> 2) & 1);
      gpio_set_level(LED3_GPIO, (current_state >> 3) & 1);
      current_state = (current_state + 1) % max_state;
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
  }
}

void n2(void)
{
  const int states_size = 9;
  int states[states_size] = {0x1, 0x2, 0x4, 0x8, 0x0, 0x8, 0x4, 0x2, 0x1};
  gpio_set_level(LED0_GPIO, 0);
  gpio_set_level(LED1_GPIO, 0);
  gpio_set_level(LED2_GPIO, 0);
  gpio_set_level(LED3_GPIO, 0);
  vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
  for(int i = 0; i < states_size; i++)
  {
    const int state = states[i];
    gpio_set_level(LED0_GPIO, (state >> 0 & 1));
    gpio_set_level(LED1_GPIO, (state >> 1 & 1));
    gpio_set_level(LED2_GPIO, (state >> 2 & 1));
    gpio_set_level(LED3_GPIO, (state >> 3 & 1));
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
  }
}


void loop(void)
{
  n1();
  n2();
}
