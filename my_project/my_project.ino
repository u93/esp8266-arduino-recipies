#include <ESP8266WiFi.h> //library used to handle WiFi in ESP8266using Arduino Core
#include <WiFiClientSecure.h> //enable HTTPS to handle Cloud Services like API Gateway
#include "FS.h" //library used to handle the ESP8266´s File System
#include <WiFiUdp.h> //general library used to handle UDP send and receive. In this case is going to be used to handle DNS and NTP communications
#include <Ticker.h>// This library allows the creation of an object that will call a given function after a certain period
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>



//Control Variables
int FS_COUNTER;

//defined ssid name and password used to connect to WiFi
const char* ssid = "Highfive";
const char* password = "mariposa921";
//const char* ssid = "dd-wrt";
//const char* password = "11111111";

//WiFiUdp objects need a port to listen, a buffer for incoming packets and a reply message
WiFiUDP udp; //here an instance of the WiFiUdp object was created
unsigned int localUdpPort = 4210;
unsigned int rPort;
int RemotePort; //Defined to be a Global Variable due the unstable values that this variable can get when receiving packets

//Here are defined the Buffer Size for every packet used in Application Layer of TCP/IP
const int NTP_PACKET_SIZE = 48;
const int DNS_PACKET_SIZE = 31;

int packetSize = 0; //packetSize variable initializated to use readDNSpacket as a function
byte packetBuffer[255]; //Buffer the generic buffer for receiving packets

//Here are defined instances of the class IPAddress to use for functionality and testing
IPAddress dnsserver(8, 8, 8, 8);
//IPAddress uLaptop(10, 0, 0, 132);

//Here are defined ESP8266 native SDK variables and functions
ADC_MODE (ADC_VCC); //Necessary requirement to use ESP.getVcc(), allows to reconfigure the ADC to measure from VCC pin

const int HTTP_PORT = 80;
//Here are defined all the variable srelated to dweet.io
const char* DWEET_HOST = "dweet.io";

//Here are defined all the variable srelated to ipinfo.io
const char* IPINFO_TOKEN = "54b63ddea9d7b8";
const char* IPINFO_HOST = "ipinfo.io";

//Ticker library related variables, constants and objects 
bool FIRST_FLAG;
bool HELP_FLAG = false;
Ticker Networking;
Ticker Info;

//Others
unsigned long previousMillis = 0;
long int unix_time;

const char API_ROOT[] = "icvvcm8olb.execute-api.us-east-1.amazonaws.com";
WiFiClientSecure secureclient;
const int SSL_PORT = 443;
//OTA related variables, contants and objects
int firmware_version;
int CHIP_ID;
int model;

//ESP8266 Status Check function realted variables, constants and objects
int heap_free_size;
long cpu_inst_cycle;
long power_supply_v;


void setup() {
 
    Serial.begin(115200); //Initiate Serial Cx
    WiFi.begin(ssid, password); //Set ESP8266 in STA and pass credentials to WiFi AP
  
    while (WiFi.status() != WL_CONNECTED) //Waiting for ESP8266 to be connected
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Wi-Fi Connected");
    Serial.println(WiFi.localIP()); //Print the IP obtained by the router
    udp.begin(localUdpPort); //start listening to UDP packets on port 4210

    //Check Internet connectivity Algorithm
    INT_CHK: sendDNS_NTPpacket (dnsserver);
    IPAddress int_chk = readDNSpacket();
    Serial.println(int_chk);
    if (!int_chk){
        Serial.println("No Internet Connection, check router connectivity");
        delay(10000);
        goto INT_CHK; 
    } 
    else {
        Serial.println("There is Internet Connection");
        Serial.println(int_chk);
    }

    //Get unix time for Firmware Check and Update information gathering
    sendNTPpacket (int_chk);
    unix_time = readNTPpacket();
    //Prints ESP8266-ESP01 Flash Memory information, like Size (from the SDK point of view and the frequency in Hz
    CHIP_ID = getESP8266_fixed_info();
    firmware_version = 6;
    delay (10);
  
    //Here will be the code for firmware check and upgrade process
    //firmware_check(firmware_version, model, CHIP_ID, unix_time);
    //t_httpUpdate_return ret = ESPhttpUpdate.update("http://server/file.bin");
    //ESPhttpUpdate.update("192.168.0.2", httpPort, "/arduino.bin");
  
 
  
    //Mounting File System, if File System mounting process fails, it will try it again
    up_FS: SPIFFS.begin();
    if (!SPIFFS.begin()) {
        Serial.println("Unable to mount File System");
        delay (500);
        FS_COUNTER++;
        if (FS_COUNTER>=3) {  //Control Structure using "goto", "if" and "Control Variable"
            Serial.println("FATAL ERROR MUNTING FILE SYSTEM! SYSTEM WILL RESTART");
            Serial.println(ESP.getResetReason());
            ESP.restart();  //Restart the system if there was a failure mountng the FS
        }
        goto up_FS;
    }
    else {
        Serial.println("File System mounted");
    }
    //SPIFFS.format();
    //while (!SPIFFS.format()) {
   
    //}
    //Serial.println("SPIFFS Formatted");
    //Dir dir = SPIFFS.openDir("/");        //Opens a directory given its absolute path. Returns "Dir"
    //while (dir.next()) {                  //Returns true if there are files to iterate
        //Serial.print(dir.fileName());
        //File f = dir.openFile("r");
        //Serial.println(f.size());
        //f.close();
    //}
  
    Networking.attach(5, udp_tcp_Networking_flag); //Activate UDP + TCP functionality every 5 seconds
    Info.attach(10, getESP8266_variable_info); //Prints information relative to the code and board like free Heap, number of CPU instruction count per cycle and the feed received from the Power Supply every 10 seconds
}

   

