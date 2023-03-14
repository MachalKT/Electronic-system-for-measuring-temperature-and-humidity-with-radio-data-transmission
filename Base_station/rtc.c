/*
 * rtc.c
 *
 * Created: 27.05.2022 15:23:02
 *  Author: Maciek
 */ 

#include "rtc.h"
#include "i2c.h"


void RTCinit() 
{
	i2cInit();
	i2cStart();
	i2cSendData(RTC_WRITE_ADDRESS);
	i2cSendData(RTC_CONTROL_REG_ADDRESS_U8 );
	i2cSendData(0x00);
	i2cStop();
}

void RTC_SetDateTime(rtct *rtc) 
{
	i2cStart();
	i2cSendData(RTC_WRITE_ADDRESS);
	i2cSendData(RTC_SECOND_REG_ADDRESS_U8 );
	i2cSendData(rtc->sec);
	i2cSendData(rtc->min);
	i2cSendData(rtc->hour);
	i2cSendData(rtc->weekDay);
	i2cSendData(rtc->day);
	i2cSendData(rtc->month);
	i2cSendData(rtc->year);
	
	
	i2cStop();
}

void RTC_GetDateTime(rtct *rtc) 
{
	i2cStart();
	i2cSendData(RTC_WRITE_ADDRESS);
	i2cSendData(RTC_SECOND_REG_ADDRESS_U8 );
	i2cStop();
	i2cStart();
	i2cSendData(RTC_READ_ADDRESS);
	
	rtc->sec = i2cReadDataAck();
	rtc->min = i2cReadDataAck();
	rtc->hour = i2cReadDataAck();
	rtc->weekDay = i2cReadDataAck();
	rtc->day = i2cReadDataAck();
	rtc->month = i2cReadDataAck();
	rtc->year = i2cReadDataNotAck();
	
	i2cStop();
}
