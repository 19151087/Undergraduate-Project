#include "main.h"

__attribute__((unused)) static const char TAG[] = "main";
using namespace ESPFirebase;

#define PMS7003_PERIOD_MS 30000


TaskHandle_t getDataFromSensorTask_handle = NULL;

// Timer handle for sensor
TimerHandle_t pms7003_timer = NULL;

// Mutex for protecting currentData
SemaphoreHandle_t currentDataMutex = NULL;

QueueHandle_t dataSensorSentToMQTT_queue;

uart_config_t pms_uart_config = UART_CONFIG_DEFAULT();

// Initialize struct for storing sensor data
dataSensor_st dataSensor;
dataSensor_st currentData;

static sht3x_t dev;

// Firebase config and Authentication
user_account_t account = {USER_EMAIL, USER_PASSWORD};
FirebaseApp app = FirebaseApp(API_KEY);
RTDB db = RTDB(&app, DATABASE_URL);

// NTP time
uint32_t timeStamp;

// pms_wake_status of PMS7003 sensor (TRUE = Wake up, FALSE = Sleep)
bool pms_wake_status = false;
// pms7003 timer callback
void pms7003_timer_callback(TimerHandle_t xTimer)
{
    uint16_t pm1_0Temp, pm2_5Temp, pm10Temp;
    // If the sensor wakes up, we will read data from it then put it in sleep mode
    if(pms_wake_status == true)
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
        pms_wake_status = false;
    }
    // If sensor is in sleep mode, it will wake up and take 30s to warm up the fan
    else // if(pms_pms_wake_status == PMS_SLEEP)
    {
        ESP_ERROR_CHECK(pms7003_wakeUpMode());
        pms_wake_status = true;
    }
}

static void getDataFromSensor_task(void *pvParameters)
{
    float temperatureTemp, humidityTemp;
    TickType_t last_wakeup = xTaskGetTickCount();
    currentDataMutex = xSemaphoreCreateMutex();
    // Prepare for timer callback function
    pms_wake_status = true;
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

    // login to firebase
    app.loginUserAccount(account);
    std::string json_str = R"({"Temperature": 0, "Humidity": 0, "PM1_0": 0, "PM2_5": 0, "PM10": 0, "Timestamp": 0})";

    std::string databasePath = "/dataSensor/Indoor";
    // Get current time
    timeStamp = getTime();
    // combine dataPath and timeStamp to create a new path
    std::string path = databasePath + "/" + std::to_string(timeStamp);
    db.putData(path.c_str(), json_str.c_str());
    // Parse the json_str and access the members and edit them
    Json::Value data;
    Json::Reader reader;
    reader.parse(json_str, data);

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
            printf("Time: %d \n", timeStamp);

            timeStamp = getTime();
            data["Temperature"] = dataSensor.temperature;
            data["Humidity"] = dataSensor.humidity;
            data["PM1_0"] = dataSensor.pm1_0;
            data["PM2_5"] = dataSensor.pm2_5;
            data["PM10"] = dataSensor.pm10;
            data["Timestamp"] = timeStamp;
            std::string path = databasePath + "/" + std::to_string(timeStamp);
            db.putData(path.c_str(), data);
        }

        // wait until 10 seconds are over
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(30000));
    }

    // Delete task, can not reach here
    vTaskDelete(NULL);
}

void init_sensors()
{
    //Initialize SHT31 sensor
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&dev, 0, sizeof(sht3x_t));

    ESP_ERROR_CHECK(sht3x_init_desc(&dev, // sht31 descriptor
                                    CONFIG_SHT31_I2C_ADDR, // i2c address
                                    CONFIG_SHT31_I2C_PORT, // i2c port
                                    (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SDA, // sda pin
                                    (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SCL)); // scl pin
    
    ESP_ERROR_CHECK(sht3x_init(&dev));

    // Initialize PMS7003 sensor
    ESP_ERROR_CHECK(pms7003_initUart(&pms_uart_config));

    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Create task to get data from sensor
    xTaskCreate(getDataFromSensor_task, // Task function
                "getDataFromSensor_task", // Task name
                (1024 * 64), // Stack size
                NULL, // Parameter
                24, // Priority
                &getDataFromSensorTask_handle); // Task handle
}

/*----------------------------------*WIFI-CONNECTION*--------------------------------*/

/* wifi callback */
void wifi_connect_ok_callback(void *pvParameter)
{
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGI(__func__, "I have a connection and my IP is %s!", str_ip);

    /* initialize ntp*/
    Set_SystemTime_SNTP();

    /* initialize sensors */
    init_sensors();
}

extern "C" void app_main(void)
{
    /* start wifi manager*/
    wifi_manager_start();
    /* register a callback when connection is success*/
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &wifi_connect_ok_callback);

    //wifi_manager_disconnect_async();
}