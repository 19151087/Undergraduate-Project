#include "pms7003.h"

__attribute__((unused)) static const char *TAG = "PMS7003";

pms_err_t pms_initUart(uart_config_t *uart_config)
{
    esp_err_t error_1 = uart_driver_install(CONFIG_PMS_UART_PORT, (RX_BUFFER_SIZE * 2), 0, 0, NULL, 0);
    esp_err_t error_2 = uart_param_config(CONFIG_PMS_UART_PORT, uart_config);
    esp_err_t error_3 = uart_set_pin(CONFIG_PMS_UART_PORT, CONFIG_PMS_PIN_TX, CONFIG_PMS_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_1);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_2);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_3);

    if ((error_1 == ESP_OK) && (error_2 == ESP_OK) && (error_3 == ESP_OK))
    {
        ESP_LOGI(__func__, "PMS7003 UART port initialize successful.");
        return PMS_OK;
    }
    else
    {
        ESP_LOGE(__func__, "PMS7003 UART port initialize failed.");
        return PMS_ERR_INIT_UART_FAILED;
    }
}

pms_err_t pms_readData(uint16_t *pm1_0, uint16_t *pm2_5, uint16_t *pm10)
{
    uint8_t circularBuffer[PMS_BUFFER_SIZE];
    uint8_t dataFrame[PMS_DATA_FRAME_SIZE];
    int circularBufferIndex = 0;
    bool foundStartBytes = false;

    while (!foundStartBytes)
    {
        // Read data from UART
        uint8_t byte;
        int bytesRead = uart_read_bytes(CONFIG_PMS_UART_PORT, &byte, 1, pdMS_TO_TICKS(100));

        if (bytesRead == 1)
        {
            // Add byte to circular buffer
            circularBuffer[circularBufferIndex] = byte;
            circularBufferIndex = (circularBufferIndex + 1) % PMS_BUFFER_SIZE;

            // Check if circular buffer contains a complete data frame
            if (circularBufferIndex >= PMS_DATA_FRAME_SIZE)
            {
                // Copy data frame from circular buffer
                for (int i = 0; i < PMS_DATA_FRAME_SIZE; i++)
                {
                    dataFrame[i] = circularBuffer[(circularBufferIndex - PMS_DATA_FRAME_SIZE + i) % PMS_BUFFER_SIZE];
                }
                // Check start bytes and extract particle concentration data
                if (dataFrame[0] == START_CHAR1 && dataFrame[1] == START_CHAR2)
                {
                    uint16_t checksum = (dataFrame[30] << 8) | dataFrame[31];
                    uint16_t calculatedChecksum = 0;
                    for (int i = 0; i < 30; i++)
                    {
                        calculatedChecksum += dataFrame[i];
                    }

                    if (checksum == calculatedChecksum)
                    {
                        *pm1_0 = (dataFrame[4] << 8) | dataFrame[5];
                        *pm2_5 = (dataFrame[6] << 8) | dataFrame[7];
                        *pm10 = (dataFrame[8] << 8) | dataFrame[9];
                        foundStartBytes = true;

                        ESP_LOGI(__func__, "PMS7003 sensor read data successful.");
                        ESP_LOGI(__func__, "PM1.0: %dug/m3\tPM2.5: %dug/m3\tPM10: %dug/m3.\r", *pm1_0, *pm2_5, *pm10);
                    }
                    else
                    {
                        return PMS_ERR_INVALID_CHECKSUM;
                    }
                }
            }
        }
        else
        {
            // Handle error reading from UART
            ESP_LOGE(__func__, "Error reading from PMS7003 sensor.");
            return PMS_ERR_READ_DATA_FAILED;
        }
    }
    return PMS_OK;
}