void loop() {
  
    while(FIRST_FLAG) {
        Serial.println("Sending DNS packet to Google´s DNS");
        sendDNS_NTPpacket (dnsserver);
        IPAddress response = readDNSpacket();
        sendNTPpacket (response);
        unix_time = readNTPpacket();
        Serial.println(unix_time);
        ipinfo_test();
        //dweeting(); //This lines below handle Dweet.io communication byt TCP-HTTP Communication
        FIRST_FLAG = false; //This line finishes the while loop for when Ticker activates communication, the communication can be successfully implemented
    }
    
}


//Here will be defined all the flags (interrumptions) for "Ticker" functionality and LESS CODE AS POSSIBLE IN INTERRUPTS!!!
void udp_tcp_Networking_flag(){
    FIRST_FLAG = true;
}

//Here will be defined all the UDP traffic functions (DNS, NTP and receivers)
unsigned long sendDNS_NTPpacket(IPAddress& address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, 255);
    // Initialize values needed to form DNS request
    packetBuffer[0] = 0x00;   // ID
    packetBuffer[1] = 0x01;
    packetBuffer[2] = 0x01;   // Flags --> Recursive Desired=1
    packetBuffer[3] = 0x00;
    packetBuffer[4] = 0x00;   //Number of Questions, in this case Questions=1
    packetBuffer[5] = 0x01;
    // 6 bytes of zero for Answer RRs, Authority RRs and Aditonal RRs
    packetBuffer[12]  = 0x04;  //Query, in this case: time.nist.gov
    packetBuffer[13]  = 0x74;
    packetBuffer[14]  = 0x69;
    packetBuffer[15]  = 0x6D;
    packetBuffer[16]  = 0x65;
    packetBuffer[17]  = 0x04;
    packetBuffer[18]  = 0x6E;
    packetBuffer[19]  = 0x69;
    packetBuffer[20]  = 0x73;
    packetBuffer[21]  = 0x74;
    packetBuffer[22]  = 0x03;
    packetBuffer[23]  = 0x67;
    packetBuffer[24]  = 0x6F;
    packetBuffer[25]  = 0x76;
    packetBuffer[26]  = 0x00;
    //Here the name of the domain that the ESP8266 is querying ends
    packetBuffer[27]  = 0x00;   //Type, in this case Type=A
    packetBuffer[28]  = 0x01;
    packetBuffer[29]  = 0x00;   //Class, in this case Class = IN
    packetBuffer[30]  = 0x01;

    //Serial.println("3");

    // all DNS fields have been given values, now
    // you can send a packet requesting a the IP Adresses of a domain:
    udp.beginPacket(address, 53); //DNS requests are to port 53
    //Serial.println("4");
    udp.write(packetBuffer, DNS_PACKET_SIZE);
    //Serial.println("5");
    udp.endPacket();
    Serial.println("DNS Packet sent");
    //Serial.println(packetBuffer);
    //Serial.println("6");
}

