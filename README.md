# EE5 Project Helios

This GitLab contains all the code files necessary for our smart sensor system that will monitor Compartment Fire Behaviour Training.

This particular branche contains the code for the sensor device. The functionality of the sensor device is to receive messages from a RN4870 BLE chip over UART and respond to them accordingly. The most important packet is the temperature measurement. This temperature is taken from a K type thermocouple. The signal from the thermocouple is amplified, compensated and linearized by a MAX31856. The temperature is then read out of the registers of the MAX31856 by the central PIC18F25k50 microcontroller over SPI. When the PIC18F25k50 is not being talked to it goes into a sleep mode. Also while being awake it uses power efficiently by turning off unused modules.

All the code in this branche was written by Vincent Surkijn.
