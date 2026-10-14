#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "UART_LOOPBACK";

#define LED_PIN         GPIO_NUM_18
#define TXD_PIN         GPIO_NUM_17
#define RXD_PIN         GPIO_NUM_16

#define UART_PORT_NUM   UART_NUM_2
#define BUF_SIZE        1024

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

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));

    uint8_t rx_buffer[BUF_SIZE];
    bool toggle_cmd = true;

    while (1) {
        const char *tx_data = toggle_cmd ? "LIGAR" : "DESLIGAR";
        toggle_cmd = !toggle_cmd;

        ESP_LOGI(TAG, "Enviado via UART2: %s", tx_data);
        uart_write_bytes(UART_PORT_NUM, tx_data, strlen(tx_data));

        int len = uart_read_bytes(UART_PORT_NUM, rx_buffer, BUF_SIZE - 1, pdMS_TO_TICKS(1000));

        if (len > 0) {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "Recebido via Loopback (UART2): %s", (char *)rx_buffer);

            if (strcmp((char *)rx_buffer, "LIGAR") == 0) {
                gpio_set_level(LED_PIN, 1);
                ESP_LOGI(TAG, "Ação executada: LED Ligado");
            } 
            else if (strcmp((char *)rx_buffer, "DESLIGAR") == 0) {
                gpio_set_level(LED_PIN, 0);
                ESP_LOGI(TAG, "Ação executada: LED Desligado");
            }
        } else {
            ESP_LOGW(TAG, "Nenhum dado recebido (Timeout)");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}