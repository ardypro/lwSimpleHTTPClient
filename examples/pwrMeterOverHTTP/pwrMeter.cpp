#include "pwrMeter.h"


pwrMeter::pwrMeter()
{
  //ctor
}

pwrMeter::~pwrMeter()
{
  //dtor
}

// byte pwrMeter::send_query(unsigned char *query, size_t string_length)
// {
//   byte i;
//   for (i = 0; i < string_length; i++)
//   {
//     UART->write(query[i]); //JingLi
//   }
//    without the following delay, the reading of the response might be wrong
//    * apparently, 
//   delay(200);            /* FIXME: value to use? */

//   return i;           /* it does not mean that the write was succesful, though */
// }//Commented out by Jack Zhong @ 2013-12-17



/***********************************************************************
 *
 * send_query(query_string, query_length )
 *
 * Function to send a query out to a modbus slave.
 ************************************************************************/
int pwrMeter::send_query(unsigned char *query, size_t string_length)
//int send_query(unsigned char *query, size_t string_length)
{

    int i;

    modbus_query(query, string_length);
    string_length += 2;

    for (i = 0; i < string_length; i++)
    {
        //                Serial.print(query[i], HEX); //Orginal
        Serial.write(query[i]); //JingLi

    }
    /* without the following delay, the reading of the response might be wrong
     * apparently, */
    delay(200);            /* FIXME: value to use? */

    return i;           /* it does not mean that the write was succesful, though */
} //Replaced by Jack Zhong @ 2013-12-17

byte pwrMeter::receive_response(unsigned char *received_string)
//byte receive_response(unsigned char *received_string)
{
  bytesReceived = 0;
  byte i = 0;

  /* wait for a response; this will block! */
  while (UART->available() == 0)
  {
    delay(1);
    if (i++ > TIMEOUT)
    {
      UART->println("receive timeout.");
#ifdef DEBUGGING
      Serial.println("receive timeout.");
#endif
    }
    return bytesReceived;
  }
  delay(200);
  /* FIXME: does UART->available wait 1.5T or 3.5T before exiting the loop? */
  while (UART->available())
  {
    received_string[bytesReceived] = UART->read();  //ORIGINAL.
    bytesReceived++;
    if (bytesReceived >= MAX_RESPONSE_LENGTH)
    {
      return PORT_ERROR;
    }
  }


  return (bytesReceived);
}


/*********************************************************************
 *
 *      modbus_response( response_data_array, query_array )
 *
 * Function to the correct response is returned and that the checksum
 * is correct.
 *
 * Returns:     string_length if OK
 *           0 if failed
 *           Less than 0 for exception errors
 *
 *      Note: All functions used for sending or receiving data via
 *            modbus return these return values.
 *
 **********************************************************************/

int modbus_response(unsigned char *data, unsigned char *query)
{
    int response_length;
    int i;
    unsigned int crc_calc = 0;
    unsigned int crc_received = 0;
    unsigned char recv_crc_hi;
    unsigned char recv_crc_lo;

    do          // repeat if unexpected slave replied
    {
        response_length = receive_response(data);
    }
    while ((response_length > 0) && (data[0] != query[0]));
    //      for (i = 0; i <response_length; i++) {           Serial.print(data[i]);Serial.print("---");   Serial.println(query[i]);}                       //only test

    if (response_length)
    {


        crc_calc = crc(data, 0, response_length - 2);

        recv_crc_hi = (unsigned) data[response_length - 2];
        recv_crc_lo = (unsigned) data[response_length - 1];

        crc_received = data[response_length - 2];
        crc_received = (unsigned) crc_received << 8;
        crc_received =
            crc_received | (unsigned) data[response_length - 1];


        /*********** check CRC of response ************/

        if (crc_calc != crc_received)
        {
            response_length = 0;
            //                       Serial.println("CRC erro");                       //only test
        }



        /********** check for exception response *****/

        if (response_length && data[1] != query[1])
        {
            response_length = 0 - data[2];
        }
    }
    return (response_length);
} //Add by Jack Zhong @ 2013-12-17



