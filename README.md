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

![Image of Station](https://github.com/GeoGuy-ca/DistributedWaterLevelMonitor/blob/master/photos/20200525_131511.jpg)

The station is constructed inside of a reusable sealing food container, all components were hot glued to the lid of the container and then cables for external sensors were run through holes and sealed with silicon. 

The water level sensor was attached to a 1x4, which was in tern screwed to the side of a bridge holding it at a steady position above the water.

![Image of Ultrasonic Sensor](https://github.com/GeoGuy-ca/DistributedWaterLevelMonitor/blob/master/photos/20200525_131445.jpg)

First Deployment:
![Image of Deployment](https://github.com/GeoGuy-ca/DistributedWaterLevelMonitor/blob/master/photos/20200428_150403.jpg)

## The Webserver
  The webserver is built using Python Flask and PostgreSQL, and is designed to provide an easy to use view of the data collected by the sensors. 
  
![Image of Waterlevel graph](https://github.com/GeoGuy-ca/DistributedWaterLevelMonitor/blob/master/photos/Screenshot%20from%202020-05-25%2013-46-06.png)

The webserver exposes the following endpoints
 * http://<domain>/rockblock this is the URL used to add data, and should only be called by the RockBlock api, it takes the following information: imei, momsn, transmit_time, iridium_latitude, iridium_longitude, iridum_cep, payload 
  * http://<domain>/deployments returns a JSON list of all current and past deployments with their locations and dates of use
  * http://<domain>/resutls?deployment=<id> Returns a JSON list of all results recorded by deployment id, the deployment id can be determined from http://<domain>/deployments
  * http://<domain>/graph?deployment=<id> Returns a graphical representation of results
  * still under development is a map that will show all deployments, clicking on pins in the map will allow linking to the graph page for the deployment in question.
