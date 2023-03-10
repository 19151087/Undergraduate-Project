#include "pms7003.h"

__attribute__((unused)) static const char *TAG = "PMS7003";

esp_err_t pms7003_initUart(uart_config_t *uart_config)
{
    /** 
     * @note: 3rd parameter of uart_driver_install() function - rx_buffer_size never less than 128 byte.
     *        if rx_buffer_size <= 128, program print error code "uart rx buffer length error".
     */

    esp_err_t error_1 = uart_driver_install(CONFIG_PMS_UART_PORT, (RX_BUFFER_SIZE * 2), 0, 0, NULL, 0); //uart_driver_install() allocates memory for the driver, and sets up the interrupt handler.
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
        pms_mode = PMS_MODE_PASSIVE;
        ESP_LOGI(__func__, "Set passive mode successfuly.");
        return ESP_OK;
    }
}

// Send request to read data from PMS7003 sensor.
esp_err_t pms7003_requestReadData(void)
{
    if(pms_mode == PMS_MODE_PASSIVE)
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
    else{
        return ESP_ERROR_PMS7003_REQUEST_READ_DATA_FAILED;
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


// Checksum for data received from PMS7003 sensor.
esp_err_t pms7003_checksum(uint8_t *data, uint8_t dataLength)
{
    uint8_t checksum = 0;
    for (int i = 0; i < dataLength - 2; i++) // dataLength - 2 because the last 2 bytes are checksum.
    {
        checksum += data[i];
    }
    if(checksum == (data[dataLength - 2] << 8 | data[dataLength - 1]))
    {
        return ESP_OK;
    }
    else{
        return ESP_ERROR_PMS7003_REQUEST_READ_DATA_FAILED;
    }
}

//Read data from PMS7003 sensor.
esp_err_t pms7003_readData(const int pms_modeAmbience, uint16_t *pm1_0, uint16_t *pm2_5, uint16_t *pm10)
{
    pms_status = PMS_STATUS_READING;
    uint8_t data[RX_BUFFER_SIZE];

    SemaphoreHandle_t print_mutex = NULL;
    print_mutex = xSemaphoreCreateMutex();

    int dataLength = uart_read_bytes(CONFIG_PMS_UART_PORT, data, RX_BUFFER_SIZE, 1000 / portTICK_PERIOD_MS); //uart_read_bytes() returns the number of bytes read.
    
    xSemaphoreTake(print_mutex, portMAX_DELAY);

    if(dataLength == -1)
    {
        ESP_LOGE(__func__, "Read data failed.");
        return ESP_ERROR_PMS7003_READ_DATA_FAILED;
    }
    else
    {
        uint8_t i = 0;
        // Check if the first 2 bytes are the start character and the checksum is correct.
        if(data[i] == START_CHARACTER_1 && data[i+1] == START_CHARACTER_2 && pms7003_checksum(data, dataLength) == ESP_OK)
        {
            uint8_t dataByte = i;
            dataByte += (pms_modeAmbience == indoor) ? 4 : 10;

            *pm1_0 = data[dataByte] << 8 | data[dataByte + 1];
            *pm2_5 = data[dataByte + 2] << 8 | data[dataByte + 3];
            *pm10 = data[dataByte + 4] << 8 | data[dataByte + 5];

            ESP_LOGI(__func__, "PMS7003 sensor read data successful.");
            ESP_LOGI(__func__, "PM1.0: %dug/m3\tPM2.5: %dug/m3\tPM10: %dug/m3.\r", *pm1_0, *pm2_5, *pm10);

            xSemaphoreGive(print_mutex);
            vSemaphoreDelete(print_mutex);
            pms_status = PMS_STATUS_OK;
            return ESP_OK;
        }
        else{
            *pm1_0 = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
            *pm2_5 = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
            *pm10  = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
            
            ESP_LOGE(__func__, "Invalid value.");
            xSemaphoreGive(print_mutex);
            vSemaphoreDelete(print_mutex);
            return ESP_ERROR_PMS7003_READ_DATA_FAILED;
        }
    }
}