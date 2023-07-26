// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
// #include <cmath>

// ESP-IDF includes
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/spi_common.h"
#include "esp_spi_flash.h"
#include "esp_mac.h"
#include "esp_chip_info.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"

// External libraries
#include "sdcard.h"
#include "button.h"
#include "pms7003.h"
#include "sht3x.h"
#include "wifi_manager.h"
#include "jsoncpp/value.h"
#include "jsoncpp/json.h"
#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "firebase_config.h"
#include "sntp_sync.h"
#include "ds3231.h"

#include "lvgl.h"
#include "lvgl_helpers.h"
#include "lv_port_indev.h"

// data structure for storing sensor data
typedef struct dataSensor_st
{
    // data from SHT31 sensor
    float temperature;
    float humidity;

    // data from PMS7003 sensor
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;

    // timestamp
    uint32_t timestamp;
} dataSensor_st;

/*Firebase settings variables*/
typedef struct FBSettings_st
{
    bool SDDelete;
    int8_t LoggingPeriod;
} FBSettings_st;

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
lv_obj_t *wifi_ip_label;
lv_obj_t *mac_adrr_label;
lv_obj_t *wifi_ssid_label;
lv_obj_t *sd_status_label;
lv_obj_t *firebase_status_label;
char sd_status[] = "Connecting...";
char firebase_status[] = "Connecting...";
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
char wifi_ssid[32];
char str_ip[16];     // IP address
uint8_t mac_adrr[6]; // MAC address
char str_mac[18];    // string MAC address

extern wifi_config_t *wifi_manager_config_sta;
wifi_config_t *wificonfig = NULL;

static button_t rst_esp_btn1;
static button_t rst_wifi_btn3;

TaskHandle_t CollectDataTask_handle = NULL;
TaskHandle_t FirebaseTask_handle = NULL;
TaskHandle_t SDCardTask_handle = NULL;
TaskHandle_t GUITask_handle = NULL;
TaskHandle_t getDateTimeFromRTCTask_handle = NULL;

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