/************************************************************************
 *
 *      read_reg_response
 *
 *      reads the response data from a slave and puts the data into an
 *      array.
 *
 ************************************************************************/

int read_reg_response(int *dest, int dest_size, unsigned char *query)
{

    unsigned char data[MAX_RESPONSE_LENGTH];
    int raw_response_length;
    int temp, i;

    raw_response_length = modbus_response(data, query);
    if (raw_response_length > 0)
        raw_response_length -= 2;

    if (raw_response_length > 0)
    {
        /* FIXME: data[2] * 2 ???!!! data[2] isn't already the byte count (number of registers * 2)?! */
        for (i = 0;
                i < (data[2] * 2) && i < (raw_response_length / 2);
                i++)
        {

            /* shift reg hi_byte to temp */
            temp = data[3 + i * 2] << 8;
            /* OR with lo_byte           */
            temp = temp | data[4 + i * 2];

            dest[i] = temp;
        }
    }
    return (raw_response_length);
}//Add by Jack Zhong @ 2013-12-17


/***********************************************************************
 *
 *      preset_response
 *
 *      Gets the raw data from the input stream.
 *
 ***********************************************************************/

int preset_response(unsigned char *query)
{
    unsigned char data[MAX_RESPONSE_LENGTH];
    int raw_response_length;

    raw_response_length = modbus_response(data, query);

    return (raw_response_length);
}//Add by Jack Zhong @ 2013-12-17


/************************************************************************
 *
 *      read_holding_registers
 *
 *      Read the holding registers in a slave and put the data into
 *      an array.
 *
 *************************************************************************/

int read_holding_registers(int slave, int start_addr, int count,
                           int *dest, int dest_size)
{
    int function = 0x03;      /* Function: Read Holding Registers */
    int ret;

    unsigned char packet[REQUEST_QUERY_SIZE + CHECKSUM_SIZE];

    if (count > MAX_READ_REGS)
    {
        count = MAX_READ_REGS;
    }

    build_request_packet(slave, function, start_addr, count, packet);

    if (send_query(packet, REQUEST_QUERY_SIZE) > -1)
    {
        ret = read_reg_response(dest, dest_size, packet);
    }
    else
    {

        ret = -1;
    }

    return (ret);
}//Add by Jack Zhong @ 2013-12-17


/************************************************************************
 *
 *      preset_multiple_registers
 *
 *      Write the data from an array into the holding registers of a
 *      slave.
 *
 *************************************************************************/

int preset_multiple_registers(int slave, int start_addr,
                              int reg_count, int *data)
{
    int function = 0x10;      /* Function 16: Write Multiple Registers */
    int byte_count, i, packet_size = 6;
    int ret;

    unsigned char packet[PRESET_QUERY_SIZE];

    if (reg_count > MAX_WRITE_REGS)
    {
        reg_count = MAX_WRITE_REGS;
    }

    build_request_packet(slave, function, start_addr, reg_count, packet);
    byte_count = reg_count * 2;
    packet[6] = (unsigned char)byte_count;

    for (i = 0; i < reg_count; i++)
    {
        packet_size++;
        packet[packet_size] = data[i] >> 8;
        packet_size++;
        packet[packet_size] = data[i] & 0x00FF;
    }

    packet_size++;
    if (send_query(packet, packet_size) > -1)
    {
        ret = preset_response(packet);
    }
    else
    {
        ret = -1;
    }

    return (ret);
}//Add by Jack Zhong @ 2013-12-17



