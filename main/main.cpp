#include "main.h"

__attribute__((unused)) static const char TAG[] = "main";

/*--------------------------LVGL-------------------------------*/
#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
void screen1(void);
void act_tabhome(lv_obj_t * parent);
void act_tab1(lv_obj_t * parent);
void act_tab2(lv_obj_t * parent);
void act_tab3(lv_obj_t * parent);
void update_param(void);

char CurrentTime[20];
lv_obj_t * wifi_label;
/*--------------------------Indicate variable in tab 1-------------------------------*/
lv_obj_t * temp_bar_tab1;
lv_obj_t * temp_label_tab1;

lv_obj_t * hum_gauge_tab1;
lv_obj_t * hum_label_tab1;

lv_obj_t * pm2p5_label_tab1;
lv_obj_t * pm10_label_tab1;
lv_obj_t * pm1_label_tab1;

lv_obj_t * date_label;
/*--------------------------Indicate variable in tab 2-------------------------------*/
lv_obj_t * pm2p5_label_tab2;
lv_obj_t * pm10_label_tab2;
lv_obj_t * pm1_label_tab2;
static lv_obj_t * pm_meter_tab2;
lv_meter_indicator_t * indic_pm2p5_tab2;
lv_meter_indicator_t * indic_pm10_tab2;
lv_meter_indicator_t * indic_pm1_tab2;
/*--------------------------Indicate variable in tab 3-------------------------------*/
static lv_obj_t * temp_meter_tab3;
static lv_obj_t * hum_meter_tab3;
lv_meter_indicator_t * temp_indic_tab3;
lv_meter_indicator_t * hum_indic_tab3;


static lv_group_t * keypad_group;


lv_indev_t * indev_keypad;

/*--------------------------MAIN-------------------------------*/
using namespace ESPFirebase;
char str_ip[16]; // IP address

#define PMS7003_PERIOD_MS 30000

static button_t rst_btn3;
static void button_cb(button_t *btn, button_state_t state)
{
    if(btn == &rst_btn3 && state == BUTTON_PRESSED_LONG)
    {
        wifi_manager_disconnect_async();
        esp_restart();
    }
}

void reset_btn(void)
{
    rst_btn3.gpio = (gpio_num_t)CONFIG_EXAMPLE_BUTTON3_GPIO;
    rst_btn3.pressed_level = 0;
    rst_btn3.internal_pull = true;
    rst_btn3.autorepeat = false;
    rst_btn3.callback = button_cb;
    ESP_ERROR_CHECK(button_init(&rst_btn3));
}


TaskHandle_t getDataFromSensorTask_handle = NULL;
TaskHandle_t sendDataToFirebaseTask_handle = NULL;
TaskHandle_t saveDataSensorToSDCardTask_handle = NULL;
TaskHandle_t GUITask_handle = NULL;

// Timer handle for sensor
TimerHandle_t pms7003_timer = NULL;
TimerHandle_t Date_timer = NULL;

// Mutex for protecting currentData
SemaphoreHandle_t currentDataMutex = NULL;
SemaphoreHandle_t writeDataToSDCardMutex = NULL;

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
uint8_t restart_esp = 0;
// pms_wake_status of PMS7003 sensor (TRUE = Wake up, FALSE = Sleep)
bool pms_wake_status = false;

// Date timer callback
void Date_timer_callback(TimerHandle_t xTimer)
{
    getDate(CurrentTime);
}

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
            restart_esp += 1;
            if(restart_esp == 3)
            {
                esp_restart();
            }

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
            //printf("Time: %d \n", timeStamp);
        }

        // wait until 10 seconds are over
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(30000));
    }

    // Delete task, can not reach here
    vTaskDelete(NULL);
}

static void sendDataToFirebase_task(void *pvParameters)
{
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
        timeStamp = getTime();
        data["Temperature"] = dataSensor.temperature;
        data["Humidity"] = dataSensor.humidity;
        data["PM1_0"] = dataSensor.pm1_0;
        data["PM2_5"] = dataSensor.pm2_5;
        data["PM10"] = dataSensor.pm10;
        data["Timestamp"] = timeStamp;
        std::string path = databasePath + "/" + std::to_string(timeStamp);
        db.putData(path.c_str(), data);
        // wait until 10 seconds are over
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    // Delete task, can not reach here
    vTaskDelete(NULL);
}

