/*
 * Sensor
 */

#define F_CPU 16000000UL

#define PARAM_SIZE 5
#define D1 0
#define D2 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>
#include "nrf24l01.h"
#include "bme280.h"
#include "nrf24l01-mnemonics.h"

//Functions
int *numberInTwo(int * output, int number);
void myMemcpy(uint8_t* input, uint8_t* output);
void processMessage(char *message);
uint8_t* readingBME280ToTab(uint8_t* parameters, uint16_t result, Bme280CalibrationData calibrationData, Bme280Data dataParameters);

Bme280Data sensor_parameters(uint16_t result, Bme280CalibrationData calibrationData);
nRF24L01 *setup_nrf(void);

//Flags
volatile bool interruptFlag = false;
volatile bool sendMessageFlag = false;
volatile bool transmitterFlag = false;
volatile int flag = 1;

int main(void) 
{
	//nRF
	uint8_t address[5] = { 0x01, 0x01, 0x01, 0x01, 0x01 };
	uint8_t addr[5];
	nRF24L01 *nrf = setup_nrf();
	nRF24L01_listen(nrf, 0, address);
	nRF24L01_read_register(nrf, CONFIG, addr, 1);
	
	//BME280
	uint16_t result;
	Bme280CalibrationData calibrationData;
	Bme280Data dataParameters;
	
	//Diodes
	DDRD |= (1<<D1);
	//PORTD |= (1<<D1);
	
	//start
	sei();
	
	sendMessageFlag = false;
	
	while (1) 
	{
		if(transmitterFlag)
		{
			if (interruptFlag)
			{
				interruptFlag = false;
				int success = nRF24L01_transmit_success(nrf);
				if (success != 0)
					nRF24L01_flush_transmit_message(nrf);
				
				transmitterFlag = false;
				nRF24L01_listen(nrf, 0, address);
			}
			
			if (sendMessageFlag)
			{
				
				nRF24L01_retry_transmit2(nrf);
				sendMessageFlag = false;
				nRF24L01Message msg;
				
				//BME280
				uint8_t parameters[PARAM_SIZE];
				result = bme280ReadCalibrationData(&calibrationData);
				readingBME280ToTab(parameters,result,calibrationData,dataParameters);
				//------
				
				myMemcpy(parameters,msg.data);
				msg.length = strlen((char *)msg.data) + 1;
				nRF24L01_transmit(nrf, address, &msg);
			}
			
		}
		else	//listening
		{
			if (interruptFlag)
			{
				interruptFlag = false;
				while (nRF24L01_data_received(nrf))
				{
					nRF24L01Message msg;
					nRF24L01_read_received_data(nrf, &msg);
					processMessage((char *)msg.data);
				}
				nRF24L01_listen(nrf, 0, address);
			}
			nRF24L01_listen(nrf, 0, address);
		}
		
		
	}

	return 0;
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

void myMemcpy(uint8_t* input, uint8_t* output)
{
	int i;
	for(i=0; i<PARAM_SIZE-1; i++)
	output[i] = input[i];
	
	output[i] = '\0';
}

void processMessage(char *message)
{
	if (strcmp(message, "SEND") == 0)
	{
		transmitterFlag = true;
		sendMessageFlag = true;
	}
}

uint8_t* readingBME280ToTab(uint8_t* parameters, uint16_t result, Bme280CalibrationData calibrationData, Bme280Data dataParameters)
{
	dataParameters = sensor_parameters(result,calibrationData);
	
	//float temperature = dataParameters.temperatureC;
	//float humidity = dataParameters.humidityPercent;
	//float pressure = dataParameters.pressurePa/100;
	
	parameters[0] = (int)dataParameters.temperatureC;
	parameters[1] = (int)dataParameters.humidityPercent;
	int pressureInTwo[2];
	numberInTwo(pressureInTwo,(int)(dataParameters.pressurePa/100));
	parameters[2] = pressureInTwo[0];
	parameters[3] = pressureInTwo[1];
	
	
	if(flag == 1)
	{
		int a = 123;
		parameters[3] = a;
		flag =0;
	}
	
	return parameters;
}

Bme280Data sensor_parameters(uint16_t result, Bme280CalibrationData calibrationData)
{
	Bme280Data data;
	if (result == BME280_OK)
	{
		result = bme280ReadData(BME280_OSS_1, BME280_OSS_1, BME280_OSS_1, &data, &calibrationData);
		if (result == BME280_OK)
		{
			return data;
		}
	}
	return data;
}

nRF24L01 *setup_nrf(void) {
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


ISR(INT0_vect) 
{
	interruptFlag = true;
}






