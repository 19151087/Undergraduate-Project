#include "sdcard.h"

__attribute__((unused)) static const char *TAG = "SDcard";

// path file: /DataSensor/Indoor/time , time take from DS3231/ntp

esp_err_t writetoSDcard(const char *namefile, const char *format, ...)
{
    FILE *file = fopen(namefile, "a+"); // open or create file for appending
    ESP_LOGI(__func__, "Opening file %s ...", namefile);
    if (file == NULL)
    {
        ESP_LOGE(__func__, "Error opening file %s.", namefile);
        return ESP_FAIL;
    }

    char *data_str;                                     // declare a pointer to a char
    va_list args, args_copy;                            // declare a variable argument list
    va_start(args, format);                             // initialize the list with the format parameter
    va_copy(args_copy, args);                           // copy the list to another variable
    int length = vsnprintf(NULL, 0, format, args_copy); // get the length of the string
    va_end(args_copy);                                  // end the copy list
    if (length < 0)
    {
        ESP_LOGE(__func__, "Failed to create string data for writing.");
        va_end(args);
        return ESP_FAIL;
    }
    data_str = (char *)malloc(length + 1); // allocate memory for the string
    if (data_str == NULL)
    {
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
    va_end(args);   // end the list
    fclose(file);   // close file
    free(data_str); // free the memory
    return ESP_OK;
}

esp_err_t readfromSDcard(const char *namefile, char **data_str)
{
    FILE *file = fopen(namefile, "r"); // open file for reading
    ESP_LOGI(__func__, "Opening file %s ...", namefile);

    if (file == NULL)
    {
        ESP_LOGE(__func__, "Error opening file %s.", namefile);
        return ESP_FAIL; // return an error code
    }

    fseek(file, 0, SEEK_END);  // move the file pointer to the end of the file
    long length = ftell(file); // get the current position of the file pointer (the length of the file)
    fseek(file, 0, SEEK_SET);  // move the file pointer back to the beginning of the file

    if (length < 0)
    {
        ESP_LOGE(__func__, "Error getting the length of the file %s.", namefile);
        return ESP_FAIL; // return an error code
    }

    *data_str = (char *)malloc(length + 1); // allocate memory for the string

    if (*data_str == NULL)
    {
        ESP_LOGE(__func__, "Error allocating memory for string data for reading.");
        return ESP_FAIL; // return an error code
    }

    size_t read_result = fread(*data_str, 1, length, file); // read the file content into the string
    (*data_str)[length] = '\0';                             // add a null terminator to the end of the string

    if (read_result != length)
    {
        ESP_LOGE(__func__, "Error reading data from file %s.", namefile);
        return ESP_FAIL; // return an error code
    }

    ESP_LOGI(__func__, "Successfully read data (%d bytes) from file %s.", read_result, namefile);
    ESP_LOGI(__func__, "Reading data from file %s...", namefile);
    ESP_LOGI(__func__, "%s;\n", *data_str);

    fclose(file);  // close file
    return ESP_OK; // return a success code
}

esp_err_t deleteSDcardData(const char *filename)
{
    // Check if file exists
    if (access(filename, F_OK) == -1)
    {
        printf("File does not exist\n");
        return ESP_FAIL;
    }
    // Delete file
    if (unlink(filename) == -1)
    {
        printf("Error deleting file\n");
        return ESP_FAIL;
    }
    ESP_LOGI(__func__, "Data has been deleted.");
    // Create new file with appropriate permissions
    FILE *file = fopen(filename, "w+");
    if (file == NULL)
    {
        printf("Error creating file\n");
        return ESP_FAIL;
    }
    fprintf(file, "Timestamp, Temperature, Humidity, PM2.5, PM1, PM10 \n");
    // Close file and check for errors
    fclose(file);
    return ESP_OK;
}