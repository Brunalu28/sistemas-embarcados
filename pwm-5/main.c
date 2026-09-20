#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // Biblioteca adicionada para o PWM

// Define GPIOs (Mantidos os originais da sua referência)
#define LED_BIT_0 GPIO_NUM_4
#define LED_BIT_1 GPIO_NUM_5
#define LED_BIT_2 GPIO_NUM_6
#define LED_BIT_3 GPIO_NUM_7

#define BTN_A GPIO_NUM_15 
#define BTN_B GPIO_NUM_16 

// Novos pinos adicionados para o PWM
#define LED_PWM_PIN    GPIO_NUM_17
#define BUZZER_PWM_PIN GPIO_NUM_18

#define DEBOUNCE_TIME_MS 250

// Variáveis voláteis pois agora serão alteradas dentro da Interrupção
volatile uint8_t counter = 0;
volatile uint8_t step = 1;
volatile uint32_t last_btn_a_time = 0;
volatile uint32_t last_btn_b_time = 0;
volatile bool update_req = true; // Flag para avisar o loop principal que precisa atualizar as saídas

// Função mantida da sua referência
void update_leds(uint8_t val) {
    gpio_set_level(LED_BIT_0, (val & 0x01) ? 1 : 0);
    gpio_set_level(LED_BIT_1, (val & 0x02) ? 1 : 0);
    gpio_set_level(LED_BIT_2, (val & 0x04) ? 1 : 0);
    gpio_set_level(LED_BIT_3, (val & 0x08) ? 1 : 0);
}

// Nova função para atualizar o PWM do LED e do Buzzer
void update_pwm(uint8_t val) {
    // Brilho do LED (0 a 15 convertido proporcionalmente para 0 a 255)
    uint32_t duty = (val * 255) / 15; 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Frequência do Buzzer
    if (val == 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0); // Mudo se contador for 0
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    } else {
        uint32_t freq = 400 + (val * 150); // Aumenta o tom gradativamente
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 127); // Liga em ~50% do Duty Cycle
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
}

// Função de Interrupção (Substitui a leitura no loop while)
static void IRAM_ATTR btn_isr_handler(void* arg) {
    // Pega o tempo de forma segura para dentro de ISR
    uint32_t current_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    int btn = (int)arg;

    if (btn == BTN_A) {
        if (current_time - last_btn_a_time > DEBOUNCE_TIME_MS) {
            // Modulo 16 arithmetic naturally handles the 0x0 to 0xF limits
            counter = (counter + step) & 0x0F; 
            last_btn_a_time = current_time;
            update_req = true; // Avisa o loop para atualizar o hardware
        }
    } else if (btn == BTN_B) {
        if (current_time - last_btn_b_time > DEBOUNCE_TIME_MS) {
            // Requisito da Atividade 05: Botão B deve DECREMENTAR
            counter = (counter - step) & 0x0F; 
            last_btn_b_time = current_time;
            update_req = true; // Avisa o loop para atualizar o hardware
        }
    }
}

void app_main(void) {
    // Configure LEDs as outputs (Sua referência original)
    gpio_config_t io_conf_led = {
        .pin_bit_mask = (1ULL<<LED_BIT_0) | (1ULL<<LED_BIT_1) | (1ULL<<LED_BIT_2) | (1ULL<<LED_BIT_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_led);

    // Configure Buttons as inputs with internal pull-ups and interrupts
    gpio_config_t io_conf_btn = {
        .pin_bit_mask = (1ULL<<BTN_A) | (1ULL<<BTN_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // <-- Alterado de DISABLE para NEGEDGE (Interrupção obrigatória)
    };
    gpio_config(&io_conf_btn);

    // Instala o serviço de interrupção
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_A, btn_isr_handler, (void*) BTN_A);
    gpio_isr_handler_add(BTN_B, btn_isr_handler, (void*) BTN_B);

    // ============= CONFIGURAÇÃO DE PWM (LEDC) =============
    // Timer para o LED (Frequência fixa)
    ledc_timer_config_t led_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&led_timer);

    ledc_channel_config_t led_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PWM_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&led_channel);

    // Timer para o Buzzer (Frequência variável)
    ledc_timer_config_t buz_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 1000, 
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buz_timer);

    ledc_channel_config_t buz_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BUZZER_PWM_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&buz_channel);
    // =======================================================

    while (1) {
        // Se a interrupção sinalizar que ocorreu uma mudança, atualizamos as saídas
        if (update_req) {
            update_req = false;
            update_leds(counter);
            update_pwm(counter);
            printf("Contador atual: %d\n", counter);
        }

        // Minimal non-blocking delay to yield task and feed watchdog (Sua referência original)
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}