
// doc data tu cam bien sau do luu vao struct datamanager

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/spi_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"

#include "button.h"
#include "pms7003.h"
#include "sht3x.h"
#include "datamanager.h"

#define PMS7003_PERIOD_MS 30000


TaskHandle_t getDataFromSensorTask_handle = NULL;

// Timer handle for sensor
TimerHandle_t pms7003_timer = NULL;


// Initialize struct for storing sensor data
dataSensor_st dataSensor;

uart_config_t pms_uart_config = UART_CONFIG_DEFAULT();


// Status of PMS7003 sensor (TRUE = Wake up, FALSE = Sleep)
bool status = false;
// pms7003 timer callback
void pms7003_timer_callback(TimerHandle_t xTimer)
{
    // If the sensor wakes up, we will read data from it then put it in sleep mode
    if(status == true)
    {
        
        if(pms7003_readData(&dataSensor.pm1_0, &dataSensor.pm2_5, &dataSensor.pm10) == true)
        {
            printf("pm1_0 : %d (ug/m3), pm2_5: %d (ug/m3), pm10: %d (ug/m3) \n", dataSensor.pm1_0, dataSensor.pm2_5, dataSensor.pm10);
            pms7003_sleepMode();
            status = false;
            memset(&dataSensor, 0, sizeof(struct dataSensor_st));
        }
        else
        {
            printf("Read data from PMS7003 failed \n");
        }
    }
    // If sensor is in sleep mode, it will wake up and take 30s to warm up the fan
    else // if(pms_status == PMS_SLEEP)
    {
        pms7003_wakeUpMode();
        status = true;
    }
}

// // sht31 timer callback
// // void sht31_timer_callback(TimerHandle_t xTimer)
// // {
// //     // get data from sensor
// //     ESP_ERROR_CHECK_WITHOUT_ABORT(sht3x_getData(&dataSensor.temperature, &dataSensor.humidity));
// // }

// task to get data from sensor
static void getDataFromSensor_task(void *pvParameters)
{
    // Initialize PMS7003 sensor
    pms7003_initUart(&pms_uart_config);
    //pms7003_passiveMode();

    // Prepare for timer callback function
    status = true;

    printf("Fan is running... \n");

    pms7003_timer = xTimerCreate("pms7003_timer", // Just a text name, not used by the kernel.
                                PMS7003_PERIOD_MS / portTICK_PERIOD_MS, // The timer period in ticks.
                                pdTRUE, // The timers will auto-reload themselves when they expire.
                                0, // Does not use the timer ID.
                                pms7003_timer_callback); // The callback function that used by timer being created.

    // if timer is created successfully
    if(pms7003_timer != NULL)
    {
        // Start timer
        xTimerStart(pms7003_timer, 0);
    }

    // Delete task
    vTaskDelete(NULL);
}


void app_main(void)
{
    // Create task to get data from sensor
    xTaskCreate(getDataFromSensor_task, // Task function
                "getDataFromSensor_task", // Task name
                4096, // Stack size
                NULL, // Parameter
                5, // Priority
                &getDataFromSensorTask_handle); // Task handle





    // uart_config_t pms_uart_config = UART_CONFIG_DEFAULT();
    // pms7003_initUart(&pms_uart_config);
    // //pms7003_activeMode();

    // uint16_t pm1_0_t, pm2_5_t, pm10_t;
    // while(1)
    // {
    //     pms7003_wakeUpMode();
    //     vTaskDelay(10000 / portTICK_PERIOD_MS);
    //     pms7003_readData(&pm1_0_t, &pm2_5_t, &pm10_t);
        
    //     //vTaskDelay(1000 / portTICK_PERIOD_MS);

    //     printf("PM1.0 (ug/m3) = %d\n", pm1_0_t);
    //     printf("PM2.5 (ug/m3) = %d\n", pm2_5_t);
    //     printf("PM10.0 (ug/m3) = %d\n", pm10_t);

    //     pms7003_sleepMode();
    //     vTaskDelay(30000 / portTICK_PERIOD_MS);
    // }

}
