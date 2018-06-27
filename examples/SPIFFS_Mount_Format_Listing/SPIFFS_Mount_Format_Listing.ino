#include <ESP8266WiFi.h> //library used to handle WiFi in ESP8266using Arduino Core.
#include "FS.h" //library used to handle the ESP8266´s File System.
#include <WiFiUdp.h> //general library used to handle UDP send and receive. In this case is going to be used to handle DNS and NTP communications.
#include <Ticker.h>

//Control Variables
int FS_COUNTER;




//defined ssid name and password used to connect to Wi-Fi.
const char* ssid = "theForceIsStrongInThisOne";
const char* password = "bR0k3n.#!N0m4d$_";



//WiFiUdp objects need a port to listen, a buffer for incoming packets and a reply message.
WiFiUDP udp; //here an instance of the WiFiUdp object was created.
unsigned int localUdpPort = 4210;
unsigned int rPort;
int RemotePort; //Defined to be a Global Variable due the unstable values that this variable can get when receiving packets.


//Here are defined the Buffer Size for every packet used in Application Layer of TCP/IP.
const int DNS_PACKET_SIZE = 31;


int packetSize = 0; //packetSize variable initializated to use readDNSpacket as a function.
byte packetBuffer[255]; //Buffer the generic buffer for receiving packets.


//Here are defined instances of the class IPAddress to use for functionality and testing.
IPAddress dnsserver(8, 8, 8, 8);


//Here are defined ESP8266 native SDK variables and functions.
ADC_MODE (ADC_VCC); //Necessary requirement to use ESP.getVcc(), allows to reconfigure the ADC to measure from VCC pin.

//Others
long int unix_time;
Ticker Networking; //Allows event scheduling.
bool FIRST_FLAG = true; //Allows the DNS query in loop to happen the first time that loop() runs.

void setup() {
 
  Serial.begin(115200); //Initiate Serial Cx
  WiFi.begin(ssid, password); //Set ESP8266 in STA and pass credentials to WiFi AP.
  
  while (WiFi.status() != WL_CONNECTED) //Waiting for ESP8266 to be connected.
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi Connected");
  Serial.println(WiFi.localIP()); //Print the IP obtained by the router.
  udp.begin(localUdpPort); //start listening to UDP packets on port 4210.

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

  getESP8266_fixed_info(); //Shows information about Sketch Size, Flash Size using ESP-spcefic SDK
  
  //Mounting File System, if File System mounting process fails, it will try it again.
  up_FS: SPIFFS.begin();
  if (!SPIFFS.begin()) {
    Serial.println("Unable to mount File System");
    delay (500);
    FS_COUNTER++;
    if (FS_COUNTER>=3) {  //Control Structure using "goto", "if" and "Control Variable"
      Serial.println("FATAL ERROR MUNTING FILE SYSTEM! SYSTEM WILL RESTART");
      Serial.println(ESP.getResetReason());
      ESP.restart();  //Restart the system if there was a failure mountng the FS.
    }
    goto up_FS;
  }
  else {
    Serial.println("File System mounted");
  }

  //The logic below allows to format the SPIFFS. 
  //I recommend to comment the lines of code below once the format has been done and re-upload the firmware; 
  //or just create a new firmware just to format the SPIFFS.
  SPIFFS.format();
  while (!SPIFFS.format()) {
   
  }
  Serial.println("SPIFFS Formatted");

  //The logic below allows to list all the files in the SPIFFS
  Dir dir = SPIFFS.openDir("/");  //Opens a directory given its absolute path. Returns "Dir".
  while (dir.next()) {         //Returns true if there are files to iterate.
    Serial.print(dir.fileName());
    File f = dir.openFile("r");
    Serial.println(f.size());
    f.close();
  }
  
  Networking.attach(60, udp_tcp_Networking_flag); //Activate UDP + TCP functionality every 10 seconds.
}

   

void loop() {
  
  while(FIRST_FLAG) {
    Serial.println("Sending DNS packet to Google´s DNS");
    sendDNS_NTPpacket (dnsserver);
    IPAddress response = readDNSpacket();
    Serial.print("loop() ---  `response´ variable is: "); Serial.println(response);
    FIRST_FLAG = false; //This line finishes the while loop for when Ticker activates communication, the communication can be successfully implemented.
  }
}

//Here will be defined all the flags (interrumptions) for "Ticker" functionality and LESS CODE AS POSSIBLE IN INTERRUPTS!!!
void udp_tcp_Networking_flag(){
  FIRST_FLAG = true;
}

//Here will be defined all the UDP traffic functions (DNS, NTP and receivers).
unsigned long sendDNS_NTPpacket(IPAddress& address) {
  // set all bytes in the buffer to 0.
  memset(packetBuffer, 0, 255);
  // Initialize values needed to form DNS request.
  packetBuffer[0] = 0x00;   // ID
  packetBuffer[1] = 0x01;
  packetBuffer[2] = 0x01;   // Flags --> Recursive Desired=1.
  packetBuffer[3] = 0x00;
  packetBuffer[4] = 0x00;   //Number of Questions, in this case Questions=1.
  packetBuffer[5] = 0x01;
  // 6 bytes of zero for Answer RRs, Authority RRs and Aditonal RRs.
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
  //Here the name of the domain that the ESP8266 is querying ends.
  packetBuffer[27]  = 0x00;   //Type, in this case Type=A.
  packetBuffer[28]  = 0x01;
  packetBuffer[29]  = 0x00;   //Class, in this case Class = IN.
  packetBuffer[30]  = 0x01;

  //Serial.println("3");

  // all DNS fields have been given values, now
  // you can send a packet requesting a the IP Adresses of a domain:
  udp.beginPacket(address, 53); //DNS requests are to port 53.
  //Serial.println("4");
  udp.write(packetBuffer, DNS_PACKET_SIZE);
  //Serial.println("5");
  udp.endPacket();
  Serial.println("DNS Packet sent");
  //Serial.println(packetBuffer);
  //Serial.println("6");
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

void getESP8266_fixed_info() {
  int sketch_size = ESP.getSketchSize();
  int free_sketch_size = ESP.getFreeSketchSpace();
  int flash_size = ESP.getFlashChipSize();
  int real_flash_size = ESP.getFlashChipRealSize();
  Serial.print("Sketch Size: "); Serial.print(sketch_size); Serial.print("and is left free: "); Serial.println(free_sketch_size);
  Serial.print("Flash Chip Size: "); Serial.println(flash_size); 
  Serial.print("Real Flash Chip Size: "); Serial.println(real_flash_size);
}  


