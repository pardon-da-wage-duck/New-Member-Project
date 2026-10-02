#include "main.h"

#include <stdbool.h>
#include <stdio.h>

#include "can.h"
#include "clock.h"
#include "gpio.h"
#include "error_handler.h"
#include "core_config.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stm32g4xx_hal.h>


void heartbeat_task(void *pvParameters) {
    (void) pvParameters;
    while(true) {
        core_GPIO_toggle_heartbeat();
        vTaskDelay(100 * portTICK_PERIOD_MS);
    }
}

int main(void) {
    HAL_Init();

    // Drivers
    core_heartbeat_init(GPIOA, GPIO_PIN_8);
    core_GPIO_set_heartbeat(GPIO_PIN_RESET);

    if (!core_clock_init()) error_handler();
    if (!core_CAN_init(FDCAN1, 1000000)) error_handler();

    int err = xTaskCreate(heartbeat_task, "heartbeat", 1000, NULL, 4, NULL);
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
