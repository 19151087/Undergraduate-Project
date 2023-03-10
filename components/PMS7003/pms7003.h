#ifndef __PMS7003_H__
#define __PMS7003_H__

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define START_CHARACTER_1   0x42
#define START_CHARACTER_2   0x4d
#define RX_BUFFER_SIZE      32

#define PMS7003_COMMAND_SIZE    7

#define ID_PMS7003  0X02

#define ESP_ERROR_PMS7003_INIT_UART_FAILED          ((ID_PMS7003 << 12)|(0x00))
#define ESP_ERROR_PMS7003_SET_ACTIVE_MODE_FAILED    ((ID_PMS7003 << 12)|(0x01))
#define ESP_ERROR_PMS7003_READ_DATA_FAILED          ((ID_PMS7003 << 12)|(0x02))
#define ESP_ERROR_PMS7003_SET_PASSIVE_MODE_FAILED   ((ID_PMS7003 << 12)|(0x03))
#define ESP_ERROR_PMS7003_REQUEST_READ_DATA_FAILED  ((ID_PMS7003 << 12)|(0x04))
#define ESP_ERROR_PMS7003_SET_SLEEP_MODE_FAILED     ((ID_PMS7003 << 12)|(0x05))
#define ESP_ERROR_PMS7003_WAKEUP_FAILED             ((ID_PMS7003 << 12)|(0x06))

#define PMS_ERROR_INVALID_VALUE                     UINT16_MAX

#define UART_CONFIG_DEFAULT()   {   .baud_rate = CONFIG_UART_BAUD_RATE,     \
                                    .data_bits = UART_DATA_8_BITS,          \
                                    .parity = UART_PARITY_DISABLE,          \
                                    .stop_bits = UART_STOP_BITS_1,          \
                                    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  \
                                    .source_clk = UART_SCLK_APB,            \
}

static const char PMS7003_COMMAND_WAKEUP[] =        { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
static const char PMS7003_COMMAND_SLEEP[] =         { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
static const char PMS7003_COMMAND_SET_PASSIVE[] =   { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
static const char PMS7003_COMMAND_SET_ACTIVE[] =    { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
static const char PMS7003_COMMAND_REQUEST_READ[] =  { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };

enum { indoor, outdoor};

enum PMS_STATUS { PMS_STATUS_OK, PMS_STATUS_READING};

enum PMS_MODE { PMS_MODE_PASSIVE, PMS_MODE_ACTIVE};

//static enum PMS_STATUS pms_status = PMS_STATUS_READING;
static enum PMS_MODE pms_mode = PMS_MODE_ACTIVE;
static enum PMS_STATUS pms_status = PMS_STATUS_OK;

/**
 * @brief 
 * 
 * @param uart_config 
 * @return esp_err_t 
 */
esp_err_t pms7003_initUart(uart_config_t *uart_config);

/**
 * @brief
 * 
 * @return esp_err_t
*/
esp_err_t pms7003_checksum(uint8_t *data, uint8_t dataLength);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t pms7003_activeMode(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t pms7003_passiveMode(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t pms7003_sleepMode(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t pms7003_wakeup(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t pms7003_requestReadData(void);

/**
 * @brief 
 * 
 * @param pms_modeAmbience 
 * @param pm1_0 
 * @param pm2_5 
 * @param pm10 
 * @return esp_err_t 
 */
esp_err_t pms7003_readData(const int pms_modeAmbience, uint16_t *pm1_0, uint16_t *pm2_5, uint16_t *pm10);


#endif