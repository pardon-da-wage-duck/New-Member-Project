#include "main.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "can.h"
#include "clock.h"
#include "gpio.h"
#include "adc.h"
#include "error_handler.h"
#include "core_config.h"
#include "rtt.h"
#include <SEGGER_RTT.h>

#include "timestamp.h"
#include "semphr.h"
#include "message_buffer.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stm32g4xx_hal.h>

#define LED_PORT GPIOA
#define LED_1_PIN GPIO_PIN_8
#define LED_2_PIN GPIO_PIN_9
#define LED_3_PIN GPIO_PIN_10

// Function for activity 1 and 2
void blinking_LEDS() {
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

void activity_3_and_4() {
    // Code for activity 3 and 4

    //Reading a potentiometer value from ADC1 channel 1 and printing it to RTT
    uint16_t potentiometer_reading = 0; // Pointer to hold the ADC value read by the potentiometer.
    core_ADC_read_channel(GPIOB, GPIO_PIN_12, &potentiometer_reading);
    rprintf("The potentiometer is reading: %d\n", potentiometer_reading);

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

void CAN_communication() {
    // Code for CAN communication
    uint64_t can_message = 0x48656C6C6F2C2043414E21; // "Hello, CAN!" in hex
    uint8_t can_message_length = sizeof(can_message);
    uint32_t can_message_id = 1;

    CanMessage_s recieved_can_message_struct;

    rprintf("Sending CAN message: %llX\n", can_message);
    bool send_success = core_CAN_send_message(FDCAN1, can_message_id, can_message_length, can_message);
    if (send_success) {
        rprintf("CAN message sent successfully\n");
    } else {
        rprintf("Failed to send CAN message\n");
    }

    bool receive_success = core_CAN_receive_from_queue(FDCAN1, &recieved_can_message_struct);
    if (receive_success) {
        rprintf("Received CAN message: %llX, Length: %d, ID: %d\n", recieved_can_message_struct.data, recieved_can_message_struct.dlc, recieved_can_message_struct.id);
    } else {
        rprintf("Failed to receive CAN message\n");
    }
}

void loop_task(void *pvParameters) {
    (void) pvParameters;
    while(true) {
        //blinking_LEDs();
        //activity_3_and_4();
        CAN_communication();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second to avoid busy looping
    }
}

int main(void) {
    //initialize the core libraries
    HAL_Init();
    core_RTT_init();
    core_CAN_init(FDCAN1, 1000000);

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
    
    int err = xTaskCreate(loop_task, "loop", 2000, NULL, 4, NULL);
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
