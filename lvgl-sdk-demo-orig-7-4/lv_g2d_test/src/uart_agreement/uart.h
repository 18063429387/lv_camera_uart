#ifndef __UART_H
#define __UART_H

//#define CMD_LEN   32
#ifndef WIN32
#include <linux/input.h>
#endif

class Uart
{
public:
	Uart(char* device, int sp = 115200, char datab = 8, char stopb = 1, char par = 'n');
	int Open();
	int Open(int nSpeed, char dataBit = 8, char stopBit = 1, char par = 'n');
	void Close();
	int Read(unsigned char* pucBuf, int iLen, int iTimeOutMs = 1000);
	int Write(unsigned char* data, int size);
	int IsOpen(void);
	void SetBaudrate(int baudrate);
	void ReconfigureUart();
	char* DeviceName();
private:
	char dev[32];
	int  fd;
	int  speed;
	char dataBits;
	char stopBits;
	char parity;
	char isBlock;
};

class IrRx
{
public:
	IrRx(char* device);
	~IrRx();
	void Close();
	int Open();
	int Open(const char* device);
	int Read(unsigned char* pucBuf, int iLen, int iTimeOutMs = 1000);
private:
	char dev[32];
	int  fd;
};


#endif // __UART_H
