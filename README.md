# ai-water-quality-monitoring-system
# Water Quality Monitoring and Alert System

An embedded IoT-based water quality monitoring system designed to collect sensor data, detect abnormal conditions, and provide real-time alerts. The project integrates multiple embedded platforms, wireless communication, and AI-assisted analysis to improve domestic water safety monitoring.

## Project Overview

The system continuously monitors water quality parameters using sensor nodes and processes the collected data through a distributed embedded architecture. When abnormal conditions are detected, alerts are generated to enable timely responses and preventive actions.

## Key Features

* Real-time water quality monitoring
* Sensor data acquisition and processing
* Local status display via LCD
* Wireless communication and remote monitoring
* Threshold-based abnormal condition detection
* Alert generation for unsafe water conditions
* AI-assisted analysis on Raspberry Pi

## System Architecture

### STM32

* Acquires sensor data
* Performs embedded control and preprocessing
* Communicates with ESP32 via UART

### ESP32

* Provides Wi-Fi connectivity
* Transmits data for remote monitoring
* Acts as the IoT communication gateway

### Raspberry Pi 5

* Performs higher-level data processing
* Supports AI-assisted water condition analysis
* Handles advanced monitoring functions

### LCD I2C Display

* Displays sensor readings
* Shows system status and alert information

## Hardware Components

* STM32 Microcontroller
* ESP32 Development Board
* Raspberry Pi 5
* Water Quality Sensors
* LCD 16x2 I2C Display

## Communication Interfaces

* UART
* I2C
* Wi-Fi

## Development Tools

* STM32CubeMX
* Keil uVision
* Embedded C
* Arduino IDE
* Python

## My Contributions

* Developed embedded firmware for sensor interfacing and data acquisition
* Implemented communication between STM32, ESP32, and Raspberry Pi
* Designed abnormal-condition detection and alert mechanisms
* Performed system integration, testing, and debugging
* Improved system stability for continuous operation

## Results

The system successfully monitors water conditions, detects abnormal events, and provides real-time alerts through embedded and wireless communication technologies. The project demonstrates practical experience in:

* Embedded Firmware Development
* STM32 Programming
* Sensor Interfacing
* UART/I2C Communication
* IoT System Integration
* ESP32 Development
* Raspberry Pi Applications
* Real-Time Monitoring Systems

## Future Improvements

* Improve sensor calibration accuracy
* Develop cloud-based monitoring dashboard
* Add mobile application support
* Enhance AI-based water quality classification
* Support additional sensors and automation features
<img width="945" height="709" alt="image" src="https://github.com/user-attachments/assets/ddbe6c33-510b-4f43-ad27-7596c522326a" />
<img width="945" height="553" alt="image" src="https://github.com/user-attachments/assets/e1ed112f-5af5-4cc7-8487-fe86864a6418" />

