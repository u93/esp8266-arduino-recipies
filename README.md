# ESP8266
In the following examples will be showed the use of different libraries and protocols by the ESP8266 using Arduino Core and Arduino IDE


## Arduino IDE Installation Process
- Follow the [Installation Process](https://arduino-esp8266.readthedocs.io/en/latest/installing.html) in Arduino Core Documentation


## ESP8266 Wiring and Power Supply

## Flash Firmware to ESP8266
- ESP8266 has to be set in BOOTLOADER mode, for this to happen the ESP8266 needs to be powered up with GP 0 pin set to "ground" in order to set it in BOOTLOADER mode.
  - If you have a firmware already uploaded you can confirm that the Wi-Fi module is ready to receive a new firmware due to "Serial Logs" absence. 
  - In some cases you will see some errors like "espcomm_upload_mem failed" twice this could be the ESP9266 is not set in BOOTLOADER mode.
  - Always make sure that in Tools/Port of the Arduino IDE you have selected the USB Port that has connection to the Wi-Fi module.
    - In Linux (Ubuntu in this case) could exist "lack of permissions to use the USB port", in that case you will see an error like "espcomm_open failed" and "espcomm_upload_mem failed". In that case the next command has to be executed every time that you plan to set a new firmware using the Serial Port "sudo chmod -R 777 /dev/ttyUSB0" changing "ttyUSB0" for you actual serial port with connection to the ESP8266 


## Integration examples
- dweet.io
  - To learn about Dweet visit the [website](http://dweet.io/).
  - To see a way of how to implement review the next [example](https://github.com/eebf1993/myesp8266/blob/master/examples/dweet.io_NTP/dweet.io_NTP.ino).
- API Gateway
  - To use API Gateway from the ESP8266 side is only necessary to use WiFiClientSecure library for HTTPS communication.
  - A deployment of API Gateway will be needed in order to test the functionality.
  - To see a way of how to implement review the next [example](https://github.com/eebf1993/myesp8266/tree/master/examples/DNS-NTP_HTTPS-API_Gateway).
- AWS IoT Core
  - Use of [AWS-SDK]() for ESP8266 for use of IoT Core.
  - To see a way of how to implement review the next [example]().
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
- [Ticker library](https://github.com/esp8266/Arduino/tree/master/libraries/Ticker) is used to trigger functions every a certain time. Make sure to follow [recomendations](http://arduino-esp8266.readthedocs.io/en/latest/libraries.html) from ESP8266 Arduino Core.

## OTA Upgrades
- Follow [OTA documentation](http://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html) on Arduino Core for understanding on how OTA Upgrade works and the several ways to do it.
- Read the following [tutorial](https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/) that acts as both example and documentation.
- Here can be found an [Example Code](https://github.com/esp8266-examples/ota-basic) plus a tutorial on how implement OTA functions and how to use them. 

## SPIFFS
- [SPIFFS](https://github.com/esp8266/Arduino/blob/master/cores/esp8266/spiffs/README.md) stands for SPI Flash File System, and can be used in the ESP8266 to store files like certificates for secure communication.
- For information about how the File System works, itś limitations and how to implement ypur code visit [here](https://github.com/esp8266/Arduino/blob/master/doc/filesystem.rst).
  - Information about your chip flash size can be obtained using ESP-specific SDK as hown in [example](https://github.com/eebf1993/myesp8266/blob/master/examples/SPIFFS_Mount_Format_Listing/SPIFFS_Mount_Format_Listing.ino).

## Espressif Documentation
- [ESP8266_Datasheet](https://www.espressif.com/sites/default/files/documentation/0a-esp8266ex_datasheet_en.pdf)
- [ESP8266_Technical_Reference](https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf)
- [ESP8266_Documents](https://www.espressif.com/en/support/download/documents?keys=&field_type_tid%5B%5D=14)



## Internal SDK use
- Arduino Core for ESP8266 allows the use for ESP-specific APIs with the objective of get information from board and firmware, and trigger certain functionalities like deepsleep and measuring power supply voltage. The information on how to use these [ESP-specific APIs](https://github.com/Imroy/pubsubclient) can be found in Arduino Core Documentation.
