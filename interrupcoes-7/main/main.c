#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "SISTEMA_ILUMINACAO";

#define LED_PIN         GPIO_NUM_18
#define BUTTON_PIN      GPIO_NUM_4

#define DEBOUNCE_TIME_US       (50 * 1000)
#define LONG_PRESS_TIME_US     (2 * 1000 * 1000)
#define TIMEOUT_10S_US         (10 * 1000 * 1000)

typedef enum {
    BUTTON_EVENT_PRESS,
    BUTTON_EVENT_RELEASE
} button_event_type_t;

typedef struct {
    button_event_type_t type;
    int64_t timestamp;
} button_event_t;

static QueueHandle_t gpio_evt_queue = NULL;
static esp_timer_handle_t led_off_timer = NULL;
static esp_timer_handle_t long_press_timer = NULL;

static bool led_state = false;
static bool long_press_handled = false;
static int64_t last_isr_time = 0;

static void led_off_timer_callback(void* arg)
{
    led_state = false;
    gpio_set_level(LED_PIN, 0);
    ESP_LOGI(TAG, "Tempo esgotado (10s) -> LED desligado.");
}

static void long_press_timer_callback(void* arg)
{
    long_press_handled = true;
    if (led_state) {
        led_state = false;
        gpio_set_level(LED_PIN, 0);
        if (esp_timer_is_active(led_off_timer)) {
            esp_timer_stop(led_off_timer);
        }
        ESP_LOGI(TAG, "Pressionamento longo (>=2s) -> LED desligado.");
    }
}

static void IRAM_ATTR gpio_button_isr_handler(void* arg)
{
    int64_t current_time = esp_timer_get_time();
    
    if ((current_time - last_isr_time) >= DEBOUNCE_TIME_US) {
        last_isr_time = current_time;
        
        int level = gpio_get_level(BUTTON_PIN);
        button_event_t evt;
        evt.type = (level == 0) ? BUTTON_EVENT_PRESS : BUTTON_EVENT_RELEASE;
        evt.timestamp = current_time;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(gpio_evt_queue, &evt, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

static void button_task(void* arg)
{
    button_event_t evt;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY)) {
            if (evt.type == BUTTON_EVENT_PRESS) {
                long_press_handled = false;
                esp_timer_stop(long_press_timer);
                esp_timer_start_once(long_press_timer, LONG_PRESS_TIME_US);
            } 
            else if (evt.type == BUTTON_EVENT_RELEASE) {
                esp_timer_stop(long_press_timer);

                if (!long_press_handled) {
                    if (!led_state) {
                        led_state = true;
                        gpio_set_level(LED_PIN, 1);
                        esp_timer_start_once(led_off_timer, TIMEOUT_10S_US);
                        ESP_LOGI(TAG, "Clique curto -> LED ligado (10s).");
                    } else {
                        esp_timer_stop(led_off_timer);
                        esp_timer_start_once(led_off_timer, TIMEOUT_10S_US);
                        ESP_LOGI(TAG, "Clique curto -> Temporizador de 10s renovado.");
                    }
                }
            }
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf_led = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_led);
    gpio_set_level(LED_PIN, 0);

    gpio_config_t io_conf_btn = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf_btn);

    gpio_evt_queue = xQueueCreate(10, sizeof(button_event_t));

    esp_timer_create_args_t led_timer_args = {
        .callback = &led_off_timer_callback,
        .name = "led_off_timer"
    };
    esp_timer_create(&led_timer_args, &led_off_timer);

    esp_timer_create_args_t long_press_args = {
        .callback = &long_press_timer_callback,
        .name = "long_press_timer"
    };
    esp_timer_create(&long_press_args, &long_press_timer);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_button_isr_handler, NULL);

    vTaskDelay(portMAX_DELAY);
}