void init_sensors(void)
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
    xTaskCreate(getDataFromSensor_task, "getDataFromSensor_task", (1024 * 48), NULL, 25, &getDataFromSensorTask_handle);
}

// void init_sdcard(void)
// {
//     // Initialize SPI Bus
//     ESP_LOGI(__func__, "Initialize SD card with SPI interface.");
//     esp_vfs_fat_mount_config_t  mount_config_t   = MOUNT_CONFIG_DEFAULT();
//     spi_bus_config_t            spi_bus_config_t = SPI_BUS_CONFIG_DEFAULT();
//     sdmmc_host_t                host_t           = SDSPI_HOST_DEFAULT();
//     sdspi_device_config_t       slot_config      = SDSPI_DEVICE_CONFIG_DEFAULT();
//     slot_config.gpio_cs = CONFIG_PIN_NUM_CS;
//     slot_config.host_id = host_t.slot;

//     sdmmc_card_t SDCARD;
//     ESP_ERROR_CHECK_WITHOUT_ABORT(sdcard_initialize(&mount_config_t, &SDCARD, &host_t, &spi_bus_config_t, &slot_config));
// }

// void init_rtc(void)
// {
//     ESP_LOGI(__func__, "Initialize DS3231 module(I2C/Wire%d).", CONFIG_RTC_I2C_PORT);
//     memset(&ds3231_device, 0, sizeof(i2c_dev_t));
//     ESP_ERROR_CHECK_WITHOUT_ABORT(ds3231_initialize(&ds3231_device, 
//                                                     CONFIG_RTC_I2C_PORT, 
//                                                     CONFIG_RTC_PIN_NUM_SDA, 
//                                                     CONFIG_RTC_PIN_NUM_SCL));
// }

/*----------------------------------*WIFI-CONNECTION*--------------------------------*/

/* wifi callback */
void wifi_connect_ok_callback(void *pvParameter)
{
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGI(__func__, "I have a connection and my IP is %s!", str_ip);
    /* initialize ntp*/
    Set_SystemTime_SNTP();
    /* initialize sensor */
    init_sensors();
    /* initialize firebase*/
    xTaskCreate(sendDataToFirebase_task, "sendDataToFirebase", (1024 * 10), NULL, 10, &sendDataToFirebaseTask_handle);
}

/*----------------------------------*GUI - TASK*--------------------------------*/
SemaphoreHandle_t xGuiSemaphore;
static void guiTask(void *pvParameter) {

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    lvgl_driver_init(); /* Initialize SPI or I2C bus used by the drivers */
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
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    
    //screen1();

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        //update_param();

        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    /* A task should NEVER return */
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

void screen1(void)
{
    lv_port_indev_init();
    keypad_group = lv_group_create();
    lv_group_set_default(keypad_group);
    lv_indev_t* cur_drv = NULL;
    for (;;) {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv) {
            break;
        }
        if (cur_drv->driver->type == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(cur_drv, keypad_group);
        }
    }

    lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);
    lv_obj_t * tabhome = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS);
    lv_obj_t * tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 1");
    lv_obj_t * tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 2");
    lv_obj_t * tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_EYE_OPEN " 3");
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
    lv_label_set_text_fmt(wifi_label, "WIFI ip address: %s", str_ip);
    lv_obj_align(wifi_label, LV_ALIGN_TOP_LEFT, 0, 0);

}

