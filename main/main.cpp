#include "main.h"

static const char TAG[] = "main";

/*--------------------------LVGL-------------------------------*/
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
void LCDScreen(void);
void act_tabhome(lv_obj_t *parent);
void act_tab1(lv_obj_t *parent);
void act_tab2(lv_obj_t *parent);
void act_tab3(lv_obj_t *parent);
void update_param(void);

char CurrentTime[20];
lv_obj_t *wifi_label;
lv_obj_t *mac_adrr_label;
lv_obj_t *wifi_status_label;
lv_obj_t *sd_status_label;
lv_obj_t *firebase_status_label;
const char *wifi_status = "Connecting...";
const char *sd_status = "Connecting...";
const char *firebase_status = "Connecting...";
/*--------------------------Indicate variable in tab 1-------------------------------*/
lv_obj_t *temp_bar_tab1;
lv_obj_t *temp_label_tab1;

lv_obj_t *hum_gauge_tab1;
lv_obj_t *hum_label_tab1;

lv_obj_t *pm2p5_label_tab1;
lv_obj_t *pm10_label_tab1;
lv_obj_t *pm1_label_tab1;

lv_obj_t *date_label;
/*--------------------------Indicate variable in tab 2-------------------------------*/
lv_obj_t *pm2p5_label_tab2;
lv_obj_t *pm10_label_tab2;
lv_obj_t *pm1_label_tab2;
static lv_obj_t *pm_meter_tab2;
lv_meter_indicator_t *indic_pm2p5_tab2;
lv_meter_indicator_t *indic_pm10_tab2;
lv_meter_indicator_t *indic_pm1_tab2;
/*--------------------------Indicate variable in tab 3-------------------------------*/
static lv_obj_t *temp_meter_tab3;
static lv_obj_t *hum_meter_tab3;
lv_meter_indicator_t *temp_indic_tab3;
lv_meter_indicator_t *hum_indic_tab3;

lv_obj_t *temp_label_tab3;
lv_obj_t *hum_label_tab3;

static lv_group_t *keypad_group;

lv_indev_t *indev_keypad;

/*--------------------------MAIN-------------------------------*/
using namespace ESPFirebase;
char str_ip[16];     // IP address
uint8_t mac_adrr[6]; // MAC address
char str_mac[18];    // string MAC address

static button_t rst_esp_btn1;
static button_t btn2;
static button_t rst_wifi_btn3;

TaskHandle_t getDataFromSensorTask_handle = NULL;
TaskHandle_t FirebaseTask_handle = NULL;
TaskHandle_t SDCardTask_handle = NULL;
TaskHandle_t GUITask_handle = NULL;
TaskHandle_t getTimeFromRTCTask_handle = NULL;

QueueHandle_t DataToSDCardQueue = NULL;
QueueHandle_t DataToFirebaseQueue = NULL;
QueueHandle_t FirebaseToSDCardQueue = NULL;
QueueHandle_t SDCardToFirebaseQueue = NULL;

SemaphoreHandle_t currentDataMutex = NULL;
SemaphoreHandle_t FirebaseMutex = NULL;
SemaphoreHandle_t SDCardMutex = NULL;

// Initialize struct for storing sensor data
dataSensor_st currentDataSensor;

static sht3x_t sht31_dev;
static i2c_dev_t ds3231_dev;

// Firebase config and Authentication
user_account_t account = {USER_EMAIL, USER_PASSWORD};
FirebaseApp app = FirebaseApp(API_KEY);
RTDB db = RTDB(&app, DATABASE_URL);

// bool toggleSDbtn2 = false;
static void button_cb(button_t *btn, button_state_t state)
{
    if (btn == &rst_esp_btn1 && state == BUTTON_PRESSED_LONG)
    {
        ESP_LOGI(TAG, "Restart ESP32");
        esp_restart();
    }
    // if (btn == &btn2 && state == BUTTON_PRESSED_LONG)
    // {
    //     toggleSDbtn2 = !toggleSDbtn2;
    //     if (toggleSDbtn2)
    //     {
    //         sd_status = "Suspended";
    //         vTaskSuspend(SDCardTask_handle);
    //     }
    //     else
    //     {
    //         sd_status = "Connected";
    //         vTaskResume(SDCardTask_handle);
    //     }
    //     ESP_LOGI(TAG, "suspend SD card!");
    // }
    if (btn == &rst_wifi_btn3 && state == BUTTON_PRESSED_LONG)
    {
        wifi_manager_disconnect_async();
    }
}

