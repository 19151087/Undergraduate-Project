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