unsigned long sendNTPpacket(IPAddress& address) {
    //Serial.println("1");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    //Serial.println("2");
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    //Serial.println("3");

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    //Serial.println("4");
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    //Serial.println("5");
    udp.endPacket();
    //Serial.println("6");
    Serial.println("NTP Packet sent");
}


IPAddress readDNSpacket() {
    delay(500);
    int packetSize = udp.parsePacket();
    if(packetSize) {
        //details of the packet received
        IPAddress remote = udp.remoteIP();
        int RemotePort = udp.remotePort();
        Serial.print("   readpacket() --- The Size of the packet received is: "); Serial.print(packetSize); Serial.println(" bytes");
        Serial.print("   readpacket() --- The packet received comes from: "); Serial.print(remote); Serial.println(" IP Address");
        Serial.print("   readpacket() --- And from port: "); Serial.println(RemotePort);
        if (RemotePort == 53) {
            udp.read(packetBuffer, packetSize);
            Serial.print("      readDNSpacket() -if-- Let´s see if this works, IP Address received is: ");
            Serial.print(packetBuffer[packetSize - 4]); Serial.print("."); Serial.print(packetBuffer[packetSize - 3]); Serial.print("."); Serial.print(packetBuffer[packetSize - 2]); Serial.print("."); Serial.println(packetBuffer[packetSize - 1]);
        }
        IPAddress dnsresponse(packetBuffer[packetSize - 4], packetBuffer[packetSize - 3], packetBuffer[packetSize - 2], packetBuffer[packetSize - 1]);
        Serial.print("   readpacket() --- Should return: "); Serial.println(dnsresponse);
        return dnsresponse;
    }
  
    else {
        IPAddress BAD_CHK(0, 0, 0, 0);
        return BAD_CHK;
    }
}

int readNTPpacket() {
    unsigned long epoch;
    delay(500);
    int packetSize = udp.parsePacket();
    if (packetSize) {
        //details of the packet received
        IPAddress remote = udp.remoteIP();
        int RemotePort = udp.remotePort();
        Serial.print("   readpacket() --- The Size of the packet received is: "); Serial.print(packetSize); Serial.println(" bytes");
        Serial.print("   readpacket() --- The packet received comes from: "); Serial.print(remote); Serial.println(" IP Address");
        Serial.print("   readpacket() --- And from port: "); Serial.println(RemotePort);
        //Serial.print("   readpacket() --- The value of packetBuffer´s last 4-bytes (should be `0.0.0.0´ is: "); Serial.print(packetBuffer[packetSize - 4]); Serial.print("."); Serial.print(packetBuffer[packetSize - 3]); Serial.print("."); Serial.print(packetBuffer[packetSize - 2]); Serial.print("."); Serial.println(packetBuffer[packetSize - 1]);
        //return RemotePort;
        if (RemotePort == 123) {
            udp.read(packetBuffer, packetSize);
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            Serial.print("Seconds since Jan 1 1900 = " );
            Serial.println(secsSince1900);

            // now convert NTP time into everyday time:
            Serial.print("Unix time = ");
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            epoch = secsSince1900 - seventyYears;
            // print Unix time:
            Serial.println(epoch);
         }
    }    
    Serial.println(epoch);
    return epoch;
}

int getESP8266_fixed_info() {
    int chip_id = ESP.getChipId();
    const char* SDK_version = ESP.getSdkVersion();
    int *allocate_test; allocate_test = (int *) malloc(1); *allocate_test = ESP.getCpuFreqMHz(); free(allocate_test);
    int sketch_size = ESP.getSketchSize();
    int free_sketch_size = ESP.getFreeSketchSpace();
    int flash_size = ESP.getFlashChipSize();
    int real_flash_size = ESP.getFlashChipRealSize();
    int flash_speed = ESP.getFlashChipSpeed();
    Serial.print("Chip ID: "); Serial.println(chip_id);
    Serial.print("Core Version: "); Serial.println(ESP.getCoreVersion());
    Serial.print("SDK Version: "); Serial.print(SDK_version); 
    Serial.print("CPU Frequency: "); Serial.print(*allocate_test); Serial.println("MHz"); 
    Serial.print("Sketch Size: "); Serial.print(sketch_size); Serial.print("and is left free: "); Serial.println(free_sketch_size);
    Serial.print("Flash Chip Size: "); Serial.println(flash_size); 
    Serial.print("Real Flash Chip Size: "); Serial.println(real_flash_size);
    Serial.print("Flash Chip Speed: "); Serial.println(flash_speed);
    return chip_id;
}  
  