void init_btn(void)
{
    // Button 1
    rst_esp_btn1.gpio = (gpio_num_t)CONFIG_EXAMPLE_BUTTON1_GPIO;
    rst_esp_btn1.pressed_level = 0;
    rst_esp_btn1.internal_pull = true;
    rst_esp_btn1.autorepeat = false;
    rst_esp_btn1.callback = button_cb;
    ESP_ERROR_CHECK(button_init(&rst_esp_btn1));

    // Button 2
    // btn2.gpio = (gpio_num_t)CONFIG_EXAMPLE_BUTTON2_GPIO;
    // btn2.pressed_level = 0;
    // btn2.internal_pull = true;
    // btn2.autorepeat = false;
    // btn2.callback = button_cb;
    // ESP_ERROR_CHECK(button_init(&btn2));

    // Button 3
    rst_wifi_btn3.gpio = (gpio_num_t)CONFIG_EXAMPLE_BUTTON3_GPIO;
    rst_wifi_btn3.pressed_level = 0;
    rst_wifi_btn3.internal_pull = true;
    rst_wifi_btn3.autorepeat = false;
    rst_wifi_btn3.callback = button_cb;
    ESP_ERROR_CHECK(button_init(&rst_wifi_btn3));
}

static void getDataFromSensor_task(void *pvParameters)
{
    TickType_t last_wakeup = xTaskGetTickCount();
    currentDataMutex = xSemaphoreCreateMutex();
    uint16_t pm1_0Temp, pm2_5Temp, pm10Temp;

    while (1)
    {
        if (xSemaphoreTake(currentDataMutex, portMAX_DELAY) == pdTRUE)
        {
            // Read data from sensor
            if (pms_readData(&pm1_0Temp, &pm2_5Temp, &pm10Temp) == PMS_OK)
            {
                currentDataSensor.pm1_0 = pm1_0Temp;
                currentDataSensor.pm2_5 = pm2_5Temp;
                currentDataSensor.pm10 = pm10Temp;
            }
            else
            {
                ESP_LOGE(__func__, "Read data from PMS7003 failed \n");
            }
            ESP_ERROR_CHECK(sht3x_measure(&sht31_dev, &(currentDataSensor.temperature), &(currentDataSensor.humidity)));
            // currentDataSensor.timestamp = sntp_getTime(); // get timestamp
            ESP_ERROR_CHECK(ds3231_get_timestamp(&ds3231_dev, &(currentDataSensor.timestamp)));
            xSemaphoreGive(currentDataMutex); // release mutex
        }

        // Print data to console for debugging
        printf("Timestamp: %u \n", currentDataSensor.timestamp);
        printf("Temperature: %.2f °C, Relative humidity: %.2f %%\n", currentDataSensor.temperature, currentDataSensor.humidity);
        printf("pm1_0 : %d (ug/m3), pm2_5: %d (ug/m3), pm10: %d (ug/m3) \n", currentDataSensor.pm1_0, currentDataSensor.pm2_5, currentDataSensor.pm10);

        // Send to message queue
        if (strcmp(firebase_status, "Logged in successfully") == 0)
        {
            if (xQueueSendToBack(DataToFirebaseQueue, (void *)&currentDataSensor, pdMS_TO_TICKS(50)) != pdPASS)
            {
                ESP_LOGE(__func__, "Failed to send data to DataToFirebaseQueue");
            }
            else
            {
                ESP_LOGI(__func__, "send data to DataToFirebaseQueue successfully");
            }
        }

        if (strcmp(sd_status, "Connected") == 0)
        {
            if (xQueueSendToBack(DataToSDCardQueue, (void *)&currentDataSensor, pdMS_TO_TICKS(50)) != pdPASS)
            {
                ESP_LOGE(__func__, "Failed to send data to DataToSDCardQueue");
            }
            else
            {
                ESP_LOGI(__func__, "send data to DataToSDCardQueue successfully");
            }
        }
        // wait until 3 seconds are over
        vTaskDelayUntil(&last_wakeup, (3000 / portTICK_PERIOD_MS));
    }

    // Delete task, can not reach here
    vTaskDelete(NULL);
}