// int pwrMeter::Analysis_data(void) //已经集成到readData()方法中，因此被注释掉
// {
//     int ret;
//     unsigned char i;
//     crcdata crcnow;
//     ret = -1;
//     //if(Comm1.Status==2)
//     {
//         if (RX_Buffer[0] == Read_ID)
//         {
//             crcnow.word16 = chkcrc(RX_Buffer, bytesReceived - 2); //bytesReceived
//             if ((crcnow.byte[0] == RX_Buffer[bytesReceived - 1]) && (crcnow.byte[1] == RX_Buffer[bytesReceived - 2])) //CRC
//             {
//                 Voltage_data = (((unsigned int)(RX_Buffer[3])) << 8) | RX_Buffer[4]; //Voltage_data
//                 Current_data = (((unsigned int)(RX_Buffer[5])) << 8) | RX_Buffer[6]; //Current_data
//                 Power_data = (((unsigned int)(RX_Buffer[7])) << 8) | RX_Buffer[8]; //Power_data
//                 Energy_data = (((unsigned long)(RX_Buffer[9])) << 24) | (((unsigned long)(RX_Buffer[10])) << 16) | (((unsigned long)(RX_Buffer[11])) << 8) | RX_Buffer[12]; ////Energy_data
//                 Pf_data = (((unsigned int)(RX_Buffer[13])) << 8) | RX_Buffer[14]; //Pf_data
//                 ret = 1;
//             }
//         }
//         //Comm1.Status=0;
//     }
//     return ret;
// }

byte pwrMeter::read_data(void)
{
  byte ret;
  union crcdata
  {
    unsigned int word16;
    unsigned char byte[2];
  };
  crcdata crcnow;

  delay(1800);
  Tx_Buffer[0] = Read_ID;
  Tx_Buffer[1] = 0x03;
  Tx_Buffer[2] = 0x00;
  Tx_Buffer[3] = 0x48;
  Tx_Buffer[4] = 0x00;
  Tx_Buffer[5] = 0x06;
  crcnow.word16 = chkcrc(Tx_Buffer, 6);
  Tx_Buffer[6] = crcnow.byte[1]; //CRC
  Tx_Buffer[7] = crcnow.byte[0];

  ret = send_query(Tx_Buffer, TX_BUFFER_SIZE);
#ifdef DEBUGGING
  Serial.println("has incoming data");
#endif
  return ret;
}

byte pwrMeter::available(byte SlaveID)
{
  Read_ID = SlaveID;
  //2013-12-15  因为查询数据的命令是固定的，因而对于常用的slaveID对应的TXBUFFER，直接给予CRC，加快运行速度
  //query interval, it is 200 originally
#define QUERYINTERVAL 500

  delay(QUERYINTERVAL); //Delay for sometime to get ready for reading data in the power module. Add by Jack Zhong
  
  switch (SlaveID)
  {
  case 1:
    UART->write(0x01);
    UART->write(0x03);
    UART->write(0x00);
    UART->write(0x48);
    UART->write(0x00);
    UART->write(0x06);
    UART->write(0x45);
    UART->write(0xDE);
    delay(QUERYINTERVAL);
    break;
  case 2:
    UART->write(0x02);
    UART->write(0x03);
    UART->write(0x00);
    UART->write(0x48);
    UART->write(0x00);
    UART->write(0x06);
    UART->write(0x45);
    UART->write(0xED);
    delay(QUERYINTERVAL);
    break;
  default:
    read_data();  //其余ID需要计算CRC，因此要调用read_data()和send_query()方法
  }

  return  receive_response(RX_Buffer);

}