/*-------------------------------- TAB - 1--------------------------------*/
void act_tab1(lv_obj_t * parent)
{
    static lv_style_t style_shadow;
    lv_style_init(&style_shadow);
    lv_style_set_shadow_width(&style_shadow, 8);
    lv_style_set_shadow_spread(&style_shadow, 5);
    lv_style_set_shadow_color(&style_shadow, lv_palette_main(LV_PALETTE_BLUE));

    lv_obj_t * temp_frame = lv_obj_create(parent);
    lv_obj_set_size(temp_frame, 145, 150);
    lv_obj_add_style(temp_frame, &style_shadow, 0);
    lv_obj_align(temp_frame, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(temp_frame, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * time_frame = lv_obj_create(parent);
    lv_obj_set_size(time_frame, 145, 30);
    lv_obj_add_style(time_frame, &style_shadow, 0);
    lv_obj_align(time_frame, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(time_frame, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * hum_frame = lv_obj_create(parent);
    lv_obj_set_size(hum_frame, 145, 105);
    lv_obj_add_style(hum_frame, &style_shadow, 0);
    lv_obj_align(hum_frame, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_scrollbar_mode(hum_frame, LV_SCROLLBAR_MODE_OFF);
    
    lv_obj_t * pm_frame = lv_obj_create(parent);
    lv_obj_set_size(pm_frame, 145, 75);
    lv_obj_add_style(pm_frame, &style_shadow, 0);
    lv_obj_align(pm_frame, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_scrollbar_mode(pm_frame, LV_SCROLLBAR_MODE_OFF);

    date_label = lv_label_create(time_frame);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text_fmt(date_label, "%s", CurrentTime);

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
    lv_bar_set_value(temp_bar_tab1, dataSensor.temperature, LV_ANIM_ON);

    temp_label_tab1 = lv_label_create(temp_frame);
    lv_obj_add_style(temp_label_tab1, &txt_style, 0);
    lv_obj_align(temp_label_tab1, LV_ALIGN_RIGHT_MID, 0, -15);
    lv_label_set_text_fmt(temp_label_tab1, " %.2f °C", dataSensor.temperature);

    /* Humidity*/
    hum_gauge_tab1 = lv_arc_create(hum_frame);
    lv_obj_remove_style(hum_gauge_tab1, NULL, LV_PART_KNOB);
    lv_obj_set_size(hum_gauge_tab1, 105, 105);
    lv_arc_set_rotation(hum_gauge_tab1, 180);
    lv_arc_set_bg_angles(hum_gauge_tab1, 0, 180);
    lv_obj_align(hum_gauge_tab1, LV_ALIGN_CENTER, 0, 30);
    lv_arc_set_value(hum_gauge_tab1, dataSensor.humidity);

    hum_label_tab1 = lv_label_create(hum_frame);
    lv_obj_add_style(hum_label_tab1, &txt_style, 0);
    lv_obj_align_to(hum_label_tab1, hum_gauge_tab1, LV_ALIGN_CENTER, -15, 0);
    lv_label_set_text_fmt(hum_label_tab1, " %.2f %%", dataSensor.humidity);

    pm2p5_label_tab1 = lv_label_create(pm_frame);
    lv_obj_align(pm2p5_label_tab1, LV_ALIGN_LEFT_MID, 0, -15);
    lv_label_set_recolor(pm2p5_label_tab1, true);
    lv_label_set_text_fmt(pm2p5_label_tab1, "#FE3301 PM2.5: %d \tug/m3#", dataSensor.pm2_5);

    pm10_label_tab1 = lv_label_create(pm_frame);
    lv_obj_align(pm10_label_tab1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_recolor(pm10_label_tab1, true);
    lv_label_set_text_fmt(pm10_label_tab1, "#01FE01 PM10: %d \tug/m3#", dataSensor.pm10);

    pm1_label_tab1 = lv_label_create(pm_frame);
    lv_obj_align(pm1_label_tab1, LV_ALIGN_LEFT_MID, 0, 15);
    lv_label_set_recolor(pm1_label_tab1, true);
    lv_label_set_text_fmt(pm1_label_tab1, "#0180FE PM1: %d \t\tug/m3#", dataSensor.pm1_0);

    /* Title*/
    lv_obj_t * temp_title_tab1 = lv_label_create(temp_frame);
    lv_obj_align(temp_title_tab1, LV_ALIGN_TOP_MID, -5, -10);
    lv_label_set_text(temp_title_tab1, "TEMPERATURE");
    
    lv_obj_t * hum_title_tab1 = lv_label_create(hum_frame);
    lv_obj_align(hum_title_tab1, LV_ALIGN_TOP_MID, 0, -10);
    lv_label_set_text(hum_title_tab1, "HUMIDITY");



    // lv_obj_t * pm_title_tab1 = lv_label_create(pm_frame);
    // lv_obj_align(pm_title_tab1, LV_ALIGN_TOP_MID, 0, 0);
    // lv_label_set_text(pm_title_tab1, "PARTICLE MATTER");
}
/*-------------------------------- TAB - 2--------------------------------*/
void act_tab2(lv_obj_t * parent)
{
    static lv_style_t indic1_style, indic2_style, indic3_style;
    lv_style_init(&indic1_style);
    lv_style_init(&indic2_style);
    lv_style_init(&indic3_style);
    
    lv_style_set_text_font(&indic1_style, &lv_font_montserrat_14);
    lv_style_set_text_font(&indic2_style, &lv_font_montserrat_14);
    lv_style_set_text_font(&indic3_style, &lv_font_montserrat_14);
    
    pm2p5_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm2p5_label_tab2, &indic1_style, 0);
    lv_obj_align(pm2p5_label_tab2, LV_ALIGN_RIGHT_MID, 0, -30);
    lv_style_set_text_color(&indic1_style, lv_palette_main(LV_PALETTE_RED));
    lv_label_set_text_fmt(pm2p5_label_tab2, "PM2.5: %d \tug/m3", dataSensor.pm2_5);

    pm10_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm10_label_tab2, &indic2_style, 0);
    lv_obj_align(pm10_label_tab2, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_style_set_text_color(&indic2_style, lv_palette_main(LV_PALETTE_GREEN));
    lv_label_set_text_fmt(pm10_label_tab2, "PM10: %d \tug/m3", dataSensor.pm10);

    pm1_label_tab2 = lv_label_create(parent);
    lv_obj_add_style(pm1_label_tab2, &indic3_style, 0);
    lv_obj_align(pm1_label_tab2, LV_ALIGN_RIGHT_MID, 0, 30);
    lv_style_set_text_color(&indic3_style, lv_palette_main(LV_PALETTE_BLUE));
    lv_label_set_text_fmt(pm1_label_tab2, "PM1: %d \tug/m3", dataSensor.pm1_0);

    pm_meter_tab2 = lv_meter_create(parent);
    lv_obj_align(pm_meter_tab2, LV_ALIGN_CENTER, -70, -5);
    lv_obj_set_size(pm_meter_tab2, 160, 160);

    /*Remove the circle from the middle*/
    lv_obj_remove_style(pm_meter_tab2, NULL, LV_PART_INDICATOR);

    /*Add a scale first*/
    lv_meter_scale_t * scale = lv_meter_add_scale(pm_meter_tab2);
    lv_meter_set_scale_ticks(pm_meter_tab2, scale, 2, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(pm_meter_tab2, scale, 1, 2, 30, lv_color_hex3(0xeee), 15);
    lv_meter_set_scale_range(pm_meter_tab2, scale, 0, 50, 270, 90);

    /*Add a three arc indicator*/
    indic_pm2p5_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
    indic_pm10_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_GREEN), -10);
    indic_pm1_tab2 = lv_meter_add_arc(pm_meter_tab2, scale, 10, lv_palette_main(LV_PALETTE_BLUE), -20);

    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm2p5_tab2, dataSensor.pm2_5);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm10_tab2, dataSensor.pm10);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm1_tab2, dataSensor.pm1_0);
}
/*-------------------------------- TAB - 3--------------------------------*/
void act_tab3(lv_obj_t * parent)
{
    temp_meter_tab3 = lv_meter_create(parent);
    lv_obj_align(temp_meter_tab3, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(temp_meter_tab3, 150, 150);

    /*Add a scale first*/
    lv_meter_scale_t * temp_scale_tab3 = lv_meter_add_scale(temp_meter_tab3);
    lv_meter_set_scale_ticks(temp_meter_tab3, temp_scale_tab3, 6, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(temp_meter_tab3, temp_scale_tab3, 6, 4, 15, lv_color_black(), 10);

    /*Add a blue arc to the start*/
    temp_indic_tab3 = lv_meter_add_arc(temp_meter_tab3, temp_scale_tab3, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 0);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 10);

    /*Make the tick lines blue at the start of the scale*/
    temp_indic_tab3 = lv_meter_add_scale_lines(temp_meter_tab3, temp_scale_tab3, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE),
                                     false, 0);
    lv_meter_set_indicator_start_value(temp_meter_tab3, temp_indic_tab3, 0);
    lv_meter_set_indicator_end_value(temp_meter_tab3, temp_indic_tab3, 10);

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



    hum_meter_tab3 = lv_meter_create(parent);
    lv_obj_align(hum_meter_tab3, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(hum_meter_tab3, 150, 150);

    /*Add a scale first*/
    lv_meter_scale_t * hum_scale_tab3 = lv_meter_add_scale(hum_meter_tab3);
    lv_meter_set_scale_ticks(hum_meter_tab3, hum_scale_tab3, 6, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(hum_meter_tab3, hum_scale_tab3, 6, 4, 15, lv_color_black(), 10);

    /*Add a blue arc to the start*/
    hum_indic_tab3 = lv_meter_add_arc(hum_meter_tab3, hum_scale_tab3, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 0);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 20);

    /*Make the tick lines blue at the start of the scale*/
    hum_indic_tab3 = lv_meter_add_scale_lines(hum_meter_tab3, hum_scale_tab3, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE),
                                     false, 0);
    lv_meter_set_indicator_start_value(hum_meter_tab3, hum_indic_tab3, 0);
    lv_meter_set_indicator_end_value(hum_meter_tab3, hum_indic_tab3, 20);

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
}

void update_param(void)
{
    lv_label_set_text_fmt(wifi_label, "WIFI ip address: %s", str_ip);
    /* tab 1*/
    lv_label_set_text_fmt(date_label, "%s", CurrentTime);
    
    char temp_update[10];
    char hum_update[10];
    lv_bar_set_value(temp_bar_tab1, dataSensor.temperature, LV_ANIM_ON);
    sprintf(temp_update, "%.2f °C", dataSensor.temperature);
    lv_label_set_text(temp_label_tab1, temp_update);

    lv_arc_set_value(hum_gauge_tab1, dataSensor.humidity);
    sprintf(hum_update, "%.2f %%", dataSensor.humidity);
    lv_label_set_text(hum_label_tab1, hum_update);

    lv_label_set_text_fmt(pm2p5_label_tab1, "#FE3301 PM2.5: %d \tug/m3#", dataSensor.pm2_5);
    lv_label_set_text_fmt(pm10_label_tab1, "#01FE01 PM10: %d \tug/m3#", dataSensor.pm10);
    lv_label_set_text_fmt(pm1_label_tab1, "#0180FE PM1: %d \tug/m3#", dataSensor.pm1_0);
    
    /* tab 2*/
    lv_label_set_text_fmt(pm2p5_label_tab2, "PM2.5: %d \tug/m3", dataSensor.pm2_5);
    lv_label_set_text_fmt(pm10_label_tab2, "PM10: %d \tug/m3", dataSensor.pm10);
    lv_label_set_text_fmt(pm1_label_tab2, "PM1: %d \tug/m3", dataSensor.pm1_0);

    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm2p5_tab2, dataSensor.pm2_5);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm10_tab2, dataSensor.pm10);
    lv_meter_set_indicator_end_value(pm_meter_tab2, indic_pm1_tab2, dataSensor.pm1_0);

    /* tab 3*/
    lv_meter_set_indicator_value(temp_meter_tab3, temp_indic_tab3, dataSensor.temperature);
    lv_meter_set_indicator_value(hum_meter_tab3, hum_indic_tab3, dataSensor.humidity);
}

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

