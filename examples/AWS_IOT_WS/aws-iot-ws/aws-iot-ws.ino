#include <ESP8266WiFi.h> //library used to handle WiFi in ESP8266using Arduino Core
#include <WiFiUdp.h> //general library used to handle UDP send and receive. In this case is going to be used to handle DNS and NTP communications
#include <Ticker.h>// This library allows the creation of an object that will call a given function after a certain period
#include <ESP8266Ping.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "AWSWebSocketClient.h"
#include <MQTTClient.h>
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

//Here are defined instances of the class IPAddress to use for functionality and testing
IPAddress dnsServer(8, 8, 8, 8);
//IPAddress uLaptop(10, 0, 0, 132);

//Here are defined ESP8266 native SDK variables and functions
ADC_MODE (ADC_VCC); //Necessary requirement to use ESP.getVcc(), allows to reconfigure the ADC to measure from VCC pin

//Ticker library related variables, constants and objects 
bool loopFlag;
Ticker schedule;

//Others
unsigned long previousMillis = 0;
long int unix_time;

//AWS IOT config, change these:
char aws_endpoint[]    = "adbf07ovrzhft-ats.iot.us-east-1.amazonaws.com";
char aws_key[]         = "AKIAJPBHY7UF7CAKKJWA";
char aws_secret[]      = "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";
char aws_region[]      = "us-east-1";
const char* thingShadowUpdate  = "$aws/things/esp8266/shadow/update";
int port = 443;

//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;
const char* thingConnectionStatus = "things/esp8266/connected";
const char* thingWillTopic = "things/esp8266/connected";
int thingWillQoS = 0;
boolean thingWillRetain = false;
const char* thingWillMessage = "{\"connected\": 0}";


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
    INT_CHK: bool int_chk = Ping.ping(dnsServer);
    if (!int_chk) {
        Serial.println("No Internet Connection, check router connectivity");
        delay(10000);
        goto INT_CHK; 
    } 
    else {
        Serial.println("There is Internet Connection");
        Serial.println(int_chk);
    }
    
    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    getESP8266_variable_info();
    heap_free_size = ESP.getFreeHeap();
    power_supply_v = ESP.getVcc();
    if (connect()) {
        sendMessage(1);
    }

    schedule.attach(5, loopFlagFunction); // Activate "loop()" functionality every 5 seconds
    schedule.attach(10, sendKeepAlive); // Sends KeepAlive to AWS IoT every 10 seconds
    schedule.attach(20, thingDisconnect);   
}


void loop() {
  
    while(loopFlag) {
        Serial.println("Sending PING packet to Google´s DNS");
        bool con_test = Ping.ping(dnsServer);
        if (con_test) {
            Serial.println("Connection success!");
        }
        else {
            Serial.println("NO CONNECTION!!!");
        }
        sendMessage(2);
        loopFlag = false; //This line finishes the while loop for when Ticker activates communication, the communication can be successfully implemented
    }
    
}

//connects to websocket layer and mqtt layer
bool connect() {
    if (client.connected()) {    
        client.disconnect();
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
    if (client.connect(clientID, thingWillTopic, thingWillQoS, thingWillRetain, thingWillMessage)) {
      Serial.println("Thing connected to AWS IoT Broker");     
      return true;
    } else {
      Serial.print("Failed to connect to AWS IoT Broker, rc=");
      Serial.print(client.state());
      return false;
    }    
}


void sendMessage(int code) {  
    if (code == 1) {
        char buf[100];
        String json = String("{\"state\":{\"reported\":{\"setup\": 1, \"connected\": 1}}}");
        Serial.println(json);
        char __json[json.length()+1];
        json.toCharArray(__json, sizeof(__json));
        Serial.println(__json);
        int rc = client.publish(thingShadowUpdate, __json);
    }
    if (code == 2) {
        char buf[100];
        String json = String("{\"state\":{\"reported\":{\"on\": true, \"power_supply\": ") + ESP.getVcc() + String(", \"heap_free_size\": ") + ESP.getFreeHeap() + String("}}}");
        Serial.println(json);
        char __json[json.length()+1];
        json.toCharArray(__json, sizeof(__json));
        Serial.println(__json);
        int rc = client.publish(thingShadowUpdate, __json);
    }
}


void sendKeepAlive() {  
    char buf[100]; 
    String json = String("{\"connected\": 1}");
    Serial.println(json);
    char __json[json.length()+1];
    json.toCharArray(__json, sizeof(__json));
    Serial.println(__json);
    int rc = client.publish(thingConnectionStatus, __json);
}


void thingDisconnect() {  
    client.disconnect();   
}


void loopFlagFunction(){
    loopFlag = true;
}


void getESP8266_variable_info() {
    int heap_free_size = ESP.getFreeHeap();
    Serial.print("Free Heap: "); Serial.println(heap_free_size);
    long cpu_inst_cycle = ESP.getCycleCount();
    Serial.print("CPU Instruction cycle count: "); Serial.println(cpu_inst_cycle);
    long power_supply_v = ESP.getVcc();
    Serial.print("Power Supply measure: "); Serial.println(power_supply_v);
}