bool pwrMeter::readData(int &watt, float &amp, float &kwh, float &pf, float &voltage)
{
  //response_length = receive_response(RX_Buffer); //移至available()
  union crcdata
  {
    unsigned int word16;
    unsigned char byte[2];
  };

  if (bytesReceived > 0)
  {
    if (RX_Buffer[0] == Read_ID)
    {
      crcdata crcnow;
      crcnow.word16 = chkcrc(RX_Buffer, bytesReceived - 2); //bytesReceived
      if ((crcnow.byte[0] == RX_Buffer[bytesReceived - 1]) && (crcnow.byte[1] == RX_Buffer[bytesReceived - 2])) //CRC
      {
        voltage = (float) ((((unsigned int)(RX_Buffer[3])) << 8) | RX_Buffer[4]) / 100; //Voltage_data
        amp = (float)((((unsigned int)(RX_Buffer[5])) << 8) | RX_Buffer[6]) / 1000; //Current_data
        watt = (((unsigned int)(RX_Buffer[7])) << 8) | RX_Buffer[8]; //Power_data
        kwh = (float) ((((unsigned long)(RX_Buffer[9])) << 24) | (((unsigned long)(RX_Buffer[10])) << 16) | (((unsigned long)(RX_Buffer[11])) << 8) | RX_Buffer[12]) / 3200; ////Energy_data
        pf = (float) ((((unsigned int)(RX_Buffer[13])) << 8) | RX_Buffer[14]) / 1000; //Pf_data

        return true;
      }
    }
  }
  else
  {
    return false ;
  }
}

#ifdef HARDSERIAL
void pwrMeter::begin(HardwareSerial *serial, int baud)
{
  UART = serial;
  UART->begin(baud);
}
#else
void pwrMeter::begin(SoftwareSerial *serial, int baud)
{
  UART = serial;
  UART->begin(baud);
}
#endif



/*
CRC

 INPUTS:
 buf   ->  Array containing message to be sent to controller.
 start ->  Start of loop in crc counter, usually 0.
 cnt   ->  Amount of bytes in message being sent to controller/
 OUTPUTS:
 temp  ->  Returns crc byte for message.
 COMMENTS:
 This routine calculates the crc high and low byte of a message.
 Note that this crc is only used for Modbus, not Modbus+ etc.
 ****************************************************************************/

unsigned int pwrMeter::crc(unsigned char *buf, int start, int cnt)
{
    int i, j;
    unsigned temp, temp2, flag;

    temp = 0xFFFF;

    for (i = start; i < cnt; i++)
    {
        temp = temp ^ buf[i];

        for (j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp = temp >> 1;
            if (flag)
                temp = temp ^ 0xA001;
        }
    }

    /* Reverse byte order. */

    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;

    return (temp);
}//added by Jack Zhong @ 2013-12-17


/***********************************************************************
 *
 *      The following functions construct the required query into
 *      a modbus query packet.
 *
 ***********************************************************************/

#define REQUEST_QUERY_SIZE 6     /* the following packets require          */
#define CHECKSUM_SIZE 2          /* 6 unsigned chars for the packet plus   */
/* 2 for the checksum.                    */

void pwrMeter::build_request_packet(int slave, int function, int start_addr,
                          int count, unsigned char *packet)
{
    packet[0] = slave;
    packet[1] = function;
    start_addr -= 1;
    packet[2] = start_addr >> 8;
    packet[3] = start_addr & 0x00ff;
    packet[4] = count >> 8;
    packet[5] = count & 0x00ff;

    //below test only
    //        packet[0] =0x01;
    //        packet[1] = 0x03;
    //        packet[2] = 0;
    //        packet[3] = 0x48;
    //        packet[4] = 0;
    //        packet[5] = 0x02;
}//added by Jack Zhong @ 2013-12-17

/*************************************************************************
 *
 * modbus_query( packet, length)
 *
 * Function to add a checksum to the end of a packet.
 * Please note that the packet array must be at least 2 fields longer than
 * string_length.
 **************************************************************************/

void pwrMeter::modbus_query(unsigned char *packet, size_t string_length)
{
    int temp_crc;

    temp_crc = crc(packet, 0, string_length);

    packet[string_length++] = temp_crc >> 8;
    packet[string_length++] = temp_crc & 0x00FF;
    packet[string_length] = 0;
}//added by Jack Zhong @ 2013-12-17



