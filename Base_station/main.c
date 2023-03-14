/*
 * _PracaInzynierska_Odbiornik_1.20.c
 *
 * Created: 06.02.2023 14:29:45
 * Author : Maciek
 */ 

/*
 * _PracaInzynierska_Odbiornik_1.19.c
 *
 * Created: 01.02.2023 12:18:02
 * Author : Maciek
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "lcd.h"
#include "myEeprom.h"
#include "rtc.h"
#include "i2c.h"

#define INCORRECT_DATA false
#define CORRECT_DATA true

#define DIODE_1 0
#define DIODE_2 1

#define BUTTON_S4 0
#define BUTTON_S1 1
#define BUTTON_S2 2
#define BUTTON_S3 3

#define EEPROM_TAB_SIZE 12

#define FIRST_EEPROM_INDEX 0
#define SHIFT_EEPROM_INDEX 12
#define LAST_EEPROM_INDEX  1008
#define WRONG_EEPROM_INDEX 1020

#define SENSOR_BME280 1

#define TEMPERATURE 0
#define HUMIDITY 1
#define PRESSURE 2
#define TIME 3
#define DATE 4

#define SENSOR_NUMBER_INDEX 0
#define YEAR_INDEX1 1
#define YEAR_INDEX2 2
#define MONTH_INDEX 3
#define DAY_INDEX 4
#define HOUR_INDEX 5
#define MINUTE_INDEX 6
#define SECOND_INDEX 7
#define TEMPERATURE_INDEX 8
#define HUMIDITY_INDEX 9
#define PRESSURE_INDEX1 10
#define PRESSURE_INDEX2 11

#define BEFORE_FIRST_MEASUREMENT -1
#define WAS_FIRST_MEASUREMENT 0
#define AFTER_FIRST_MEASUREMENT 1


//Functions
void readAndDisplay(int parameter, int shift);
char *readValueToString(char *output, int index, int shift);
bool decodeInfo(uint8_t* input, int* output);
int *buildEepromTab(int* output, int sensorNumber, rtct* rtc, int* parameter);
int *numberInTwo(int * output, int number);
int convertHexToInt(uint8_t hex);
void writeToEepromInt(int* tab,int size, int shift);

nRF24L01 *setup_nrf(void);
void set_time(rtct* rtc);
void clean_timer1(void);
void init_timer1(void);
void init_timer0(void);
void init_button_interrup(void);

//Functions for any eventuality
void diodeOn(int diode);
void diodeOff(int diode);

//Flags
volatile bool interruptFlag = false;
volatile bool sendMessageFlag = false;
volatile bool transmitterFlag = false;
volatile bool correctFlag = false;
volatile bool buttonS2Press = false;
volatile bool buttonS4Press = false;
volatile bool buttonS1Press = false;
volatile bool buttonS3Press = false;
volatile int firstTime = BEFORE_FIRST_MEASUREMENT;

//variables
volatile int counter0 = 0;
volatile int counter1 = 0;
volatile int measurementNumber = 0;
volatile int whichParameter = 0;

int main(void) 
{	
	//nRF
	uint8_t address[5] = { 0x01, 0x01, 0x01, 0x01, 0x01 };
	uint8_t addr[5];
	nRF24L01 *nrf = setup_nrf();
	nRF24L01_read_register(nrf, CONFIG, addr, 1);
	
	//LCD
	LCD_Initalize();
	
	//RTC
	rtct rtc;
	RTCinit();
	//set_time(&rtc);
	
	
	//Diodes
	DDRD |= (1<<DIODE_1) | (1<<DIODE_2);
	diodeOn(DIODE_1);
	
	//Variables
	int shiftWrite = FIRST_EEPROM_INDEX;
	int shiftRead = FIRST_EEPROM_INDEX;
	int maxShiftRead = 0;
	
	//start
	init_button_interrup();
	//init_timer0();
	
	sei();
	
	LCD_Clear();
	LCD_GoTo(0,0);
	LCD_WriteText("ODBIORNIK");
	LCD_GoTo(0,1);
	LCD_WriteText("CZEKAJ");
	_delay_ms(2000);
	
	int amountMeas = 0;
	int i = 0;
	i = 0;
	while(1)
	{
		amountMeas = (int)(eeprom_read_byte((uint8_t *)(0x00 + FIRST_EEPROM_INDEX + i)));
		if(i >= WRONG_EEPROM_INDEX)
		{
			break;
		}
		if(amountMeas == 255)
		{
			if(i > 0)
			{
				firstTime = WAS_FIRST_MEASUREMENT;
				readAndDisplay(0,0);
			}
			break;
		}
		i = i + SHIFT_EEPROM_INDEX;
		measurementNumber = amountMeas;
	}
	
	shiftWrite = i;
	maxShiftRead = i - SHIFT_EEPROM_INDEX;
	if(maxShiftRead < 0)
	maxShiftRead = 0;
	
	RTC_GetDateTime(&rtc); 
	uint8_t timeTransmit = rtc.sec + 0x10;
	if(timeTransmit >= 0x60)
		timeTransmit = timeTransmit - 0x60;
	
	sendMessageFlag = true;
	transmitterFlag = true;
	
	
	while (true)
	{
		//------------------------------------------------------
		//FIRST RECEIVE-----------------------------------------
		//------------------------------------------------------
		if(firstTime == WAS_FIRST_MEASUREMENT)
		{
			firstTime = AFTER_FIRST_MEASUREMENT;
			readAndDisplay(whichParameter,shiftRead);
		}
		
		
		//------------------------------------------------------
		//RTC---------------------------------------------------
		//------------------------------------------------------
		RTC_GetDateTime(&rtc);
		if(rtc.sec == timeTransmit)
		{
			transmitterFlag = true;
			sendMessageFlag = true;
			timeTransmit = rtc.sec + 0x10;
			
			if(timeTransmit >= 0x60)
				timeTransmit = timeTransmit - 0x60;
		}
		
		//------------------------------------------------------
		//TRANSMITTER-------------------------------------------
		//------------------------------------------------------
		if(transmitterFlag) //
		{
			
			if (interruptFlag)
			{
				interruptFlag = false;
				transmitterFlag = false;
				
				int success = nRF24L01_transmit_success(nrf);
				if (success != 0)
				nRF24L01_flush_transmit_message(nrf);
				
				nRF24L01_listen(nrf, 0, address);
				init_timer1();
				diodeOff(DIODE_2);
			}
				
			if (sendMessageFlag)
			{
				nRF24L01_retry_transmit2(nrf);
				sendMessageFlag = false;
				
				nRF24L01Message msg;
				memcpy(msg.data,"SEND",5);
				msg.length = strlen((char *)msg.data) + 1;
				
				nRF24L01_transmit(nrf, address, &msg);
				diodeOn(DIODE_2);
			}
				
		}
		//------------------------------------------------------
		//RECEIVER----------------------------------------------
		//------------------------------------------------------
		else
		{
			
			if(interruptFlag)
			{
				interruptFlag = false;
				clean_timer1();
				
				while (nRF24L01_data_received(nrf))
				{
					//read nrf message
					nRF24L01Message msg;
					nRF24L01_read_received_data(nrf, &msg);
					
					//write to int tab and check value
					int parameters[3];
					correctFlag = decodeInfo(msg.data,parameters);
					if(!correctFlag)
					{
						init_timer1();
						break;
					}
					
					if(firstTime == BEFORE_FIRST_MEASUREMENT)
						firstTime = WAS_FIRST_MEASUREMENT;
					
					//write to eeprom
					measurementNumber++;
					if(measurementNumber > 84)
						measurementNumber = 84;
					int tabToEeprom[EEPROM_TAB_SIZE];
					buildEepromTab(tabToEeprom,measurementNumber,&rtc,parameters);
					
					writeToEepromInt(tabToEeprom,EEPROM_TAB_SIZE,shiftWrite);
					
					if(maxShiftRead < LAST_EEPROM_INDEX)
						maxShiftRead = shiftWrite;
						
					shiftWrite = shiftWrite + SHIFT_EEPROM_INDEX;
					
					if(shiftWrite >= WRONG_EEPROM_INDEX)
						shiftWrite = 0;
					
					
					
					//readAll(maxShiftRead, temp,hum,pres);					
					//itoa(measurementNumber,measNumberchar,10);
					//display(measNumberchar, temp, hum, pres);					
				}
				//nRF24L01_listen(nrf, 0, address);
				
				
			}
		}
		
		

		
		
		//------------------------------------------------------
		//BUTTONS-----------------------------------------------
		//------------------------------------------------------
		if(buttonS2Press)
		{
			buttonS2Press = false;
			if(firstTime == BEFORE_FIRST_MEASUREMENT)
				continue;
			whichParameter++;
			if(whichParameter>4)
				whichParameter = 0;
			readAndDisplay(whichParameter,shiftRead);
			
		}
		
		if(buttonS4Press)
		{
			buttonS4Press = false;
			if(firstTime == BEFORE_FIRST_MEASUREMENT)
				continue;
			shiftRead = shiftRead + SHIFT_EEPROM_INDEX;
			
			if(shiftRead > maxShiftRead)
				shiftRead = 0;
				
			whichParameter = 0;
			readAndDisplay(whichParameter,shiftRead);
			//whichParameter++;
		}
		
		
		if(buttonS1Press)
		{
			buttonS1Press = false;
			if(firstTime == BEFORE_FIRST_MEASUREMENT)
				continue;
			shiftRead = shiftRead - SHIFT_EEPROM_INDEX;
			
			if(shiftRead < 0)
				shiftRead = maxShiftRead;
			
			whichParameter = 0;
			readAndDisplay(whichParameter,shiftRead);
			//whichParameter++;
		}
		
		if(buttonS3Press)
		{
			buttonS3Press = false;
			if(firstTime == BEFORE_FIRST_MEASUREMENT)
				continue;
			if(firstTime != AFTER_FIRST_MEASUREMENT)
				continue;
			
			shiftRead = shiftWrite - SHIFT_EEPROM_INDEX;
			
			if(shiftRead < 0)
				shiftRead = maxShiftRead;
				
			whichParameter = 0;
			readAndDisplay(whichParameter,shiftRead);
			//whichParameter++;
		}
		
	}
	return 0;
}

void readAndDisplay(int parameter, int shift)
{
	char string[5];

	LCD_Clear();
	LCD_GoTo(0,0);
	LCD_WriteText("M ");
	LCD_WriteText(readValueToString(string,SENSOR_NUMBER_INDEX,shift));
	
	switch(parameter)
	{
		case TEMPERATURE:
		{
			LCD_GoTo(0,1);
			LCD_WriteText("Temp: ");
			LCD_WriteText(readValueToString(string,TEMPERATURE_INDEX,shift));
			LCD_WriteText(" C");
		}
		break;
		case HUMIDITY:
		{	
			LCD_GoTo(0,1);
			LCD_WriteText("Hum: ");
			LCD_WriteText(readValueToString(string,HUMIDITY_INDEX,shift));
			LCD_WriteText(" %");
		}
		
		break;
		case PRESSURE:
		{
			LCD_GoTo(0,1);
			LCD_WriteText("Pres: ");
			LCD_WriteText(readValueToString(string,PRESSURE_INDEX1,shift));
			LCD_WriteText(" hPa");
		}
		break;
		case TIME:
		{
			LCD_GoTo(0,1);
			LCD_WriteText("Time: ");
			LCD_WriteText(readValueToString(string,HOUR_INDEX,shift));
			LCD_WriteText(":");
			LCD_WriteText(readValueToString(string,MINUTE_INDEX,shift));
			LCD_WriteText(":");
			LCD_WriteText(readValueToString(string,SECOND_INDEX,shift));
		}
		break;
		case DATE:
		{
			LCD_GoTo(0,1);
			LCD_WriteText("Date: ");
			LCD_WriteText(readValueToString(string,DAY_INDEX,shift));
			LCD_WriteText(".");
			LCD_WriteText(readValueToString(string,MONTH_INDEX,shift));
			LCD_WriteText(".");
			LCD_WriteText(readValueToString(string,YEAR_INDEX1,shift));
		}
		break;
		default:
			readAndDisplay(0, shift);
		break;
	}
}

char *readValueToString(char *output, int index, int shift)
{
	if(index == TEMPERATURE_INDEX)	//temperature may be a minus number
	{
		int temperature = eeprom_read_byte((uint8_t *)(0x00 + index + shift));
		
		if(temperature >= 128)
			temperature = temperature - 256;
	
		return itoa(temperature,output,10);
	}
	
	
	if(index == PRESSURE_INDEX1 || index == YEAR_INDEX1)	//numbers have value in two index
	{
		int value = eeprom_read_byte((uint8_t *)(0x00 + index + shift)) * 100;
		value = value + eeprom_read_byte((uint8_t *)(0x00 + (index + 1) + shift));
		
		return itoa(value,output,10);
	}
	
	if(index >= MONTH_INDEX && index <= SECOND_INDEX)
	{
		itoa(eeprom_read_byte((uint8_t *)(0x00 + index + shift)),output,10);
		if(strlen(output) == 1)
		{
			output[1] = output[0];
			output[0] = '0';
			output[2] = '\0';
		}
		return output;
	}
	
	return itoa(eeprom_read_byte((uint8_t *)(0x00 + index + shift)),output,10);	//normal read
}

bool decodeInfo(uint8_t* input, int* output)
{
	//TEMPERATURE---------------------------
	//Assign
	output[0] = (int)input[0];
	//If < 0
	if(output[0] > 127)
		output[0] = output[0] - 256;
	//check value
	if(output[0] < -40 || output[0] > 85)
		return INCORRECT_DATA;
	//--------------------------------------
	
	//HUMIDITY------------------------------
	//Assign
	output[1] = (int)input[1];
	//check value
	if(output[1] < 10 || output[1] > 80)
		return INCORRECT_DATA;
	//--------------------------------------
	
	//PRESSURE------------------------------
	//assign
	int hundreds = (int)input[2];
	int unity = (int)input[3];
	//check value
	if(hundreds < 3 || hundreds > 11)
		return INCORRECT_DATA;
	if(unity < 0 || unity > 99)
		return INCORRECT_DATA;
	//assign
	output[2] = hundreds * 100 + unity;
	//check
	if(output[2] < 300 || output[2] > 1100)
		return INCORRECT_DATA;
	//--------------------------------------
	
	return CORRECT_DATA;
}

int *buildEepromTab(int* output, int sensorNumber, rtct* rtc, int* parameter)
{
	output[SENSOR_NUMBER_INDEX] = sensorNumber;
	
	int numbersInTwo[2];
	numberInTwo(numbersInTwo,convertHexToInt(rtc->year));
	
	output[YEAR_INDEX1] = numbersInTwo[0];
	output[YEAR_INDEX2] = numbersInTwo[1];
	output[MONTH_INDEX] = convertHexToInt(rtc->month);
	output[DAY_INDEX] = convertHexToInt(rtc->day);
	
	output[HOUR_INDEX] = convertHexToInt(rtc->hour);
	output[MINUTE_INDEX] = convertHexToInt(rtc->min);
	output[SECOND_INDEX] = convertHexToInt(rtc->sec);
	
	output[TEMPERATURE_INDEX] = parameter[0];
	output[HUMIDITY_INDEX] = parameter[1];
	
	numberInTwo(numbersInTwo,parameter[2]);
	output[PRESSURE_INDEX1] = numbersInTwo[0];
	output[PRESSURE_INDEX2] = numbersInTwo[1];
	
	return output;
}

int *numberInTwo(int * output, int number)
{
	int thousands, hundreds, tens, unity;
	
	thousands = number / 1000;
	number  = number - (thousands * 1000);
	
	hundreds = number /100;
	number  = number - (hundreds * 100);
	
	tens = number / 10;
	number  = number - (tens * 10);
	
	unity = number;
	
	output[0] = thousands*10 + hundreds;
	output[1] = tens*10 + unity;
	
	return output;
}

int convertHexToInt(uint8_t hex)
{
	char tabdate[5];
	
	itoa(hex,tabdate,16);
	return atoi(tabdate);
}

void diodeOn(int diode)
{
	PORTD |= (1<<diode);
}

void diodeOff(int diode)
{
	PORTD &= ~(1<<diode);
}

void writeToEepromInt(int* tab,int size, int shift)
{
	int i=0;
	for(i=0;i<size;i++)
	{
		eeprom_write_byte((uint8_t *)(0x00 + i+ shift), (uint8_t)tab[i]);
	}
}

nRF24L01 *setup_nrf(void)
{
	nRF24L01 *rf = nRF24L01_init();
	
	rf->ss.port = &PORTB;
	rf->ss.pin = PB2;
	rf->ce.port = &PORTB;
	rf->ce.pin = PB1;
	rf->sck.port = &PORTB;
	rf->sck.pin = PB5;
	rf->mosi.port = &PORTB;
	rf->mosi.pin = PB3;
	rf->miso.port = &PORTB;
	rf->miso.pin = PB4;
	
	// interrupt on falling edge of INT0 (PD2)
	EICRA |= _BV(ISC01);
	EIMSK |= _BV(INT0);
	nRF24L01_begin(rf);
	
	return rf;
}

void set_time(rtct *rtc)
{
	//time: 00:00:00
	rtc->hour = 0x23;
	rtc->min =  0x10;
	rtc->sec =  0x02;
	
	rtc->day = 0x07;
	rtc->month = 0x03;
	rtc->year = 0x23;
	
	rtc->weekDay = 2;
	
	RTC_SetDateTime(rtc); 
}

void clean_timer1(void)
{
	TCCR1B &= 0x0;
	counter1 = 0;
}

void init_timer1(void)
{
	//prescaler 256
	TCCR1B = (1<<CS12) ;//| (1<<CS10);
	//interrupt
	TIMSK1 = (1<<TOIE1);
}

void init_timer0(void)
{
	TCCR0B |= (1<<CS02) | (1<<CS00);
	TIMSK0 |= (1<<TOIE0);
}

void init_button_interrup(void)
{
	PCICR |= (1<<PCIE1);
	PCMSK1 |= (1<<PCINT8) | (1<<PCINT9) | (1<<PCINT10) | (1<<PCINT11);
}

// nRF24L01 interrupt
ISR(INT0_vect)
{
	interruptFlag = true;
}

ISR(TIMER1_OVF_vect)
{
	counter1++;
	if(counter1 == 4)
	{
		transmitterFlag = true;
		sendMessageFlag = true;
		clean_timer1();
	}
}

ISR(TIMER0_OVF_vect)
{
	counter0++;
	if(counter0 == 500)
	{
		PORTC ^= (1<<2);
		if(correctFlag)
		{
			transmitterFlag = true;
			sendMessageFlag = true;
		}
		counter0 = 0;
	}
}

ISR(PCINT1_vect)
{
	if(bit_is_clear(PINC,BUTTON_S4))
		buttonS4Press = true;
	
	if(bit_is_clear(PINC,BUTTON_S1))
		buttonS1Press = true;
	
	if(bit_is_clear(PINC,BUTTON_S2))
		buttonS2Press = true;
	
	if(bit_is_clear(PINC,BUTTON_S3))
		buttonS3Press = true;
}