static void Firebase_task(void *pvParameters)
{
    dataSensor_st ReceivedDataFromQueue;
    FBSettings_st FBsettings;
    FBSettings_st ReceivedFeedbackFromSDcard;
    uint32_t prevFBTimeStamp;
    bool SDEnable = false;
    ds3231_get_timestamp(&ds3231_dev, &prevFBTimeStamp);
    bool connectStatus = false;
    // login to firebase
    if (app.loginUserAccount(account) == ESP_OK)
    {
        connectStatus = true;
        firebase_status = "Logged in successfully";
    }
    else
    {
        connectStatus = false;
    }
    std::string JSONDataStr = R"({"Temperature": 0, "Humidity": 0, "PM1_0": 0, "PM2_5": 0, "PM10": 0, "Timestamp": 0})";

    std::string databasePath = "/dataSensor/";
    databasePath += str_mac;

    std::string dataLogPath = databasePath + "/dataLog";
    std::string realtimePath = databasePath + "/realtime/";
    std::string settingsPath = databasePath + "/settings";

    Json::Value settings;
    // Parse the JSONDataStr and access the members and edit them
    Json::Value data;
    Json::Reader reader;
    reader.parse(JSONDataStr, data);

    FirebaseMutex = xSemaphoreCreateMutex();

    while (1)
    {
        if (connectStatus == true)
        {
            settings = db.getData(settingsPath.c_str());
            if (xSemaphoreTake(FirebaseMutex, portMAX_DELAY) == pdTRUE)
            {
                SDEnable = settings["SD_enable"].asBool();
                FBsettings.SDDelete = settings["SD_delete"].asBool();
                FBsettings.LoggingPeriod = settings["logging_period"].asInt();
                xSemaphoreGive(FirebaseMutex);
            }
            if (SDEnable == false)
            {
                sd_status = "Suspended";
                vTaskSuspend(SDCardTask_handle);
            }
            else
            {
                sd_status = "Connected";
                vTaskResume(SDCardTask_handle);
            }
            if (SDEnable == true && FBsettings.SDDelete == true)
            {
                if (xQueueSendToBack(FirebaseToSDCardQueue, (void *)&FBsettings, pdMS_TO_TICKS(50)) != pdPASS)
                {
                    ESP_LOGE(__func__, "Failed to send data to FirebaseToSDCardQueue");
                }
                else
                {
                    ESP_LOGI(__func__, "send data to FirebaseToSDCardQueue successfully");
                }
            }
            if (uxQueueMessagesWaiting(SDCardToFirebaseQueue) != 0)
            {
                if (xQueueReceive(SDCardToFirebaseQueue, (void *)&ReceivedFeedbackFromSDcard, portMAX_DELAY) == pdPASS)
                {
                    if (xSemaphoreTake(FirebaseMutex, portMAX_DELAY) == pdTRUE)
                    {
                        settings["SD_delete"] = ReceivedFeedbackFromSDcard.SDDelete;
                        db.patchData(settingsPath.c_str(), settings);
                        xSemaphoreGive(FirebaseMutex);
                    }
                }
            }
            if (uxQueueMessagesWaiting(DataToFirebaseQueue) != 0)
            {
                if (xQueueReceive(DataToFirebaseQueue, (void *)&ReceivedDataFromQueue, portMAX_DELAY) == pdPASS)
                {
                    if (xSemaphoreTake(FirebaseMutex, portMAX_DELAY) == pdTRUE)
                    {
                        // Update the json string
                        data["Temperature"] = ReceivedDataFromQueue.temperature;
                        data["Humidity"] = ReceivedDataFromQueue.humidity;
                        data["PM1_0"] = ReceivedDataFromQueue.pm1_0;
                        data["PM2_5"] = ReceivedDataFromQueue.pm2_5;
                        data["PM10"] = ReceivedDataFromQueue.pm10;
                        data["Timestamp"] = ReceivedDataFromQueue.timestamp;
                        // Store to realtimePath every 3 seconds
                        db.putData(realtimePath.c_str(), data);
                        // Store to databasePath every 30 seconds by minus timestamp with prevTimeStamp
                        if (ReceivedDataFromQueue.timestamp - prevFBTimeStamp >= FBsettings.LoggingPeriod)
                        {
                            // Create a new timestamp data and put to firebase
                            std::string path = dataLogPath + "/" + std::to_string(ReceivedDataFromQueue.timestamp);
                            db.putData(path.c_str(), data);
                            prevFBTimeStamp = ReceivedDataFromQueue.timestamp;
                        }
                        xSemaphoreGive(FirebaseMutex);
                    }
                    else
                    {
                        ESP_LOGE(__func__, "Failed to take FirebaseMutex");
                    }
                }
                else
                {
                    ESP_LOGE(__func__, "Failed to receive data from DataToFirebaseQueue");
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Delete task, can not reach here
    vTaskDelete(NULL);
}

static void SDCard_task(void *pvParameters)
{
    dataSensor_st ReceivedDataFromQueue;
    FBSettings_st SDCardSettings;
    FBSettings_st ReceivedSettingsFromFirebase;
    SDCardMutex = xSemaphoreCreateMutex();
    const char *dataFileName = MOUNT_POINT "/data.txt";
    esp_err_t e;
    uint32_t prevSDTimeStamp;
    SDCardSettings.LoggingPeriod = 30; // default
    ds3231_get_timestamp(&ds3231_dev, &prevSDTimeStamp);
    while (1)
    {
        if (uxQueueMessagesWaiting(DataToSDCardQueue) != 0)
        {
            if (xQueueReceive(DataToSDCardQueue, (void *)&ReceivedDataFromQueue, portMAX_DELAY) == pdPASS)
            {
                if (xSemaphoreTake(SDCardMutex, portMAX_DELAY) == pdTRUE)
                {
                    if (ReceivedDataFromQueue.timestamp - prevSDTimeStamp >= SDCardSettings.LoggingPeriod)
                    {
                        e = writetoSDcard(dataFileName, "%u,%.2f,%.2f,%d,%d,%d\n", ReceivedDataFromQueue.timestamp,
                                          ReceivedDataFromQueue.temperature,
                                          ReceivedDataFromQueue.humidity,
                                          ReceivedDataFromQueue.pm1_0,
                                          ReceivedDataFromQueue.pm2_5,
                                          ReceivedDataFromQueue.pm10);
                        prevSDTimeStamp = ReceivedDataFromQueue.timestamp;
                        xSemaphoreGive(SDCardMutex);
                        if (e != ESP_OK)
                        {
                            ESP_LOGE(__func__, "Failed to write data to SD card");
                        }
                        else
                        {
                            ESP_LOGI(__func__, "Write data to SD card successfully");
                        }
                    }
                    else
                    {
                        xSemaphoreGive(SDCardMutex);
                    }
                }
                else
                {
                    ESP_LOGE(__func__, "Failed to take SDCardMutex");
                }
            }
            else
            {
                ESP_LOGE(__func__, "Failed to receive data from DataToSDCardQueue");
            }
        }
        if (uxQueueMessagesWaiting(FirebaseToSDCardQueue) != 0)
        {
            if (xQueueReceive(FirebaseToSDCardQueue, (void *)&ReceivedSettingsFromFirebase, portMAX_DELAY) == pdPASS)
            {
                if (xSemaphoreTake(SDCardMutex, portMAX_DELAY) == pdTRUE)
                {
                    if (ReceivedSettingsFromFirebase.LoggingPeriod != SDCardSettings.LoggingPeriod)
                    {
                        SDCardSettings.LoggingPeriod = ReceivedSettingsFromFirebase.LoggingPeriod;
                    }
                    if (ReceivedSettingsFromFirebase.SDDelete == true)
                    {
                        deleteSDcardData(dataFileName) == ESP_OK ? SDCardSettings.SDDelete = false : SDCardSettings.SDDelete = true;
                        if (xQueueSendToBack(SDCardToFirebaseQueue, (void *)&SDCardSettings, pdMS_TO_TICKS(50)) != pdPASS)
                        {
                            ESP_LOGE(__func__, "Failed to send data to SDCardToFirebaseQueue");
                        }
                        else
                        {
                            ESP_LOGI(__func__, "send data to SDCardToFirebaseQueue successfully");
                        }
                    }
                    xSemaphoreGive(SDCardMutex);
                }
                else
                {
                    ESP_LOGE(__func__, "Failed to take SDCardMutex");
                }
            }
            else
            {
                ESP_LOGE(__func__, "Failed to receive data from FirebaseToSDcardQueue");
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // wait get sensor data task send data to queue
    }
}

static void getTimeFromRTC_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        struct tm rtcinfo;
        if (ds3231_get_time(&ds3231_dev, &rtcinfo) != ESP_OK)
        {
            ESP_LOGE(__func__, "Could not get time.");
            while (1)
            {
                vTaskDelay(1);
            }
        }
        sprintf(CurrentTime, "%02d:%02d:%02d", rtcinfo.tm_hour + 7, rtcinfo.tm_min, rtcinfo.tm_sec);

        // ESP_LOGI(__func__, "%04d-%02d-%02d %02d:%02d:%02d", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday,
        //          rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}
/*----------------------------------*INITIALIZE - PERIPHERALS *--------------------------------*/
void init_sensors(void)
{
    // Initialize SHT31 sensor
    memset(&sht31_dev, 0, sizeof(sht3x_t));

    ESP_ERROR_CHECK(sht3x_init_desc(&sht31_dev,
                                    CONFIG_SHT31_I2C_ADDR,
                                    CONFIG_SHT31_I2C_PORT,
                                    (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SDA,
                                    (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SCL));

    ESP_ERROR_CHECK(sht3x_init(&sht31_dev));

    // Initialize PMS7003 sensor
    uart_config_t pms_uart_config = PMS_UART_CONFIG_DEFAULT();
    if (pms_initUart(&pms_uart_config) != PMS_OK)
    {
        esp_restart();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Create task to get data from sensor
    xTaskCreate(getDataFromSensor_task, "getDataFromSensor_task", (1024 * 16), NULL, 25, &getDataFromSensorTask_handle);
}

void init_sdcard(void)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = MOUNT_CONFIG_DEFAULT();
    sdmmc_card_t *sdcard;
    sdmmc_host_t host = SDSPI_HOST_VSPI();
    spi_bus_config_t bus_cfg = SPI_BUS_CONFIG_DEFAULT();
    ESP_LOGI(TAG, "Initializing SD card using SPI peripheral");
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SPI_DMA_CH2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        sd_status = "No connection";
        return;
    }
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t)CONFIG_SD_PIN_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &sdcard);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        sd_status = "No connection";
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sd_status = "Connected";

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sdcard);

    // Use POSIX and C standard library functions to work with files.
    // if the data.txt file doesn't exist create a file on the SD card and write the data labels
    const char *file_name = MOUNT_POINT "/data.txt";
    struct stat st;
    if (stat(file_name, &st) != 0)
    {
        ESP_LOGI(TAG, "Opening file %s ...", file_name);
        FILE *f = fopen(file_name, "w");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }
        fprintf(f, "Timestamp, Temperature, Humidity, PM2.5, PM1, PM10 \n");
        fclose(f);
        ESP_LOGI(TAG, "File written");
    }
    else
    {
        ESP_LOGI(TAG, "File already exists");
    }
    char *data_str;
    readfromSDcard(file_name, &data_str); // read the data from the SD card for debugging
    free(data_str);                       // free the memory

    xTaskCreate(SDCard_task, "SDCard_task", (1024 * 6), NULL, 9, &SDCardTask_handle);
}

void init_rtc(void)
{
    /* initialize rtc */
    memset(&ds3231_dev, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(ds3231_init_desc(&ds3231_dev, 0, (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SDA, (gpio_num_t)CONFIG_SHT31_I2C_MASTER_SCL));
    xTaskCreate(getTimeFromRTC_task, "getTimeFromRTC_task", configMINIMAL_STACK_SIZE * 4, NULL, 11, &getTimeFromRTCTask_handle);
}

/*----------------------------------*WIFI - CONNECTION*--------------------------------*/
/* wifi callback */
void wifi_disconnect_callback(void *pvParameters)
{
    memset(str_ip, 0, sizeof(str_ip));
    memset(str_mac, 0, sizeof(str_mac));
    wifi_status = "Disconnected";
    firebase_status = "Disconnected";
    vTaskDelete(FirebaseTask_handle);
}

void wifi_connect_ok_callback(void *pvParameter)
{
    ip_event_got_ip_t *param = (ip_event_got_ip_t *)pvParameter;
    /* transform IP to human readable string */
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
    esp_read_mac(mac_adrr, ESP_MAC_WIFI_STA);
    sprintf(str_mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_adrr[0], mac_adrr[1], mac_adrr[2], mac_adrr[3], mac_adrr[4], mac_adrr[5]);
    ESP_LOGI(__func__, "I have a connection and my IP is %s!", str_ip);
    ESP_LOGI(__func__, "My MAC is %s!", str_mac);
    wifi_status = "Connected";
    wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &wifi_disconnect_callback);
    /* initialize ntp*/
    Set_SystemTime_SNTP();
    // update 'now' variable with current time
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    time(&now);
    // now = now + (7 * 60 * 60); // GMT +7
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(__func__, "The current date/time in Viet Nam is: %s", strftime_buf);

    if (ds3231_set_time(&ds3231_dev, &timeinfo) != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not set time.");
    }

    /* initialize firebase*/
    xTaskCreate(Firebase_task, "Firebase", (1024 * 10), NULL, 10, &FirebaseTask_handle);
}

/*----------------------------------*GUI - TASK*--------------------------------*/
SemaphoreHandle_t xGuiSemaphore;
static void guiTask(void *pvParameter)
{
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    lvgl_driver_init(); /* Initialize SPI bus used by the drivers */
    /* Initialize buffer*/
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    /* Initialize the working buffer depending on the selected display.*/
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv; /* Initialize display driver */
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    lv_disp_drv_register(&disp_drv); /* Register the driver */

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    LCDScreen();

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        update_param();

        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    /* A task should NEVER return */
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

void LCDScreen(void)
{
    lv_port_indev_init();
    keypad_group = lv_group_create();
    lv_group_set_default(keypad_group);
    lv_indev_t *cur_drv = NULL;
    for (;;)
    {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv)
        {
            break;
        }
        if (cur_drv->driver->type == LV_INDEV_TYPE_KEYPAD)
        {
            lv_indev_set_group(cur_drv, keypad_group);
        }
    }

    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);
    lv_obj_t *tabhome = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS);
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 1");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 2");
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 3");
    lv_tabview_set_act(tabview, 0, LV_ANIM_ON);
    act_tabhome(tabhome);
    act_tab1(tab1);
    act_tab2(tab2);
    act_tab3(tab3);
}

/*-------------------------------- TAB - HOME--------------------------------*/
void act_tabhome(lv_obj_t *parent)
{
    wifi_label = lv_label_create(parent);
    lv_obj_align(wifi_label, LV_ALIGN_TOP_LEFT, 0, 0);
    mac_adrr_label = lv_label_create(parent);
    lv_obj_align_to(mac_adrr_label, wifi_label, LV_ALIGN_LEFT_MID, 0, 20);
    wifi_status_label = lv_label_create(parent);
    lv_obj_align_to(wifi_status_label, mac_adrr_label, LV_ALIGN_LEFT_MID, 0, 20);
    firebase_status_label = lv_label_create(parent);
    lv_obj_align_to(firebase_status_label, wifi_status_label, LV_ALIGN_LEFT_MID, 0, 20);
    sd_status_label = lv_label_create(parent);
    lv_obj_align_to(sd_status_label, firebase_status_label, LV_ALIGN_LEFT_MID, 0, 20);
}
/*-------------------------------- TAB - 1--------------------------------*/
void act_tab1(lv_obj_t *parent)
{
    static lv_style_t style_shadow;
    lv_style_init(&style_shadow);
    lv_style_set_shadow_width(&style_shadow, 8);
    lv_style_set_shadow_spread(&style_shadow, 5);
    lv_style_set_shadow_color(&style_shadow, lv_palette_main(LV_PALETTE_BLUE));

    lv_obj_t *temp_frame = lv_obj_create(parent);
    lv_obj_set_size(temp_frame, 145, 150);
    lv_obj_add_style(temp_frame, &style_shadow, 0);
    lv_obj_align(temp_frame, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(temp_frame, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *time_frame = lv_obj_create(parent);
    lv_obj_set_size(time_frame, 145, 30);
    lv_obj_add_style(time_frame, &style_shadow, 0);
    lv_obj_align(time_frame, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(time_frame, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *hum_frame = lv_obj_create(parent);
    lv_obj_set_size(hum_frame, 145, 105);
    lv_obj_add_style(hum_frame, &style_shadow, 0);
    lv_obj_align(hum_frame, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_scrollbar_mode(hum_frame, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *pm_frame = lv_obj_create(parent);
    lv_obj_set_size(pm_frame, 145, 75);
    lv_obj_add_style(pm_frame, &style_shadow, 0);
    lv_obj_align(pm_frame, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_scrollbar_mode(pm_frame, LV_SCROLLBAR_MODE_OFF);

    date_label = lv_label_create(time_frame);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 0);

    static lv_style_t txt_style;
    lv_style_init(&txt_style);
    lv_style_set_text_font(&txt_style, &lv_font_montserrat_18);

    /* Temperature */
    static lv_style_t temp_bar_style;

    lv_style_init(&temp_bar_style);
    lv_style_set_bg_opa(&temp_bar_style, LV_OPA_COVER);
    lv_style_set_bg_color(&temp_bar_style, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_color(&temp_bar_style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_dir(&temp_bar_style, LV_GRAD_DIR_VER);

    temp_bar_tab1 = lv_bar_create(temp_frame);
    lv_obj_add_style(temp_bar_tab1, &temp_bar_style, LV_PART_INDICATOR);
    lv_obj_set_size(temp_bar_tab1, 20, 100);
    lv_obj_align(temp_bar_tab1, LV_ALIGN_LEFT_MID, 0, 15);
    lv_bar_set_range(temp_bar_tab1, -20, 40);

    temp_label_tab1 = lv_label_create(temp_frame);
    lv_obj_add_style(temp_label_tab1, &txt_style, 0);
    lv_obj_align(temp_label_tab1, LV_ALIGN_RIGHT_MID, 0, -15);

    /* Humidity*/
    hum_gauge_tab1 = lv_arc_create(hum_frame);
    lv_obj_remove_style(hum_gauge_tab1, NULL, LV_PART_KNOB);
    lv_obj_set_size(hum_gauge_tab1, 105, 105);
    lv_arc_set_rotation(hum_gauge_tab1, 180);
    lv_arc_set_bg_angles(hum_gauge_tab1, 0, 180);
    lv_obj_align(hum_gauge_tab1, LV_ALIGN_CENTER, 0, 30);

    hum_label_tab1 = lv_label_create(hum_frame);
    lv_obj_add_style(hum_label_tab1, &txt_style, 0);
    lv_obj_align_to(hum_label_tab1, hum_gauge_tab1, LV_ALIGN_CENTER, -15, 0);

    /* Dust*/
    pm2p5_label_tab1 = lv_label_create(pm_frame);
    static lv_style_t pm2p5_style;
    lv_style_init(&pm2p5_style);
    lv_style_set_text_font(&pm2p5_style, &lv_font_montserrat_46);
    lv_obj_add_style(pm2p5_label_tab1, &pm2p5_style, 0);
    lv_obj_align(pm2p5_label_tab1, LV_ALIGN_CENTER, -24, 6);

    /* Title*/
    lv_obj_t *temp_title_tab1 = lv_label_create(temp_frame);
    lv_obj_align(temp_title_tab1, LV_ALIGN_TOP_MID, -5, -10);
    lv_label_set_text(temp_title_tab1, "TEMPERATURE");

    lv_obj_t *hum_title_tab1 = lv_label_create(hum_frame);
    lv_obj_align(hum_title_tab1, LV_ALIGN_TOP_MID, 0, -10);
    lv_label_set_text(hum_title_tab1, "HUMIDITY");

    lv_obj_t *pm2p5_title_tab1 = lv_label_create(pm_frame);
    lv_obj_align(pm2p5_title_tab1, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_label_set_text(pm2p5_title_tab1, "PM2.5\n"
                                        "ug/m3");
}
/*-------------------------------- TAB - 2--------------------------------*/
void act_tab2(lv_obj_t *parent)
{
    static lv_style_t indic1_style, indic2_style, indic3_style;
    lv_style_init(&indic1_style);
    lv_style_init(&indic2_style);
    lv_style_init(&indic3_style);

    lv_style_set_text_font(&indic1_style, &lv_font_montserrat_20);
    lv_style_set_text_font(&indic2_style, &lv_font_montserrat_20);
    lv_style_set_text_font(&indic3_style, &lv_font_montserrat_20);

    lv_obj_t *pm_label_title = lv_label_create(parent);
    lv_label_set_text(pm_label_title, "PARTICLE MATTER (ug/m3)");
    lv_obj_align(pm_label_title, LV_ALIGN_TOP_MID, 0, 0);

    pm2p5_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm2p5_label_tab2, &indic1_style, 0);
    lv_obj_align(pm2p5_label_tab2, LV_ALIGN_LEFT_MID, 196, -30);
    lv_style_set_text_color(&indic1_style, lv_palette_main(LV_PALETTE_RED));

    pm10_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm10_label_tab2, &indic2_style, 0);
    lv_obj_align(pm10_label_tab2, LV_ALIGN_LEFT_MID, 196, 0);
    lv_style_set_text_color(&indic2_style, lv_palette_main(LV_PALETTE_GREEN));

    pm1_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm1_label_tab2, &indic3_style, 0);
    lv_obj_align(pm1_label_tab2, LV_ALIGN_LEFT_MID, 196, 30);
    lv_style_set_text_color(&indic3_style, lv_palette_main(LV_PALETTE_BLUE));

    pm_meter_tab2 = lv_meter_create(parent);
    lv_obj_align(pm_meter_tab2, LV_ALIGN_LEFT_MID, 0, 10);
    lv_obj_set_size(pm_meter_tab2, 160, 160);

    /*Remove the circle from the middle*/
    lv_obj_remove_style(pm_meter_tab2, NULL, LV_PART_INDICATOR);

    /*Add a scale first*/
    lv_meter_scale_t *scale = lv_meter_add_scale(pm_meter_tab2);
    lv_meter_set_scale_ticks(pm_meter_tab2, scale, 21, 2, 12, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(pm_meter_tab2, scale, 4, 2, 30, lv_color_hex3(0xeee), 10);
    lv_meter_set_scale_range(pm_meter_tab2, scale, 0, 115, 240, 150);

    /*Add a three arc indicator*/
    indic_pm2p5_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
    indic_pm10_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_GREEN), -10);
    indic_pm1_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_BLUE), -20);
}
/*-------------------------------- TAB - 3--------------------------------*/
void act_tab3(lv_obj_t *parent)
{
    lv_obj_t *pm_label_title = lv_label_create(parent);
    lv_label_set_text(pm_label_title, "TEMPERATURE & HUMIDITY");
    lv_obj_align(pm_label_title, LV_ALIGN_TOP_MID, 0, 0);

    /*Add a meter for temperature*/
    temp_meter_tab3 = lv_meter_create(parent);
    lv_obj_align(temp_meter_tab3, LV_ALIGN_LEFT_MID, 0, 12);
    lv_obj_set_size(temp_meter_tab3, 150, 150);

    lv_meter_scale_t *temp_scale_tab3 = lv_meter_add_scale(temp_meter_tab3);
    lv_meter_set_scale_ticks(temp_meter_tab3, temp_scale_tab3, 21, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(temp_meter_tab3, temp_scale_tab3, 5, 4, 15, lv_color_black(), 10);

    /*Add a light blue arc to the start*/
    temp_indic_tab3 = lv_meter_add_arc(temp_meter_tab3, temp_scale_tab3, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 0);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 15);

    /*Make the tick lines light blue at the start of the scale*/
    temp_indic_tab3 = lv_meter_add_scale_lines(temp_meter_tab3, temp_scale_tab3, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE),
                                               false, 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 0);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 15);

    /*Add a green arc in the midddle*/
    temp_indic_tab3 = lv_meter_add_arc(temp_meter_tab3, temp_scale_tab3, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 15);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 30);

    /*Make the tick lines blue at the middle of the scale*/
    temp_indic_tab3 = lv_meter_add_scale_lines(temp_meter_tab3, temp_scale_tab3, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN), false,
                                               0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 15);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 30);

    /*Add a red arc to the end*/
    temp_indic_tab3 = lv_meter_add_arc(temp_meter_tab3, temp_scale_tab3, 3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 30);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 40);

    /*Make the tick lines red at the end of the scale*/
    temp_indic_tab3 = lv_meter_add_scale_lines(temp_meter_tab3, temp_scale_tab3, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,
                                               0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 30);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 40);

    /*Add a needle line indicator*/
    temp_indic_tab3 = lv_meter_add_needle_line(temp_meter_tab3, temp_scale_tab3, 2, lv_palette_main(LV_PALETTE_GREY), -10);

    /*Set temperature scale range*/
    lv_meter_set_scale_range(temp_meter_tab3, temp_scale_tab3, 0, 40, 240, 150);

    hum_meter_tab3 = lv_meter_create(parent);
    lv_obj_align(hum_meter_tab3, LV_ALIGN_RIGHT_MID, 0, 12);
    lv_obj_set_size(hum_meter_tab3, 150, 150);

    /*Add a scale first*/
    lv_meter_scale_t *hum_scale_tab3 = lv_meter_add_scale(hum_meter_tab3);
    lv_meter_set_scale_ticks(hum_meter_tab3, hum_scale_tab3, 21, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(hum_meter_tab3, hum_scale_tab3, 5, 4, 15, lv_color_black(), 10);

    /*Add a blue arc to the start*/
    hum_indic_tab3 = lv_meter_add_arc(hum_meter_tab3, hum_scale_tab3, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 0);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 40);

    /*Make the tick lines blue at the start of the scale*/
    hum_indic_tab3 = lv_meter_add_scale_lines(hum_meter_tab3, hum_scale_tab3, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE),
                                              false, 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 0);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 40);

    /*Add a green arc in the midddle*/
    hum_indic_tab3 = lv_meter_add_arc(hum_meter_tab3, hum_scale_tab3, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 40);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 80);

    /*Make the tick lines green at the start of the scale*/
    hum_indic_tab3 = lv_meter_add_scale_lines(hum_meter_tab3, hum_scale_tab3, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN),
                                              false, 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 40);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 80);

    /*Add a red arc to the end*/
    hum_indic_tab3 = lv_meter_add_arc(hum_meter_tab3, hum_scale_tab3, 3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 80);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 100);

    /*Make the tick lines red at the end of the scale*/
    hum_indic_tab3 = lv_meter_add_scale_lines(hum_meter_tab3, hum_scale_tab3, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,
                                              0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 80);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 100);

    /*Add a needle line indicator*/
    hum_indic_tab3 = lv_meter_add_needle_line(hum_meter_tab3, hum_scale_tab3, 2, lv_palette_main(LV_PALETTE_GREY), -10);

    /*Set humidity scale range*/
    lv_meter_set_scale_range(hum_meter_tab3, hum_scale_tab3, 0, 100, 240, 150);

    static lv_style_t style_label_tab3;
    lv_style_init(&style_label_tab3);
    lv_style_set_text_font(&style_label_tab3, &lv_font_montserrat_20);

    /*label for temperature value*/
    temp_label_tab3 = lv_label_create(parent);
    lv_obj_add_style(temp_label_tab3, &style_label_tab3, 0);
    lv_obj_align(temp_label_tab3, LV_ALIGN_BOTTOM_MID, -70, -24);

    /*label for temperature value*/
    hum_label_tab3 = lv_label_create(parent);
    lv_obj_add_style(hum_label_tab3, &style_label_tab3, 0);
    lv_obj_align(hum_label_tab3, LV_ALIGN_BOTTOM_MID, 70, -24);
}

