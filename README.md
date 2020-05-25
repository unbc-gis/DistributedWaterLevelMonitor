# DistributedWaterLevelMonitor
Python Application for monitoring waterlevel at provincial scale

Installation:
Install postgresql and create a database and user, add login credentials to passwd.cred

Add new deployment:
Use the script addDeployment.py, setting the IMEI, Location and Mounting heigt, lines 5-7

# The Project
This project is being developed as part of a collaboration between the the UNBC GIS Lab and the BC Ministry of Forests Lands and Natural Resourses. The purpose of this project is to develop a low cost distributed sensor network for monitoring water levels and flood events across the province. 

The individual stations communicate via Rockblock satillite modems using the Iridium constilation, and are built using Arduino microcontrollers. 

## The Station
The station is built using an Arduino Microcontroller (Original prototype was Arduino Micro, however Arduino Mega is the recomened board as it contains sufficent storage space for SD card Library for more frequent logging).

The station makes use of the following sesors. 
* MaxBotix MB7052-100 Ultrasonic Range Finder for water level monitoring
* Adafruit DS18b20 for water temperature monitoring
* Adafruit BME280 for weather monitoring (Temperature, Humidity, Pressure)

Data transmission: RockBlock 19354
Data Logging: Adafuit Micro-SD Breakout+
Real time Clock: PCF 8523
Power is provided by a car battery and a 6-36v to 5v voltage regulator

