#include "main.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "can.h"
#include "clock.h"
#include "gpio.h"
#include "adc.h"
#include "error_handler.h"
#include "core_config.h"
#include "rtt.h"
#include <SEGGER_RTT.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stm32g4xx_hal.h>

#define LED_PORT GPIOA
#define LED_1_PIN GPIO_PIN_8
#define LED_2_PIN GPIO_PIN_9
#define LED_3_PIN GPIO_PIN_10

// Function for activity 1 and 2
void blinking_LED() {
    //Set GPIO pin 8 high for 500ms, otherwise set low for 500ms
    core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_SET);
    vTaskDelay(500 * portTICK_PERIOD_MS);
    core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_RESET);

    //Set GPIO pin 9 high for 500ms, otherwise set low for 1000ms
    core_GPIO_digital_write(LED_PORT, LED_2_PIN, GPIO_PIN_SET);
    vTaskDelay(1000 * portTICK_PERIOD_MS);
    core_GPIO_digital_write(LED_PORT, LED_2_PIN, GPIO_PIN_RESET);

    //Set GPIO pin 10 high for 500ms, otherwise set low for 200ms
    core_GPIO_digital_write(LED_PORT, LED_3_PIN, GPIO_PIN_SET);
    vTaskDelay(200 * portTICK_PERIOD_MS);
    core_GPIO_digital_write(LED_PORT, LED_3_PIN, GPIO_PIN_RESET);
}

void GPIO_task(void *pvParameters) {
    (void) pvParameters;
    while(true) {
        // Code for activity 3 and 4

        //Reading a potentiometer value from ADC1 channel 1 and printing it to RTT
        uint16_t* potentiometer_reading; // Pointer to hold the ADC value read by the potentiometer.
        core_ADC_read_channel(GPIOB, GPIO_PIN_12, potentiometer_reading);
        rprintf("The potentiometer is reading: %u\n", (unsigned int)*potentiometer_reading);

        //Start by turning all the leds off
        core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_RESET);
        core_GPIO_digital_write(LED_PORT, LED_2_PIN, GPIO_PIN_RESET);
        core_GPIO_digital_write(LED_PORT, LED_3_PIN, GPIO_PIN_RESET);

        //Check the potentiometer reading and turn on the corresponding LEDs
        if(potentiometer_reading < 1000) {
            core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_SET);
        }
        else if(potentiometer_reading > 1500 && potentiometer_reading < 2000){
            core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_SET);
            core_GPIO_digital_write(LED_PORT, LED_2_PIN, GPIO_PIN_SET);
        }
        else if(potentiometer_reading > 3000 && potentiometer_reading < 4000){
            core_GPIO_digital_write(LED_PORT, LED_1_PIN, GPIO_PIN_SET);
            core_GPIO_digital_write(LED_PORT, LED_2_PIN, GPIO_PIN_SET);
            core_GPIO_digital_write(LED_PORT, LED_3_PIN, GPIO_PIN_SET);
        }
    }
}

int main(void) {
    HAL_Init();
    core_RTT_init();

    // Drivers
    //Initialize GPIO pins for heartbeat
    core_GPIO_init(GPIOA, GPIO_PIN_8, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    core_GPIO_init(GPIOA, GPIO_PIN_9, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    core_GPIO_init(GPIOA, GPIO_PIN_10, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    //core_GPIO_set_heartbeat(GPIO_PIN_RESET);

    //Initialize ADC GPIO Port
    core_ADC_init(ADC1);
    core_ADC_setup_pin(GPIOB, GPIO_PIN_12, 0);

    if (!core_clock_init()) error_handler();
    if (!core_CAN_init(FDCAN1, 1000000)) error_handler();
    
    int err = xTaskCreate(GPIO_task, "heartbeat", 2000, NULL, 4, NULL);
    if (err != pdPASS) {
        error_handler();
    }

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // hand control over to FreeRTOS
    vTaskStartScheduler();

    // we should not get here ever
    error_handler();
    return 1;
}

// Called when stack overflows from rtos
// Not needed in header, since included in FreeRTOS-Kernel/include/task.h
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName) {
    (void) xTask;
    (void) pcTaskName;

    error_handler();
}