void update_param(void)
{
    lv_label_set_text_fmt(wifi_label, "WIFI ip address: %s", str_ip);
    lv_label_set_text_fmt(mac_adrr_label, "MAC address: %s", str_mac);
    lv_label_set_text_fmt(wifi_status_label, "Wifi status: %s", wifi_status);
    lv_label_set_text_fmt(firebase_status_label, "Firebase status: %s", firebase_status);
    lv_label_set_text_fmt(sd_status_label, "SD card status: %s", sd_status);
    /* tab 1*/
    lv_label_set_text_fmt(date_label, "%s", CurrentTime);

    char temp_update[10];
    char hum_update[10];
    lv_bar_set_value(temp_bar_tab1, currentDataSensor.temperature, LV_ANIM_ON);
    sprintf(temp_update, "%.2f °C", currentDataSensor.temperature);
    lv_label_set_text(temp_label_tab1, temp_update);

    lv_arc_set_value(hum_gauge_tab1, currentDataSensor.humidity);
    sprintf(hum_update, "%.2f %%", currentDataSensor.humidity);
    lv_label_set_text(hum_label_tab1, hum_update);

    lv_label_set_text_fmt(pm2p5_label_tab1, "%d", currentDataSensor.pm2_5);

    /* tab 2*/
    lv_label_set_text_fmt(pm2p5_label_tab2, "PM2.5: %d", currentDataSensor.pm2_5);
    lv_label_set_text_fmt(pm10_label_tab2, "PM10: %d", currentDataSensor.pm10);
    lv_label_set_text_fmt(pm1_label_tab2, "PM1: %d", currentDataSensor.pm1_0);

    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm2p5_tab2, currentDataSensor.pm2_5);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm10_tab2, currentDataSensor.pm10);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm1_tab2, currentDataSensor.pm1_0);

    /* tab 3*/
    lv_meter_set_indicator_value(temp_meter_tab3, temp_indic_tab3, currentDataSensor.temperature);
    lv_label_set_text(temp_label_tab3, temp_update);

    lv_meter_set_indicator_value(hum_meter_tab3, hum_indic_tab3, currentDataSensor.humidity);
    lv_label_set_text(hum_label_tab3, hum_update);
}

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

extern "C" void app_main(void)
{
    // Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI("ESP_info", "Minimum free heap size: %d bytes\r\n", esp_get_minimum_free_heap_size());
    // Booting firmware
    ESP_LOGI(__func__, "Booting....");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    init_btn(); // press for 2 seconds to reset wifi config and reset esp32

    /* start wifi manager*/
    wifi_manager_start();
    /* register a callback when connection is success*/
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &wifi_connect_ok_callback);

    /* initialize lvgl */
    xTaskCreatePinnedToCore(guiTask, "gui", (1024 * 4), NULL, 5, NULL, 1);

    ESP_ERROR_CHECK(i2cdev_init());
    /* initialize sensor */
    init_sensors();
    /* initialize rtc */
    init_rtc();

    // Create data send queue
    DataToSDCardQueue = xQueueCreate(10, sizeof(struct dataSensor_st));
    DataToFirebaseQueue = xQueueCreate(10, sizeof(struct dataSensor_st));
    FirebaseToSDCardQueue = xQueueCreate(2, sizeof(struct FBSettings_st));
    SDCardToFirebaseQueue = xQueueCreate(2, sizeof(struct FBSettings_st));

    /* initialize sdcard */
    init_sdcard();
}