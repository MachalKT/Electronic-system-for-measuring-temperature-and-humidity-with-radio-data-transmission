//RTC DS1307

#ifndef RTC_H_
#define RTC_H_

#include <avr/io.h>

#define RTC_SECOND_REG_ADDRESS_U8   0x00u   // Address to access SEC register
#define RTC_DATE_REG_ADDRESS_U8     0x04u   // Address to access DATE register
#define RTC_CONTROL_REG_ADDRESS_U8  0x07u   // Address to access CONTROL register

#define RTC_WRITE_ADDRESS	0xD0	/* Define RTC DS1307 slave write address */
#define RTC_READ_ADDRESS	0xD1	/* Make LSB bit high of slave address for read */

typedef struct
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t weekDay;
	uint8_t day;
	uint8_t month;
	uint8_t year;
}rtct;

void RTCinit();
void RTC_SetDateTime(rtct *rtc);
void RTC_GetDateTime(rtct *rtc);

#endif /* RTC_H_ */