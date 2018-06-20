# ESP8266
In the following examples will be showed the use of different libraries and protocols by the ESP8266 using Arduino Core and Arduino IDE


## Arduino IDE Installation Process
- Follow the [Installation Process](https://arduino-esp8266.readthedocs.io/en/latest/installing.html) in Arduino Core Documentation


## ESP8266 Wiring and Power Supply


## Integration examples
- dweet.io
- API Gateway
- AWS IoT Core
- S3
- MQTT Mosquitto Broker
- WebSockets Communication
- Internal Web Server and access using RESTful APIs

## Network Protocols
- DNS
- NTP
- HTTP
- HTTPS
- MQTT
  - [MQTT Essentials](https://www.hivemq.com/mqtt-essentials/)
  - [pubsubclient](https://github.com/Imroy/pubsubclient) as the library for MQTT use in ESP8266 with Arduino IDE
    - MIT Licence
- WebSockets
  - [Websockets Tutorial](https://www.tutorialspoint.com/websockets/index.htm)
  - [Websockets Library](https://github.com/morrissinger/ESP8266-Websocket) for client and server implementation

## Scheduling Events
- [Ticker library](https://github.com/esp8266/Arduino/tree/master/libraries/Ticker) is used to trigger functions every a certain time. Make sure to follow [recomendations](http://arduino-esp8266.readthedocs.io/en/latest/libraries.html) from ESP8266 Arduino Core

## OTA Upgrades from Remote Server



## Espressif Documentation
- [ESP8266_Datasheet](https://www.espressif.com/sites/default/files/documentation/0a-esp8266ex_datasheet_en.pdf)
- [ESP8266_Technical_Reference](https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf)
- [ESP8266_Documents](https://www.espressif.com/en/support/download/documents?keys=&field_type_tid%5B%5D=14)



## Internal SDK use
- Arduino Core for ESP8266 allows the use for ESP-specific APIs with the objective of get information from board and firmware, and trigger certain functionalities like deepsleep and measuring power supply voltage. The information on how to use these [ESP-specific APIs](https://github.com/Imroy/pubsubclient) can be found in Arduino Core Documentation 
