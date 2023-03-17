#ifndef __SNTP_SYNC_H__
#define __SNTP_SYNC_H__

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sntp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current date time
 * 
 * @param date_time
 * @return void
 */
uint32_t getTime(void);

/**
* @brief Initialize SNTP
*
* @return void
*/
void sntp_init_func(void);

/**
 * @brief Get time
 * 
 * @return void
 */
void obtainTime(void);

/**
 * @brief Check sync
 * 
 * @return void
 */
void Set_SystemTime_SNTP(void);

// /**
//  * @brief Initialize SNTP
//  * 
//  * @return esp_err_t 
//  */
// void sntp_init_func(void);

// /**
//  * @brief 
//  * 
//  * @param timeInfo 
//  * @param time 
//  * @return network time
//  */
// uint32_t sntp_getTime(void);

// /**
//  * @brief  Show Viet Nam time
//  * 
//  * @return esp_err_t 
//  */
// esp_err_t show_Vie_time(void);

#ifdef __cplusplus
}
#endif 

#endif