#include "gpio.h"
#include "rcc.h"
#include "systick.h"

#define EXTI_BASE 0x40010400
#define EXTI ((EXTI_t *)EXTI_BASE)

#define EXTI15_10_IRQn 40
#define NVIC_ISER1 ((uint32_t *)(0xE000E104)) // NVIC Interrupt Set-Enable Register

#define SYSCFG_BASE 0x40010000
#define SYSCFG ((SYSCFG_t *)SYSCFG_BASE)

#define GPIOA ((GPIO_t *)0x48000000) // Base address of GPIOA
#define GPIOC ((GPIO_t *)0x48000800) // Base address of GPIOC

#define LED_PIN 5 // Pin 5 of GPIOA
#define DOOR_LED_PIN 4 // Pin 4 of GPIOA 
#define BUTTON_PIN 13 // Pin 13 of GPIOC


void configure_gpio_for_usart(void)
{
    // Enable GPIOA clock
    *RCC_AHB2ENR |= (1 << 0);

    // Configure PA2 (TX) as alternate function
    GPIOA->MODER &= ~(3U << (2 * 2)); // Clear mode bits for PA2
    GPIOA->MODER |= (2U << (2 * 2));  // Set alternate function mode for PA2

    // Configure PA3 (RX) as alternate function
    GPIOA->MODER &= ~(3U << (3 * 2)); // Clear mode bits for PA3
    GPIOA->MODER |= (2U << (3 * 2));  // Set alternate function mode for PA3

    // Set alternate function to AF7 for PA2 and PA3
    GPIOA->AFR[0] &= ~(0xF << (4 * 2)); // Clear AFR bits for PA2
    GPIOA->AFR[0] |= (7U << (4 * 2));   // Set AFR to AF7 for PA2
    GPIOA->AFR[0] &= ~(0xF << (4 * 3)); // Clear AFR bits for PA3
    GPIOA->AFR[0] |= (7U << (4 * 3));   // Set AFR to AF7 for PA3

    // Configure PA2 and PA3 as very high speed
    GPIOA->OSPEEDR |= (3U << (2 * 2)); // Very high speed for PA2
    GPIOA->OSPEEDR |= (3U << (3 * 2)); // Very high speed for PA3

    // Configure PA2 and PA3 as no pull-up, no pull-down
    GPIOA->PUPDR &= ~(3U << (2 * 2)); // No pull-up, no pull-down for PA2
    GPIOA->PUPDR &= ~(3U << (3 * 2)); // No pull-up, no pull-down for PA3
}

void configure_gpio(void)
{
   *RCC_AHB2ENR |= (1 << 0) | (1 << 2); // Enable clock for GPIOA and GPIOC

    // Configure PA5 as output
    GPIOA->MODER &= ~(3U << (LED_PIN * 2)); // Clear mode bits for PA5
    GPIOA->MODER |= (1U << (LED_PIN * 2));  // Set output mode for PA5

    // Configure PA4 as output
    GPIOA->MODER &= ~(3U << (DOOR_LED_PIN * 2)); // Clear mode bits for PA4
    GPIOA->MODER |= (1U << (DOOR_LED_PIN * 2));  // Set output mode for PA4

    // Configure PC13 as input
    GPIOC->MODER &= ~(3U << (BUTTON_PIN * 2)); // Clear mode bits for PC13
    GPIOC->MODER |= (0U << (BUTTON_PIN * 2));  // Set input mode for PC13

    // Enable clock for SYSCFG
    *RCC_APB2ENR |= (1 << 0); // RCC_APB2ENR_SYSCFGEN

    // Configure SYSCFG EXTICR to map EXTI13 to PC13
    SYSCFG->EXTICR[3] &= ~(0xF << 4); // Clear bits for EXTI13
    SYSCFG->EXTICR[3] |= (0x2 << 4);  // Map EXTI13 to Port C

    // Configure EXTI13 for falling edge trigger
    EXTI->FTSR1 |= (1 << BUTTON_PIN);  // Enable falling trigger
    EXTI->RTSR1 &= ~(1 << BUTTON_PIN); // Disable rising trigger

    // Unmask EXTI13
    EXTI->IMR1 |= (1 << BUTTON_PIN);

    // Enable EXTI15_10 interrupt
    *NVIC_ISER1 |= (1 << (EXTI15_10_IRQn - 32));
    
}

// Emula el comprtamiento de la puerta
void gpio_set_door_led_state(uint8_t state) {
    if (state) {
        GPIOA->ODR |= (1 << 4); // encender LED estado puerta
    } else {
        GPIOA->ODR &= ~(1 << 4); // apagar LED estado puerta
    }
}

void gpio_toggle_heartbeat_led(void) {
    GPIOA->ODR ^= (1 << 5);
}

volatile uint8_t button_pressed = 0; // Flag to indicate button press
uint8_t button_driver_get_event(void)
{
    uint8_t event = button_pressed;
    button_pressed = 0;
    return event;

}

uint32_t b1_tick = 0;
void detect_button_press(void)
{
    if (systick_GetTick() - b1_tick < 50) {
        return; // Ignore bounces of less than 50 ms
    } else if (systick_GetTick() - b1_tick > 500) {
        button_pressed = 1; // single press
    } else {
        button_pressed = 2; // double press
    }

    b1_tick = systick_GetTick();
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR1 & (1 << BUTTON_PIN)) {
        EXTI->PR1 = (1 << BUTTON_PIN); // Clear pending bit
        detect_button_press();
    }
}