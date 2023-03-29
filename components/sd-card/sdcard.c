#include "sdcard.h"

__attribute__((unused)) static const char *TAG = "SDcard";

// path file: /DataSensor/Indoor/time , time take from DS3231/ntp

esp_err_t sdcard_initialize(esp_vfs_fat_sdmmc_mount_config_t *_mount_config, sdmmc_card_t *_sdcard,
                            sdmmc_host_t *_host, spi_bus_config_t *_bus_config, sdspi_device_config_t *_slot_config)
{
    esp_err_t ret;
    ESP_LOGI(__func__, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(__func__, "Using SPI peripheral");

    ret = spi_bus_initialize(_host->slot, _bus_config, SPI_DMA_CH2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(__func__, "Failed to initialize bus.");
        ESP_LOGE(__func__, "Failed to initialize the SDcard.");
        return ESP_FAIL;
    }
    _slot_config->gpio_cs = CONFIG_SD_PIN_CS;
    _slot_config->host_id = _host->slot;

    ESP_LOGI(__func__, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, _host, _slot_config, _mount_config, &_sdcard);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(__func__, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(__func__, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }
    ESP_LOGI(__func__, "SDCard has been initialized.");
    ESP_LOGI(__func__, "Filesystem mounted");

    // Card has been initialized, print its properties
    ESP_LOGI(__func__, "SDCard properties.");
    sdmmc_card_print_info(stdout, _sdcard);
    return ESP_OK;
}


esp_err_t writetoSDcard(const char* namefile, const char* format, ...) 
{ 
    FILE * file = fopen(namefile, "a+"); // open or create file for appending
    ESP_LOGI(__func__, "Opening file %s ...", namefile);
    if (file == NULL)
    {
        ESP_LOGE(__func__, "Error opening file %s.", namefile);
        return ESP_FAIL;
    }

    char *data_str; // declare a pointer to a char
    va_list args, args_copy; // declare a variable argument list
    va_start(args, format); // initialize the list with the format parameter
    va_copy(args_copy, args); // copy the list to another variable
    int length = vsnprintf(NULL, 0, format, args_copy); // get the length of the string
    va_end(args_copy); // end the copy list
    if(length < 0) {
        ESP_LOGE(__func__, "Failed to create string data for writing.");
        va_end(args);
        return ESP_FAIL;
    }
    data_str = (char*)malloc(length + 1); // allocate memory for the string
    if(data_str == NULL) {
        ESP_LOGE(__func__, "Failed to create string data for writing.");
        va_end(args);
        return ESP_FAIL;
    }

    vsnprintf(data_str, length + 1, format, args); // create the string
    ESP_LOGI(__func__, "Success to create string data(%d) for writing.", length);
    ESP_LOGI(__func__, "Writing data to file %s...", namefile);
    ESP_LOGI(__func__, "%s;\n", data_str);

    int write_result = vfprintf(file, format, args); // write to file
    if (write_result < 0)
    {
        ESP_LOGE(__func__, "Failed to write data to file %s.", namefile);
        return ESP_FAIL;
    }
    ESP_LOGI(__func__, "Success to write data (%d bytes) to file %s.", write_result, namefile);
    va_end(args); // end the list
    fclose(file); // close file
    free(data_str); // free the memory
    return ESP_OK;
}

esp_err_t readfromSDcard(const char* namefile, char** data_str) 
{ 
    FILE * file = fopen(namefile, "r"); // open file for reading
    ESP_LOGI(__func__, "Opening file %s ...", namefile);
    
    if (file == NULL)
    {
        ESP_LOGE(__func__, "Error opening file %s.", namefile);
        return ESP_FAIL; // return an error code
    }

    fseek(file, 0, SEEK_END); // move the file pointer to the end of the file
    long length = ftell(file); // get the current position of the file pointer (the length of the file)
    fseek(file, 0, SEEK_SET); // move the file pointer back to the beginning of the file
    
    if(length < 0) {
        ESP_LOGE(__func__, "Error getting the length of the file %s.", namefile);
        return ESP_FAIL; // return an error code
    }
    
    *data_str = (char*)malloc(length + 1); // allocate memory for the string
    
    if(*data_str == NULL) {
        ESP_LOGE(__func__, "Error allocating memory for string data for reading.");
        return ESP_FAIL; // return an error code
    }

    size_t read_result = fread(*data_str, 1, length, file); // read the file content into the string
    (*data_str)[length] = '\0'; // add a null terminator to the end of the string
    
    if (read_result != length)
    {
        ESP_LOGE(__func__, "Error reading data from file %s.", namefile);
        return ESP_FAIL; // return an error code
    }
    
    ESP_LOGI(__func__, "Successfully read data (%d bytes) from file %s.", read_result, namefile);
    ESP_LOGI(__func__, "Reading data from file %s...", namefile);
    ESP_LOGI(__func__, "%s;\n", *data_str);
    
    fclose(file); // close file
    return ESP_OK; // return a success code
}
