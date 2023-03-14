
#include "main.h"

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";

#define PMS7003_PERIOD_MS 30000


TaskHandle_t getDataFromSensorTask_handle = NULL;

// Timer handle for sensor
TimerHandle_t pms7003_timer = NULL;

// Mutex for protecting currentData
SemaphoreHandle_t currentDataMutex = NULL;


// Initialize struct for storing sensor data
dataSensor_st dataSensor;
dataSensor_st currentData;

static sht3x_t dev;

uart_config_t pms_uart_config = UART_CONFIG_DEFAULT();
// Status of PMS7003 sensor (TRUE = Wake up, FALSE = Sleep)
bool status = false;


// pms7003 timer callback
void pms7003_timer_callback(TimerHandle_t xTimer)
{
    uint16_t pm1_0Temp, pm2_5Temp, pm10Temp;
    // If the sensor wakes up, we will read data from it then put it in sleep mode
    if(status == true)
    {
        if(pms7003_readData(&pm1_0Temp, &pm2_5Temp, &pm10Temp) == true)
        {
            ESP_ERROR_CHECK(pms7003_sleepMode());
            
            if(xSemaphoreTake(currentDataMutex, portMAX_DELAY) == pdTRUE)
            {
                // Update the data structure
                currentData.pm1_0 = pm1_0Temp;
                currentData.pm2_5 = pm2_5Temp;
                currentData.pm10 = pm10Temp;
                xSemaphoreGive(currentDataMutex);
            }
        }
        else
        {
            printf("Read data from PMS7003 failed \n");
        }
        status = false;
    }
    // If sensor is in sleep mode, it will wake up and take 30s to warm up the fan
    else // if(pms_status == PMS_SLEEP)
    {
        ESP_ERROR_CHECK(pms7003_wakeUpMode());
        status = true;
    }
}

// task to get data from sensor
static void getDataFromSensor_task(void *pvParameters)
{
    float temperatureTemp, humidityTemp;
    TickType_t last_wakeup = xTaskGetTickCount();

    // Prepare for timer callback function
    status = true;
    printf("Fan is running... \n");

    currentDataMutex = xSemaphoreCreateMutex();

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

    while (1)
    {
        // perform one measurement and do something with the results
        ESP_ERROR_CHECK(sht3x_measure(&dev, &temperatureTemp, &humidityTemp));

        // Lock mutex to protect currentData
        if(xSemaphoreTake(currentDataMutex, portMAX_DELAY) == pdTRUE)
        {
            // Update the data structure
            currentData.temperature = temperatureTemp;
            currentData.humidity = humidityTemp;
            // Release mutex
            xSemaphoreGive(currentDataMutex);
        }

        // Check if any data is updated
        if(memcmp(&dataSensor, &currentData, sizeof(dataSensor_st)) != 0)
        {

            memcpy(&dataSensor, &currentData, sizeof(dataSensor_st));

            // Print data to console
            printf("Temperature: %.2f °C, Relative humidity: %.2f %%\n", dataSensor.temperature, 
                                                        dataSensor.humidity);
            printf("pm1_0 : %d (ug/m3), pm2_5: %d (ug/m3), pm10: %d (ug/m3) \n",dataSensor.pm1_0, 
                                                                                dataSensor.pm2_5, 
                                                                                dataSensor.pm10);
        }

        // wait until 5 seconds are over
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(5000));
    }

    // Delete task
    vTaskDelete(NULL);
}

void init_sensors()
{
    //(&dataSensor, 0, sizeof(struct dataSensor_st));
    //Initialize SHT31 sensor
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&dev, 0, sizeof(sht3x_t));

    ESP_ERROR_CHECK(sht3x_init_desc(&dev, // sht31 descriptor
                                    CONFIG_SHT31_I2C_ADDR, // i2c address
                                    CONFIG_SHT31_I2C_PORT, // i2c port
                                    CONFIG_SHT31_I2C_MASTER_SDA, // sda pin
                                    CONFIG_SHT31_I2C_MASTER_SCL)); // scl pin
    
    ESP_ERROR_CHECK(sht3x_init(&dev));

    // Initialize PMS7003 sensor
    ESP_ERROR_CHECK(pms7003_initUart(&pms_uart_config));

    vTaskDelay(500 / portTICK_PERIOD_MS);
}


/* wifi callback */
void cb_connection_ok(void *pvParameter)
{
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
}


void app_main(void)
{
    /* start wifi manager*/
    wifi_manager_start();

    /* register a callback when connection is success*/
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

    init_sensors();
    // Create task to get data from sensor
    xTaskCreate(getDataFromSensor_task, // Task function
                "getDataFromSensor_task", // Task name
                (1024 * 64), // Stack size
                NULL, // Parameter
                5, // Priority
                &getDataFromSensorTask_handle); // Task handle
}
