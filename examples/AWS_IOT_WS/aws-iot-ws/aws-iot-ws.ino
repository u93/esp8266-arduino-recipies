#include <ESP8266WiFi.h> //library used to handle WiFi in ESP8266using Arduino Core
#include <WiFiUdp.h> //general library used to handle UDP send and receive. In this case is going to be used to handle DNS and NTP communications
#include <Ticker.h>// This library allows the creation of an object that will call a given function after a certain period
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "AWSWebSocketClient.h"
#include <MQTTClient.h>
#include <IPStack.h>
#include <Countdown.h>
#include <PubSubClient.h>

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

//Ticker library related variables, constants and objects 
bool FIRST_FLAG;
Ticker Networking;
Ticker Info;

//Others
unsigned long previousMillis = 0;
long int unix_time;

//AWS IOT config, change these:
char aws_endpoint[]    = "adbf07ovrzhft-ats.iot.us-east-1.amazonaws.com";
char aws_key[]         = "AKIAJPBHY7UF7CAKKJWA";
char aws_secret[]      = "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";
char aws_region[]      = "us-east-1";
const char* aws_topic  = "$aws/things/esp8266/shadow/update";
int port = 443;
const char* twilio_topic = "test"; 

//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

AWSWebSocketClient awsWSclient(1000);

PubSubClient client(awsWSclient);

//# of connections
long connection = 0;

//ESP8266 Status Check function realted variables, constants and objects
int heap_free_size;
long cpu_inst_cycle;
long power_supply_v;

void setup() {
  // put your setup code here, to run once:
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
    
    Networking.attach(5, udp_tcp_Networking_flag); //Activate UDP + TCP functionality every 5 seconds

    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    getESP8266_variable_info();
    if (connect()) {
        sendmessage ();
    }
}


void loop() {
  
    while(FIRST_FLAG) {
        Serial.println("Sending DNS packet to Google´s DNS");
        sendDNS_NTPpacket (dnsserver);
        IPAddress response = readDNSpacket();
        sendNTPpacket (response);
        unix_time = readNTPpacket();
        Serial.println(unix_time);
        //dweeting(); //This lines below handle Dweet.io communication byt TCP-HTTP Communication
        FIRST_FLAG = false; //This line finishes the while loop for when Ticker activates communication, the communication can be successfully implemented
    }
    
}

//connects to websocket layer and mqtt layer
bool connect () {
    if (client.connected()) {    
        client.disconnect ();
    }  
    //delay is not necessary... it just help us to get a "trustful" heap space value
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");

    //creating client id
    char* clientID = "esp8266";    
    client.setServer(aws_endpoint, port);
    if (client.connect(clientID)) {
      Serial.println("Thing connected to AWS IoT Broker");     
      return true;
    } else {
      Serial.print("Failed to connect to AWS IoT Broker, rc=");
      Serial.print(client.state());
      return false;
    }    
}



void sendmessage () {
    //send a message   
    char buf[100];
    //strcpy(buf, "{\"state\":{\"reported\":{\"power_supply\": " + String("power_supply_v") + "}, \"heap_free_size\":{\"on\": "false"}}}");   
    String json = String("{\"state\":{\"reported\":{\"power_supply\": ") + power_supply_v + String(", \"heap_free_size\": ") + heap_free_size + String("}}}");
    Serial.println(json);
    char __json[json.length()+1];
    json.toCharArray(__json, sizeof(__json));
    Serial.println(__json);
    int rc = client.publish(aws_topic, __json); 
}

void udp_tcp_Networking_flag(){
    FIRST_FLAG = true;
}

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

void getESP8266_variable_info() {
    int heap_free_size = ESP.getFreeHeap();
    Serial.print("Free Heap: "); Serial.println(heap_free_size);
    long cpu_inst_cycle = ESP.getCycleCount();
    Serial.print("CPU Instruction cycle count: "); Serial.println(cpu_inst_cycle);
    long power_supply_v = ESP.getVcc();
    Serial.print("Power Supply measure: "); Serial.println(power_supply_v);
}
