# Water Quality Monitoring and Alert Systemm
An embedded IoT system for monitoring domestic water quality and generating alerts when abnormal conditions are detected. The project combines microcontroller-based sensing, wireless communication, and system integration for real-time monitoring.

## Overview

This project was developed to monitor water conditions in a domestic environment using embedded devices and sensor-based data collection. The system focuses on detecting abnormal water quality indicators and sending alerts for early response.

## Main Features

- Read and process water-related sensor data
- Monitor abnormal conditions in real time
- Display system status locally
- Support wireless communication for remote monitoring
- Trigger alerts when measured values exceed safe thresholds
- Integrate embedded sensing with AI-assisted analysis

## Hardware / Platforms

- STM32
- ESP32
- Raspberry Pi 5
- Water quality sensors
- LCD I2C display
- Wireless communication module

## Interfaces and Communication

- UART
- I2C
- WiFi

## System Architecture

- **STM32**: acquires sensor data and handles embedded control tasks  
- **ESP32**: provides wireless communication and IoT connectivity  
- **Raspberry Pi 5**: supports higher-level processing and AI-assisted analysis  
- **LCD**: displays local system information and status  

## Development Tools

- STM32CubeMX
- Keil uVision / Embedded C
- Arduino IDE
- Python (for AI-assisted processing if applicable)

## My Responsibilities

- Developed embedded firmware for sensor data acquisition
- Integrated communication between system modules
- Implemented alert logic for abnormal conditions
- Supported testing, debugging, and system integration
- Improved system reliability for continuous monitoring

## Result

The system can collect water-related data, detect abnormal conditions, and provide real-time alert support through embedded and wireless components. It demonstrates practical experience in firmware development, sensor interfacing, IoT communication, and multi-device integration.

## Future Improvements

- Improve sensor calibration and measurement accuracy
- Add cloud dashboard or mobile app integration
- Optimize AI-based classification for water condition analysis
- Expand support for more sensors and automation features
