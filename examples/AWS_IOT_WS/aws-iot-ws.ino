#include <ESP8266WiFi.h> //library used to handle WiFi in ESP8266using Arduino Core.
#include <WiFiUdp.h> //general library used to handle UDP send and receive. In this case is going to be used to handle DNS and NTP communications.
#include <Ticker.h>// This library allows the creation of an object that will call a given function after a certain period.
#include <ESP8266Ping.h>
#include <ArduinoJson.h>

// Necessary libraries to connect the ESP8266 module to AWS IoTplatfor using AWS Signature v4 and WS.
#include <WebSocketsClient.h>
#include "AWSWebSocketClient.h"
#include <MQTTClient.h>
#include <PubSubClient.h>

//Control Variables
int FS_COUNTER;

//defined ssid name and password used to connect to WiFi
const char* ssid = "*****";
const char* password = "*****";

//WiFiUdp objects need a port to listen, a buffer for incoming packets and a reply message.
WiFiUDP udp; //here an instance of the WiFiUdp object was created.
unsigned int localUdpPort = 4210;
unsigned int rPort;
int RemotePort; //Defined to be a Global Variable due the unstable values that this variable can get when receiving packets.

//Here are defined instances of the class IPAddress to use for functionality and testing.
IPAddress dnsServer(8, 8, 8, 8);

//Here are defined ESP8266 native SDK variables and functions.
ADC_MODE (ADC_VCC); //Necessary requirement to use ESP.getVcc(), allows to reconfigure the ADC to measure from VCC pin.

//Ticker library related variables, constants and objects.
bool loopFlag = true;
Ticker schedule;

//Others
unsigned long previousMillis = 0;
long int unix_time;

//AWS IOT config golbal variables.
char aws_endpoint[]    = "*****.iot.us-east-1.amazonaws.com";
char aws_key[]         = "*****";
char aws_secret[]      = "*****";
char aws_region[]      = "us-east-1";
const char* thingShadowUpdate  = "$aws/things/esp8266/shadow/update";
int port = 443;

//MQTT config global variables.
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;
const char* thingConnectionStatus = "things/esp8266/connected";
const char* thingWillTopic = "things/esp8266/connected";
int thingWillQoS = 0;
boolean thingWillRetain = false;
const char* thingWillMessage = "{\"connected\": 0}";


AWSWebSocketClient awsWSclient(1000);
PubSubClient client(awsWSclient);

//# of connections.
long connection = 0;

//ESP8266 Status Check function realted variables, constants and objects.
int heap_free_size;
long cpu_inst_cycle;
long power_supply_v;


void setup() {
    Serial.begin(115200); //Initiate Serial Cx.
    WiFi.begin(ssid, password); //Set ESP8266 in STA and pass credentials to WiFi AP.
  
    while (WiFi.status() != WL_CONNECTED) //Waiting for ESP8266 to be connected.
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Wi-Fi Connected");
    Serial.println(WiFi.localIP()); //Print the IP obtained by the router.
    udp.begin(localUdpPort); //start listening to UDP packets on port 4210.

    //Check Internet connectivity Algorithm, just wanted to implement "goto" functionality.
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
    
    //Fill AWS account's parameters.    
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

    schedule.attach(5, loopFlagFunction); // Activate "loop()" functionality every 5 seconds.
    schedule.attach(10, sendKeepAlive); // Sends KeepAlive to AWS IoT every 10 seconds.   
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
        loopFlag = false; //This line finishes the while loop for when Ticker activates communication, the communication can be successfully implemented.
        //thingDisconnect();
    }
    
}

//Connects to WebSocket layer and MQTT layer.
bool connect() { 
    if (client.connected()) {    
        client.disconnect();
    }  
    //Delay is not necessary... it just help us to get a "trustful" heap space value.
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");

    //Creating clientID.
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

// Allows to send messages based on the argument passed. 1 - Used in "setup()" code to verify that everything went fine. 2 - Allows to publish ESP8266 specific data.
void sendMessage(int code) { 
    if (client.connected()) {
        Serial.println(client.state());
        if (code == 1) {
            client.connected();
            client.state();
            String json = String("{\"state\":{\"reported\":{\"setup\": 1, \"connected\": 1}}}");
            Serial.println(json);
            char __json[json.length()+1];
            json.toCharArray(__json, sizeof(__json));
            Serial.println(__json);
            int rc = client.publish(thingShadowUpdate, __json);
        }
        if (code == 2) {
            client.connected();
            client.state();
            String json = String("{\"state\":{\"reported\":{\"on\": true, \"power_supply\": ") + ESP.getVcc() + String(", \"heap_free_size\": ") + ESP.getFreeHeap() + String("}}}");
            Serial.println(json);
            char __json[json.length()+1];
            json.toCharArray(__json, sizeof(__json));
            Serial.println(__json);
            int rc = client.publish(thingShadowUpdate, __json);
        }
    }
}

// Allows sending a "publish" keepalive message to an specific MQTT Topic, not to the Thing Device Shadow.  
void sendKeepAlive() { 
    String json = String("{\"connected\": 1}");
    Serial.println(json);
    char __json[json.length()+1];
    json.toCharArray(__json, sizeof(__json));
    Serial.println(__json);
    int rc = client.publish(thingConnectionStatus, __json);
}

// Gracefully disconnect from AWS IoT broker triggering Last Will Testament message set at connection time.
void thingDisconnect() { 
    Serial.println("Disconnecting..."); 
    client.disconnect();  
}

// Allows to use it as a control flag to trigger events, in this case, an specific section of code in "loop()" area.
void loopFlagFunction() { 
    loopFlag = true;
}

// Allows to get information about the ESP8266 module using its specific NONOS SDKs adapted to Arduino core.
void getESP8266_variable_info() {
    int heap_free_size = ESP.getFreeHeap(); // Use ESP8266 NONOS SDK to get Free Heap size.
    Serial.print("Free Heap: "); Serial.println(heap_free_size);
    long cpu_inst_cycle = ESP.getCycleCount(); // Use ESP8266 NONOS SDK to get the CPU Cycle Count.
    Serial.print("CPU Instruction cycle count: "); Serial.println(cpu_inst_cycle);
    long power_supply_v = ESP.getVcc(); // Use ESP8266 NONOS SDK to get the Power Supply voltage measured using the internal ADC that was configured at the beginning of the code.
    Serial.print("Power Supply measure: "); Serial.println(power_supply_v);
}
