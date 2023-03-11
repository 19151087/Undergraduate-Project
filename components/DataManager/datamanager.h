#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

// data structure for storing sensor data
typedef struct dataSensor_st {
    // data from SHT31 sensor
    float temperature;
    float humidity;

    // data from PMS7003 sensor
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
} dataSensor_st;

#endif