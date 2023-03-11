/**
 * @file DeviceManager.h
 * @author Nguyen Nhu Hai Long 
 * @brief Manager all device 
 * @version 0.1
 * @date 2022-11-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __DEVICEMANAGER_H__
#define __DEVICEMANAGER_H__

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "string.h"
#include "time.h"


typedef enum {
    DISCONNECTED = 0,
    CONNECTED,
    CONNECTING,
    NOT_FOUND,
} status_t;

struct statusDevice_st
{
    status_t wifi;
    status_t sht31Sensor;
    status_t pms7003Sensor;
};

struct moduleError_st
{
    esp_err_t sht31Error;
    esp_err_t pms7003Error;
};


#endif