extern "C" void app_main(void)
{
    
    reset_btn();
        
    // Initialize software timer to update date
    Date_timer = xTimerCreate("Date_timer", 1000 / portTICK_PERIOD_MS, pdTRUE, (void *) 0, Date_timer_callback);
    if(Date_timer != NULL)
    {
        xTimerStart(Date_timer, 0);
    }
    /* start wifi manager*/
    wifi_manager_start();
    /* register a callback when connection is success*/
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &wifi_connect_ok_callback);
    xTaskCreatePinnedToCore(guiTask, "gui", (1024 * 4), NULL, 5, NULL, 1);

    //wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &wifi_disconnect_callback);
    
    /* initialize sdcard */
    //init_sdcard();
    /* initialize rtc */
    //init_rtc();

    // // Create dataSensorQueue
    // dataSensorSentToSD_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    // while (dataSensorSentToSD_queue == NULL)
    // {
    //     ESP_LOGE(__func__, "Create dataSensorSentToSD Queue failed.");
    //     ESP_LOGI(__func__, "Retry to create dataSensorSentToSD Queue...");
    //     vTaskDelay(500 / portTICK_RATE_MS);
    //     dataSensorSentToSD_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    // };
    // ESP_LOGI(__func__, "Create dataSensorSentToSD Queue success.");

}
