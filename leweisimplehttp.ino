#include <Arduino.h>
#include "lwSimpleHTTPClient.h"

#include "Ethernet.h"
#include "EthernetClient.h"
#include "SPI.h"


const char* uk  ="029b3884b91e4d00b514158ba1e2ac57";
const char* gw= "02";

lwSimpleHTTPClient* cl;

int i=-2;
float f=-2.38;
void setup()
{
    Serial.begin(4800);

    cl=new lwSimpleHTTPClient(uk,gw);
    if( cl->begin())
        Serial.println("DHCP OK");
    else
        Serial.println("DHCP Fail");

}

void loop()
{
   if( cl->append("test",f))
		Serial.println("append successfully");
	 else
		Serial.println("append failed");
    cl->send();
    f++;
    delay(12000);
}
