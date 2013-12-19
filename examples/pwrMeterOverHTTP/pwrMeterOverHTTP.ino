#include <DHT11.h>

#include "Ethernet.h"
#include "SPI.h"
#include "Arduino.h"

#include "pwrMeter.h"
//#include "lwSimpleHTTPClient.h"  //upload data one by one
//#include "SoftwareSerial.h"      //communicate with 485 with software serial
#include "lwPowermeterOverHTTP.h"  //upload batch data

#define userkey "029b3884b91e4d00b514158ba1e2ac57"
#define gateway "01"

//SoftwareSerial mySerial(2, 3);
pwrMeter meter;
lwPowermeterOverHTTP *hclient;
DHT11 dht(3);


void setup()
{
  Serial.begin(4800);
  //meter.begin(&mySerial, 4800);
  meter.begin(&Serial);
  hclient = new lwPowermeterOverHTTP(userkey, gateway);
  if (  hclient->initialize())
    Serial.println("DHCP OK");
  else
    Serial.println("DHCP FAIL");
}

void loop()
{
  float t;
  float h;
  int err;
  if((err=dht.read(h,t))==0)
  {



    //hclient->post("WD",t);
    //hclient->post("SD",h);
  }

  if (meter.available(2) > 0)  // 读模块2的缓冲区
  {
    int Watt = 0;
    float Amp = 0;
    float Kwh = 0;
    float Pf = 0;
    float Voltage = 0;

    Serial.println("reading power data...");

    if (meter.readData(Watt, Amp, Kwh, Pf, Voltage)) // 一次性读入所有的电量数据
    {
      hclient->postBatchPowerInfo(Watt,Amp,Kwh, Pf, Voltage, t, h);

      Serial.println();
      Serial.print("humidity:\t");
      Serial.println(h);
      Serial.println();

      Serial.println();
      Serial.print("voltage:\t");
      Serial.println(Voltage);
      Serial.println();

    }
    else
      Serial.println("reading data failed");
  }
  else
    Serial.println("no data in rx buffer or read data failed");

  delay(12000);  
}















