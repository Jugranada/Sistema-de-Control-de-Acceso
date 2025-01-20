#include "gpio.h"
#include "systick.h"
#include "uart.h"

#define TEMP_UNLOCK_DURATION 5000 // Duration in ms for temporary unlock

typedef enum {
    LOCKED,
    TEMP_UNLOCK,
    PERM_UNLOCK
} DoorState_t;

DoorState_t current_state = LOCKED;
uint32_t unlock_timer = 0;

void run_state_machine(void) {
    switch (current_state) {
        case LOCKED:
            // Estado cerrado: sin acción periódica
            break;
        case TEMP_UNLOCK:
            if (systick_GetTick() - unlock_timer >= TEMP_UNLOCK_DURATION) {
                gpio_set_door_led_state(0); // Apagar LED de estado
                current_state = LOCKED;
            }
            break;
        case PERM_UNLOCK:
            // Estado desbloqueado permanente: sin acción periódica
            break;
    }
}

void handle_event(uint8_t event) {
    if (event == 1) { // Presión simple
        usart2_send_string("Presión simple\r\n");
        gpio_set_door_led_state(1); // Encender LED
        current_state = TEMP_UNLOCK;
        unlock_timer = systick_GetTick();
    } else if (event == 2) { // Presión doble
        usart2_send_string("Presión doble\r\n");
        gpio_set_door_led_state(1); // Encender LED
        current_state = PERM_UNLOCK;
    } else if (event == 'O') { // Comando abrir
        usart2_send_string("Comando abrir recibido\r\n");
        gpio_set_door_led_state(1);
        current_state = TEMP_UNLOCK;
        unlock_timer = systick_GetTick();
    } else if (event == 'C') { // Comando cerrar
        usart2_send_string("Comando cerrar recibido\r\n");
        gpio_set_door_led_state(0);
        current_state = LOCKED;
    }
}

int main(void) {
    configure_systick_and_start();
    configure_gpio();
    usart2_init();

    usart2_send_string("System Initialized\r\n");
    
    // [...Inicialización del sistema]

    uint32_t heartbeat_tick = 0;
    while (1) {
        if (systick_GetTick() - heartbeat_tick >= 500) {
            heartbeat_tick = systick_GetTick();
            gpio_toggle_heartbeat_led();
        }

        uint8_t button_pressed = button_driver_get_event();
        if (button_pressed != 0) {
            usart2_send_string("Button Pressed\r\n");
            handle_event(button_pressed);
            button_pressed = 0;
        }

        uint8_t rx_byte = usart2_get_command();
        if (rx_byte != 0) {
            usart2_send_string("Command Received\r\n");
            handle_event(rx_byte);
        }

        run_state_machine();
    }
}