void getESP8266_variable_info() {
    int heap_free_size = ESP.getFreeHeap();
    Serial.print("Free Heap: "); Serial.println(heap_free_size);
    long cpu_inst_cycle = ESP.getCycleCount();
    Serial.print("CPU Instruction cycle count: "); Serial.println(cpu_inst_cycle);
    long power_supply_v = ESP.getVcc();
    Serial.print("Power Supply measure: "); Serial.println(power_supply_v);
}

void dweeting() {
    WiFiClient client;
    Serial.println(DWEET_HOST);
    if (!client.connect(DWEET_HOST, HTTP_PORT)) { //Enables TCP Connection with dweet.io
        Serial.println("connection failed");
        return;
    } 
    client.print(String("GET /dweet/for/myesp8266-esp01?unix_time=") + String(unix_time) + " HTTP/1.1\r\n" + "Host: " + DWEET_HOST + "\r\n" + "Connection: close\r\n\r\n"); //Sends the HTTP GET request to dweet.io             
    delay(10);
    Serial.println("Begin reading");
    while (client.connected()) { //Reads and print response of the server
        while(client.available()) {
            String line; //= "Begin to read"; Serial.print(line);
            line = client.readStringUntil('\r');
            Serial.print(line);
        }  
    }    
    Serial.println("");Serial.println("Finish reading");
}

void ipinfo_test() {
    WiFiClient client;
    Serial.println(IPINFO_HOST);
    if (!client.connect(IPINFO_HOST, HTTP_PORT)) { //Enables TCP Connection with dweet.io
        Serial.println("connection failed");
        return;
    }   
    client.print(String("GET /8.8.8.8/geo") + " HTTP/1.1\r\n" + "Host: " + IPINFO_HOST + "\r\n" + "Connection: close\r\n\r\n"); //Sends the HTTP GET request to ipinfo.io
    delay(10);            
    Serial.println("Begin reading");
    while (client.connected()) { //Reads and print response of the server
        while(client.available()) {
            String line; //= "Begin to read"; Serial.print(line);
            line = client.readStringUntil('\r');
            Serial.print(line);
        }  
    }
    Serial.println("");Serial.println("Finish reading");
}              

char status_report() {
    delay (10);
    if (secureclient.connect(API_ROOT, SSL_PORT)) {
        Serial.println("API Gateway Connected");
        String URL = String("/dev/develop/esp8266/statusreport?freeheap=") + heap_free_size + String("&vcc=") + power_supply_v + String("&id=") + CHIP_ID + String("&time=") + unix_time;
        Serial.println(URL);
        secureclient.println("GET " + URL + " HTTP/1.1");
        secureclient.print("Host: ");
        secureclient.println(API_ROOT);
        secureclient.println("User-Agent: arduino/1.0");
        secureclient.println("Connection: close");
        secureclient.println("");
    }
    while (!secureclient.available()) {
        delay(100);
    }
    while (secureclient.connected()) { //Reads and print response of the server
        while(secureclient.available()) { 
            String line;
            line = secureclient.readStringUntil('\r');
            Serial.print(line);
        }      
    }
}

char firmware_check(int fw_version, int model, int chip_id, long int current_time) {
    delay (10);
    if (secureclient.connect(API_ROOT, SSL_PORT)) {
        Serial.println("API Gateway Connected");
        Serial.println(fw_version);
        String URL = String("/dev/develop/esp8266/update?version=") + firmware_version + String("&model=") + model + String("&id=") + CHIP_ID + String("&time=") + unix_time;
        Serial.println(URL);
        secureclient.println("GET " + URL + " HTTP/1.1");
        secureclient.print("Host: ");
        secureclient.println(API_ROOT);
        secureclient.println("User-Agent: arduino/1.0");
        secureclient.println("Connection: close");
        secureclient.println("");
    }
    while (!secureclient.available()) {
        delay(100);
    }
    while (secureclient.connected()) { //Reads and print response of the server
        while(secureclient.available()) { 
            String line;
            line = secureclient.readStringUntil('\r');
            Serial.print(line);
        }      
    }
}
