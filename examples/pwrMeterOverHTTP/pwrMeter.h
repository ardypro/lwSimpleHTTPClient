#ifndef PWRMETER_H
#define PWRMETER_H
//如果跟485通信的是软串口，就需要屏蔽下面这句
#define HARDSERIAL

#ifdef HARDSERIAL
#include "HardwareSerial.h"
#else
#include "SoftwareSerial.h"
#endif
#include "Arduino.h"

/*
  ModbusReaderMethodTwo - example from LT/JSY C code

 ModbusReaderMethodTwo is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ModbusReaderMethodTwo is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ModbusReaderMethodTwo.  If not, see <http://www.gnu.org/licenses/>.

 Edited by Lozoo 932871121<linzaiz@qq.com> on 2013.11.22
 Copyright © 2009-2013 Lozoo 932871121<linzaiz@qq.com>
 */

/*
    Modified by Jack Zhong (jzopen#yeah.net)

 HISTORY:
 2013-12-15  将CRC计算部分的函数改为inline，加快运行速度
 2013-12-04  支持多表，默认的表ID=1，这也是电量表默认的ID
 增加电量模块管理功能：有效总电能清零、修改slave模块ID
 2013-12-03  支持软串口


 Instruction:
 ------------

 pwrMeter meter;

 meter.begin(&Serial,4800);      //设置与485通信的串口以及波特率

 if (meter.available())         //检查是否有数据进入缓冲区
 {
 if (meter.readData(功率，电流，电量，功率因数，电压))
 {
 Serial.print(电压);
 ...
 }
 delay(2000);              //延时，等待下一个操作
 }
 */


/* FIXME: not yet being used */
unsigned long interframe_delay = 2;  /* Modbus t3.5 = 2 ms */

/*
* preset_multiple_registers: Modbus function 16. Write the data from an
 * array into the holding registers of a slave.
 * INPUTS
 * slave: modbus slave id number
 * start_addr: address of the slave's first register (+1)
 * reg_count: number of consecutive registers to preset
 * data: array of words (ints) with the data to write into the slave
 * RETURNS: the number of bytes received as response on success, or
 *         0 if no bytes received (i.e. response timeout)
 *        -1 to -4 (modbus exception code)
 *        -5 for other errors (port error, etc.).
 */

int preset_multiple_registers(int slave, int start_addr,
                              int reg_count, int *data);

/*
* read_holding_registers: Modbus function 3. Read the holding registers
 * in a slave and put the data into an array
 * INPUTS
 * slave: modbus slave id number
 * start_addr: address of the slave's first register (+1)
 * count: number of consecutive registers to read
 * dest: array of words (ints) on which the read data is to be stored
 * dest_size: size of the array, which should be at least 'count'
 * RETURNS: the number of bytes received as response on success, or
 *         0 if no valid response received (i.e. response timeout, bad crc)
 *        -1 to -4 (modbus exception code)
 *        -5 for other errors (port error, etc.).
 */

int read_holding_registers(int slave, int start_addr, int count,
                           int *dest, int dest_size);


inline unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
{
    unsigned char i;
    unsigned char chk;
    crc = crc ^ crcbuf;
    for (i = 0; i < 8; i++)
    {
        chk = (unsigned char)(crc & 1);
        crc = crc >> 1;
        crc = crc & 0x7fff;
        if (chk == 1)
        {
            crc = crc ^ 0xa001;
        }
        crc = crc & 0xffff;
    }
    return crc;
}

inline unsigned int chkcrc(unsigned char *buf, unsigned char len)
{
    unsigned char hi, lo;
    unsigned int i;
    unsigned int crc;
    crc = 0xFFFF;
    for (i = 0; i < len; i++)
    {
        crc = calccrc(*buf, crc);
        buf++;
    }
    hi = (unsigned char)(crc % 256);
    lo = (unsigned char)(crc / 256);
    crc = (((unsigned int)(hi)) << 8) | lo;
    return crc;
}



#define TX_BUFFER_SIZE 8
// #define TIMEOUT 10000          /* 10 second */
// #define MAX_RESPONSE_LENGTH 256
// #define PORT_ERROR 0   //原来为-5

//#define TIMEOUT 1000          /* 1 second */
#define TIMEOUT 10000          /* 10 second */
#define MAX_READ_REGS 125
#define MAX_WRITE_REGS 125
#define MAX_RESPONSE_LENGTH 256
#define PRESET_QUERY_SIZE 256
/* errors */
#define PORT_ERROR -5

class pwrMeter
{
public:
    pwrMeter();
    virtual ~pwrMeter();
#ifdef HARDSERIAL
    void begin(HardwareSerial *serial, int baud = 4800);
#else
    void begin(SoftwareSerial *serial, int baud = 4800);
#endif
    byte available(byte SlaveID = 1);
    bool readData(int &watt, float &amp, float &kwh, float &pf, float &voltage);

protected:
    int send_query(unsigned char *query, size_t string_length);
    byte receive_response(unsigned char *received_string);
    //unsigned int calccrc(unsigned char crcbuf,unsigned int crc);
    //unsigned int chkcrc(unsigned char *buf,unsigned char len);
    //int Analysis_data(void);

    void modbus_query(unsigned char *packet, size_t string_length);
    void build_request_packet(int slave, int function, int start_addr,
                          int count, unsigned char *packet);
    unsigned int crc(unsigned char *buf, int start, int cnt);

    byte read_data(void);
    byte Read_ID;
    // union crcdata
    // {
    //     unsigned int word16;
    //     unsigned char byte[2];
    // };

    // struct commdata
    // {
    //     unsigned int Status;
    //     unsigned int nRx;
    // }
    // Comm1;
private:
#ifdef HARDSERIAL
    HardwareSerial *UART;
#else
    SoftwareSerial *UART;
#endif
    byte bytesReceived;
    unsigned char Tx_Buffer[TX_BUFFER_SIZE];
    unsigned char RX_Buffer[MAX_RESPONSE_LENGTH]; //需要优化

    // unsigned int     Voltage_data;
    // unsigned int     Current_data;
    // unsigned int     Power_data ;
    // unsigned long    Energy_data;
    // unsigned int     Pf_data;
};

#endif // PWRMETER_H

