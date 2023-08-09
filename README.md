# Electronic system for measuring temperature and humidity with radio data transmission

![VideoCapture_20230809-173911](https://github.com/MachalKT/Electronic-system-for-measuring-temperature-and-humidity-with-radio-data-transmission/assets/86099181/effef892-fa23-4358-a098-4de387d5d16e)

## Description

The system consists of two devices, a base station and a sensor. The sensor measures parameters such as (temperature, humidity and pressure) and transmits the data via radio to the base station. Base station transmits information about sending data to the sensor every specified period of time. 
After obtaining data from the sensor, the data is saved in the EEPROM memory of the microcontroller. Data is displayed on the LCD display. The buttons control the displayed data.

## Hardware

Base station:
- microcontroller AVR ATmega328p
- Real-Time clock DS1307
- nRF24L01+
- LCD 2x16

Sensor:
- microcontroller AVR ATmega328p
- sensor BME280
- nRF24L01+
