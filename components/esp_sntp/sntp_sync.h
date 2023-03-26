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
 * @return timestamp
 */
uint32_t getTime(void);

/**
 * @brief Get current date time
 * 
 * @param date_time
 * @return void
 */
void getDate(char *date_time);

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


#ifdef __cplusplus
}
#endif 

#endif

/*
%a	Abbreviated weekday name	                                            Sun
%A	Full weekday name	                                                    Sunday
%b	Abbreviated month name	                                                Mar
%B	Full month name	                                                        March
%c	Date and time representation	                        Sun Aug 19 02:56:02 2012
%d	Day of the month (01-31)	                                            19
%H	Hour in 24h format (00-23)	                                            14
%I	Hour in 12h format (01-12)	                                            05
%j	Day of the year (001-366)	                                            231
%m	Month as a decimal number (01-12)	                                    08
%M	Minute (00-59)	                                                        55
%p	AM or PM designation	                                                PM
%S	Second (00-61)	                                                        02
%U	Week number with the first Sunday as the first day of week one (00-53)	33
%w	Weekday as a decimal number with Sunday as 0 (0-6)	                    4
%W	Week number with the first Monday as the first day of week one (00-53)	34
%x	Date representation	                                                    08/19/12
%X	Time representation	                                                    02:50:06
%y	Year, last two digits (00-99)	                                        01
%Y	Year	                                                                2012
%Z	Timezone name or abbreviation	                                        CDT
%%	A % sign	                                                            %
*/