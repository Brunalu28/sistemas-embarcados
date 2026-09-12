#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Define GPIOs
#define LED_BIT_0 GPIO_NUM_4
#define LED_BIT_1 GPIO_NUM_5
#define LED_BIT_2 GPIO_NUM_6
#define LED_BIT_3 GPIO_NUM_7

#define BTN_A GPIO_NUM_15 
#define BTN_B GPIO_NUM_16 

#define DEBOUNCE_TIME_MS 250

uint8_t counter = 0;
uint8_t step = 1;
uint32_t last_btn_a_time = 0;
uint32_t last_btn_b_time = 0;

void update_leds(uint8_t val) {
    gpio_set_level(LED_BIT_0, (val & 0x01) ? 1 : 0);
    gpio_set_level(LED_BIT_1, (val & 0x02) ? 1 : 0);
    gpio_set_level(LED_BIT_2, (val & 0x04) ? 1 : 0);
    gpio_set_level(LED_BIT_3, (val & 0x08) ? 1 : 0);
}

void app_main(void) {
    // Configure LEDs as outputs
    gpio_config_t io_conf_led = {
        .pin_bit_mask = (1ULL<<LED_BIT_0) | (1ULL<<LED_BIT_1) | (1ULL<<LED_BIT_2) | (1ULL<<LED_BIT_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_led);

    // Configure Buttons as inputs with internal pull-ups
    gpio_config_t io_conf_btn = {
        .pin_bit_mask = (1ULL<<BTN_A) | (1ULL<<BTN_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_btn);

    update_leds(counter);

    while (1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Button A: Increment Counter
        if (gpio_get_level(BTN_A) == 0) {
            if (current_time - last_btn_a_time > DEBOUNCE_TIME_MS) {
                // Modulo 16 arithmetic naturally handles the 0x0 to 0xF overflow limits
                counter = (counter + step) & 0x0F; 
                update_leds(counter);
                last_btn_a_time = current_time;
            }
        }

        // Button B: Toggle Step between 1 and 2
        if (gpio_get_level(BTN_B) == 0) {
            if (current_time - last_btn_b_time > DEBOUNCE_TIME_MS) {
                step = (step == 1) ? 2 : 1;
                last_btn_b_time = current_time;
            }
        }

        // Minimal non-blocking delay to yield task and feed watchdog
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}