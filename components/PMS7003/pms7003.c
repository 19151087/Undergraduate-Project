#include "pms7003.h"
#include "esp_timer.h"

__attribute__((unused)) static const char *TAG = "PMS7003";

static enum PMS_MODE pms_mode = PMS_MODE_ACTIVE;

esp_err_t pms7003_initUart(uart_config_t *uart_config)
{
    esp_err_t error_1 = uart_driver_install(CONFIG_PMS_UART_PORT, // UART_NUM_2 
                                            PMS7003_DRIVER_BUF_SIZE, // rx_buffer_size
                                            PMS7003_DRIVER_BUF_SIZE, // tx_buffer_size
                                            0, // queue_size
                                            NULL, // uart_queue
                                            0); // intr_alloc_flags
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_1);
    esp_err_t error_2 = uart_param_config(CONFIG_PMS_UART_PORT, uart_config); //uart_param_config() configures the UART driver and the UART hardware.
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_2);    
    esp_err_t error_3 = uart_set_pin(CONFIG_PMS_UART_PORT, CONFIG_PMS_PIN_TX, CONFIG_PMS_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error_3);

    if (error_1 == ESP_OK &&
        error_2 == ESP_OK &&
        error_3 == ESP_OK)
    {
        ESP_LOGI(__func__, "PMS7003 UART port initialize successful.");
        return ESP_OK;
    } else {
        ESP_LOGE(__func__, "PMS7003 UART port initialize failed.");
        return ESP_ERROR_PMS7003_INIT_UART_FAILED;
    }
}

// Set PMS7003 sensor to active mode.
esp_err_t pms7003_activeMode(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, PMS7003_COMMAND_SET_ACTIVE, PMS7003_COMMAND_SIZE); //uart_write_bytes() returns the number of bytes written.
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Set active mode failed.");
        return ESP_ERROR_PMS7003_SET_ACTIVE_MODE_FAILED;
    } else {
        pms_mode = PMS_MODE_ACTIVE;
        ESP_LOGI(__func__, "Set active mode successfuly.");
        return ESP_OK;
    }
}

// Set PMS7003 sensor to passive mode.
esp_err_t pms7003_passiveMode(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, PMS7003_COMMAND_SET_PASSIVE, PMS7003_COMMAND_SIZE); //uart_write_bytes() returns the number of bytes written.
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Set passive mode failed.");
        return ESP_ERROR_PMS7003_SET_PASSIVE_MODE_FAILED;
    } else {
        ESP_LOGI(__func__, "Set passive mode successfuly.");
        return ESP_OK;
    }
}

// Send request to read data from PMS7003 sensor.
esp_err_t pms7003_requestReadData(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, PMS7003_COMMAND_REQUEST_READ, PMS7003_COMMAND_SIZE); 
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Request read data failed.");
        return ESP_ERROR_PMS7003_REQUEST_READ_DATA_FAILED;
    }
    else {
        ESP_LOGI(__func__, "Request read data successfuly.");
        return ESP_OK;
    }
}

// Set PMS7003 sensor to sleep mode.
esp_err_t pms7003_sleepMode(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, PMS7003_COMMAND_SLEEP, PMS7003_COMMAND_SIZE); //uart_write_bytes() returns the number of bytes written.
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Set sleep mode failed.");
        return ESP_ERROR_PMS7003_SET_SLEEP_MODE_FAILED;
    } else {
        ESP_LOGI(__func__, "Set sleep mode successfuly.");
        return ESP_OK;
    }
}

// Set PMS7003 sensor to wake up mode.
esp_err_t pms7003_wakeUpMode(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, PMS7003_COMMAND_WAKEUP, PMS7003_COMMAND_SIZE); //uart_write_bytes() returns the number of bytes written.
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Set wake up mode failed.");
        return ESP_ERROR_PMS7003_WAKEUP_FAILED;
    } else {
        ESP_LOGI(__func__, "Set wake up mode successfuly.");
        return ESP_OK;
    }
}

//Read data from PMS7003 sensor.
bool pms7003_readData(uint16_t* pm1_0, uint16_t* pm2_5, uint16_t* pm10) {
    uint8_t data[32];
    uint8_t checksum = 0, sum = 0;
    bool valid;

    // read data
    uint8_t datalength = uart_read_bytes(CONFIG_PMS_UART_PORT, data, 32, 100 / portTICK_PERIOD_MS);

    if(datalength != 32) {
        ESP_LOGE(__func__, "Data length invalid.");
    }
    else
    {
        // checksum
        for (int i = 0; i < 30; i++) {
            sum += data[i];
        }

        checksum = ((data[30] << 8) | data[31]);

        if (data[0] == 0x42 && data[1] == 0x4d && sum == checksum) {
            *pm1_0 = (data[10] << 8) | data[11];
            *pm2_5 = (data[12] << 8) | data[13];
            *pm10 = (data[14] << 8) | data[15];
        
            valid = true;
            ESP_LOGI(__func__, "Read data successfully.\n");
        } 
        else {
            *pm1_0 = -1;
            *pm2_5 = -1;
            *pm10 = -1;

            ESP_LOGE(__func__, "Data invalid.\n");
        }
    }
    return valid;
}

// Checksum for data received from PMS7003 sensor.
//esp_err_t pms7003_checksum(uint8_t *data, uint8_t dataLength)
// {
//     uint8_t checksum = 0;
//     for (int i = 0; i < dataLength - 2; i++) // dataLength - 2 because the last 2 bytes are checksum.
//     {
//         checksum += data[i];
//     }
//     if(checksum == (data[dataLength - 2] << 8 | data[dataLength - 1]))
//     {
//         return ESP_OK;
//     }
//     else{
//         return ESP_ERROR_PMS7003_REQUEST_READ_DATA_FAILED;
//     }
// }
