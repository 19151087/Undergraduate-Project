#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <string.h>
#include <stdarg.h>
#include <esp_log.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDSPI_VSPI_HOST VSPI_HOST

#define SPI_BUS_CONFIG_DEFAULT()  { .mosi_io_num = CONFIG_SD_PIN_MOSI,    \
                                    .miso_io_num = CONFIG_SD_PIN_MISO,    \
                                    .sclk_io_num = CONFIG_SD_PIN_CLK,     \
                                    .quadwp_io_num = -1,            \
                                    .quadhd_io_num = -1,            \
                                    .max_transfer_sz = 4000,        \
}

#define MOUNT_CONFIG_DEFAULT()    { .format_if_mount_failed = true,         \
                                    .max_files = 5,                         \
                                    .allocation_unit_size = (16 * 1024),  \
}

#define SDSPI_HOST_VSPI() {\
    .flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG, \
    .slot = SDSPI_VSPI_HOST, \
    .max_freq_khz = SDMMC_FREQ_DEFAULT, \
    .io_voltage = 3.3f, \
    .init = &sdspi_host_init, \
    .set_bus_width = NULL, \
    .get_bus_width = NULL, \
    .set_bus_ddr_mode = NULL, \
    .set_card_clk = &sdspi_host_set_card_clk, \
    .do_transaction = &sdspi_host_do_transaction, \
    .deinit_p = &sdspi_host_remove_device, \
    .io_int_enable = &sdspi_host_io_int_enable, \
    .io_int_wait = &sdspi_host_io_int_wait, \
    .command_timeout_ms = 0, \
}

#define MOUNT_POINT "/sdcard"
static const char mount_point[] = MOUNT_POINT;


/**
 * @brief Initializes SD card with configuration.
 * 
 * @param[in] _mount_config Pointer to structure with extra parameters for mounting FATFS.
 * @param[in] _sdcard       Pointer SD/MMC card information structure.
 * @param[in] _host         Pointer to structure describing SDMMC host. This structure can be
 *                          initialized using SDSPI_HOST_DEFAULT() macro.
 * @param[in] _bus_config   Pointer to configuration structure for a SPI bus.
 * @param[in] _slot_config  Pointer to structure with slot configuration.
 *                          For SPI peripheral, pass a pointer to sdspi_device_config_t
 *                          structure initialized using SDSPI_DEVICE_CONFIG_DEFAULT().
 * 
 * @return esp_err_t 
 * 
 * @retval ESP_OK on success.
 * @retval ESP_FAIL on fail.
 */
esp_err_t sdcard_initialize(esp_vfs_fat_sdmmc_mount_config_t *_mount_config, sdmmc_card_t *_sdcard,
                            sdmmc_host_t *_host, spi_bus_config_t *_bus_config, sdspi_device_config_t *_slot_config);

/**
 * @brief Write data to file follow format.
 * 
 * @param[in] nameFile Name file.
 * @param[in] format Template structure for data store.
 * @param ... #__VA_ARGS__ : List arguments
 * 
 * @return esp_err_t 
 * 
 * @retval  - ESP_OK on success.
 * @retval  - ESP_FAIL on fail.
 */
esp_err_t writetoSDcard(const char* namefile, const char* format, ...);


/**
 * @brief Read data from file.
 * 
 * @param[in] nameFile Name file.
 * @param[out] data_str Pointer to data read from file.
 * 
 * @return esp_err_t
 * 
 * @retval  - ESP_OK on success.
 * @retval  - ESP_FAIL on fail.
*/
esp_err_t readfromSDcard(const char* namefile, char** data_str);


#ifdef __cplusplus
}
#endif 

#endif // __PMS7003_H__