#ifndef __PMS7003_H__
#define __PMS7003_H__

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define START_CHAR1 0x42
#define START_CHAR2 0x4d
#define RX_BUFFER_SIZE 128
#define PMS_DATA_FRAME_SIZE 32
#define PMS_BUFFER_SIZE 40

#define PMS_UART_CONFIG_DEFAULT()              \
    {                                          \
        .baud_rate = CONFIG_UART_BAUD_RATE,    \
        .data_bits = UART_DATA_8_BITS,         \
        .parity = UART_PARITY_DISABLE,         \
        .stop_bits = UART_STOP_BITS_1,         \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
        .source_clk = UART_SCLK_APB,           \
    }

    typedef enum
    {
        PMS_OK = 0,
        PMS_ERR_INIT_UART_FAILED,
        PMS_ERR_READ_DATA_FAILED,
        PMS_ERR_INVALID_CHECKSUM,
        PMS_ERR_INVALID_STARTCHAR,
        PMS_ERR_INVALID_FRAMELENGTH,
        PMS_ERR_INVALID_VALUE = 0xFF,
    } pms_err_t;

    static const char PMS7003_COMMAND_WAKEUP[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};
    static const char PMS7003_COMMAND_SLEEP[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
    static const char PMS7003_COMMAND_SET_PASSIVE[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
    static const char PMS7003_COMMAND_SET_ACTIVE[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71};
    static const char PMS7003_COMMAND_REQUEST_READ[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};

    /**
     * @brief
     *
     * @param uart_config
     * @return esp_err_t
     */
    pms_err_t pms_initUart(uart_config_t *uart_config);

    /**
     * @brief
     *
     * @param pm1_0
     * @param pm2_5
     * @param pm10
     * @return esp_err_t
     */
    pms_err_t pms_readData(uint16_t *pm1_0, uint16_t *pm2_5, uint16_t *pm10);

#ifdef __cplusplus
}
#endif

#endif // __PMS7003_H__