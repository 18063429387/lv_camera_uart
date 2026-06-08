#include <stdio.h>
#include <string.h>
#include "uart.h"

// #ifndef WIN32    //  only for linux
#if 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/serial.h>

Uart::Uart(char* device, int nSpeed, char dataBit, char stopBit, char par)
{
	strncpy(dev, device, strlen(device));
	dev[strlen(dev)] = 0;

	printf("Uart::Uart(%s, %d, %d, %d, %d)\r\n", device, nSpeed, dataBit, stopBit, par);
	speed = nSpeed;
	dataBits = dataBit;
	stopBits = stopBit;
	parity = par;

	fd = -1;
}

int Uart::Open()
{
	struct termios newtio, oldtio;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd < 0) {
		perror("Open");
		return -1;
	}

	if (tcgetattr(fd, &oldtio) != 0) {
		close(fd);
		fd = -1;
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	newtio.c_cflag |= (dataBits == 7 ? CS7 : CS8);

	switch (parity)
	{
	case 'o':
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		break;
	case 'e':
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'n':
		newtio.c_cflag &= ~PARENB;
		break;
	default:
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (speed)
	{
	case 1200:
		cfsetispeed(&newtio, B1200);
		cfsetospeed(&newtio, B1200);
		break;
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 19200:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;
	case 38400:
		cfsetispeed(&newtio, B38400);
		cfsetospeed(&newtio, B38400);
		break;
	case 57600:
		cfsetispeed(&newtio, B57600);
		cfsetospeed(&newtio, B57600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	default:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	}

	if (stopBits == 1)
		newtio.c_cflag &= ~CSTOPB;
	else
		newtio.c_cflag |= CSTOPB;

	printf("[%d - %s] uart open %s, speed = %d, dataBits = %d, stopBits = %d, parity = %c\r\n", 
			__LINE__, __FILE__, dev, speed, dataBits, stopBits, parity);

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd, TCIFLUSH);

	// if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
	// {
	// 	close(fd);
	// 	fd = -1;
	// 	perror("tcsetattr");
	// 	return -1;
	// }
	int ret = tcsetattr(fd, TCSANOW, &newtio);
	printf("[%d - %s] before tcsetattr: fd=%d, c_cflag=0x%08x, c_iflag=0x%08x, c_oflag=0x%08x, c_lflag=0x%08x\n",
		__LINE__, __FILE__, fd, (unsigned)newtio.c_cflag, (unsigned)newtio.c_iflag, (unsigned)newtio.c_oflag, (unsigned)newtio.c_lflag);
	if (ret != 0)
	{
		close(fd);
		fd = -1;
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

int Uart::Open(int nSpeed, char dataBit, char stopBit, char par)
{
	if (IsOpen()) {
		Close();
	}
	speed = nSpeed;
	dataBits = dataBit;
	stopBits = stopBit;
	parity = par;

	return Open();
}

void Uart::Close()
{
	if (fd >= 0) {
		close(fd);
	}
	fd = -1;
}

int Uart::Read(unsigned char* pucBuf, int iLen, int iTimeOutMs)
{
	if (IsOpen() <= 0) {
		printf("%s: Uart %s not open!\r\n", __FUNCTION__, &dev[0]);
		return -1;
	}

	int ret;
	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);

	struct timeval timeout;
	if (iTimeOutMs == -1) {
		timeout.tv_sec = 0x7fffffff;
		timeout.tv_usec = 0;
	}
	else {
		timeout.tv_sec = iTimeOutMs / 1000;
		timeout.tv_usec = (iTimeOutMs % 1000) * 1000;
	}

	ret = select(fd + 1, &readFds, NULL, NULL, &timeout);
	if (ret > 0) {
		ret = read(fd, pucBuf, iLen);
		if (ret < 0) {
			perror("read");
		}
		else if (ret > 0) {
			//printf("uart read data (ret = %d):\r\n", ret);
			//for (int i = 0; i < ret; i++) {
			//    printf("%02x, ", pucBuf[i]);
			//}
			//printf("\r\n\r\n");
		}
	}
	else if (ret == 0) {
		//printf("timeout\r\n");
		return 0;
	}
	else {
		perror("select");
		return ret;
	}

	return ret;
}

int Uart::Write(unsigned char* data, int size)
{
	if (IsOpen() <= 0) {
		printf("%s: Uart %s not open!\r\n", __FUNCTION__, &dev[0]);
		return -1;
	}

	int n = 0, ret;
	while (n < size)
	{
		ret = write(fd, &data[n], size - n);
		if (ret < 0) {
			tcflush(fd, TCOFLUSH);
			perror("write");
			break;
		}
		else if (ret == 0) {
			printf("%s: ret == 0 ?\r\n", __FUNCTION__);
		}
		n += ret;
	}

	return n;
}

int Uart::IsOpen(void)
{
	return (fd >= 0);
}


void Uart::SetBaudrate(int baudrate)
{
	speed = baudrate;
	if (IsOpen())
		ReconfigureUart();
}


void Uart::ReconfigureUart()
{
	if (!IsOpen())
		return;

	struct termios options; // The options for the file descriptor

	if (tcgetattr(fd, &options) == -1) {
		printf("<error>::tcgetattr\n");
		return;
	}

	// set up raw mode / no echo / binary
	options.c_cflag |= (tcflag_t)(CLOCAL | CREAD);
	options.c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ECHOK | ECHONL |
		ISIG | IEXTEN); //|ECHOPRT

	options.c_oflag &= (tcflag_t)~(OPOST);
	options.c_iflag &= (tcflag_t)~(INLCR | IGNCR | ICRNL | IGNBRK);
#ifdef IUCLC
	options.c_iflag &= (tcflag_t)~IUCLC;
#endif
#ifdef PARMRK
	options.c_iflag &= (tcflag_t)~PARMRK;
#endif

	// setup baud rate
	bool custom_baud = false;
	speed_t baud;
	switch (speed) {
#ifdef B0
	case 0: baud = B0; break;
#endif
#ifdef B50
	case 50: baud = B50; break;
#endif
#ifdef B75
	case 75: baud = B75; break;
#endif
#ifdef B110
	case 110: baud = B110; break;
#endif
#ifdef B134
	case 134: baud = B134; break;
#endif
#ifdef B150
	case 150: baud = B150; break;
#endif
#ifdef B200
	case 200: baud = B200; break;
#endif
#ifdef B300
	case 300: baud = B300; break;
#endif
#ifdef B600
	case 600: baud = B600; break;
#endif
#ifdef B1200
	case 1200: baud = B1200; break;
#endif
#ifdef B1800
	case 1800: baud = B1800; break;
#endif
#ifdef B2400
	case 2400: baud = B2400; break;
#endif
#ifdef B4800
	case 4800: baud = B4800; break;
#endif
#ifdef B7200
	case 7200: baud = B7200; break;
#endif
#ifdef B9600
	case 9600: baud = B9600; break;
#endif
#ifdef B14400
	case 14400: baud = B14400; break;
#endif
#ifdef B19200
	case 19200: baud = B19200; break;
#endif
#ifdef B28800
	case 28800: baud = B28800; break;
#endif
#ifdef B57600
	case 57600: baud = B57600; break;
#endif
#ifdef B76800
	case 76800: baud = B76800; break;
#endif
#ifdef B38400
	case 38400: baud = B38400; break;
#endif
#ifdef B115200
	case 115200: baud = B115200; break;
#endif
#ifdef B128000
	case 128000: baud = B128000; break;
#endif
#ifdef B153600
	case 153600: baud = B153600; break;
#endif
#ifdef B230400
	case 230400: baud = B230400; break;
#endif
#ifdef B256000
	case 256000: baud = B256000; break;
#endif
#ifdef B460800
	case 460800: baud = B460800; break;
#endif
#ifdef B500000
	case 500000: baud = B500000; break;
#endif
#ifdef B576000
	case 576000: baud = B576000; break;
#endif
#ifdef B921600
	case 921600: baud = B921600; break;
#endif
#ifdef B1000000
	case 1000000: baud = B1000000; break;
#endif
#ifdef B1152000
	case 1152000: baud = B1152000; break;
#endif
#ifdef B1500000
	case 1500000: baud = B1500000; break;
#endif
#ifdef B2000000
	case 2000000: baud = B2000000; break;
#endif
#ifdef B2500000
	case 2500000: baud = B2500000; break;
#endif
#ifdef B3000000
	case 3000000: baud = B3000000; break;
#endif
#ifdef B3500000
	case 3500000: baud = B3500000; break;
#endif
#ifdef B4000000
	case 4000000: baud = B4000000; break;
#endif
	default:
		custom_baud = true;
		struct serial_struct ser;

		if (-1 == ioctl(fd, TIOCGSERIAL, &ser)) {
			printf("<error>::ioctl\n");
			return;
		}

		// set custom divisor
		ser.custom_divisor = ser.baud_base / static_cast<int> (speed);
		// update flags
		ser.flags &= ~ASYNC_SPD_MASK;
		ser.flags |= ASYNC_SPD_CUST;

		if (-1 == ioctl(fd, TIOCSSERIAL, &ser)) {
			printf("<error>::ioctl\n");
			return;
		}

	}
	if (custom_baud == false) {
		::cfsetispeed(&options, baud);
		::cfsetospeed(&options, baud);
	}

	// setup char len
	options.c_cflag &= (tcflag_t)~CSIZE;
	if (dataBits == 8)
		options.c_cflag |= CS8;
	else if (dataBits == 7)
		options.c_cflag |= CS7;
	else if (dataBits == 6)
		options.c_cflag |= CS6;
	else if (dataBits == 5)
		options.c_cflag |= CS5;
	else
	{
		printf("<error> invalid char len\n");
		return;
	}
	// setup stopbits
	if (stopBits == 1)
		options.c_cflag &= (tcflag_t)~(CSTOPB);
	else if (stopBits == 3)
		options.c_cflag |= (CSTOPB);
	else if (stopBits == 2)
		options.c_cflag |= (CSTOPB);
	else
	{
		printf("<error> invalid stop bit\n");
		return;
	}

	// setup parity
	options.c_iflag &= (tcflag_t)~(INPCK | ISTRIP);
	if (parity == 0) {
		options.c_cflag &= (tcflag_t)~(PARENB | PARODD);
	}
	else if (parity == 2) {
		options.c_cflag &= (tcflag_t)~(PARODD);
		options.c_cflag |= (PARENB);
	}
	else if (parity == 1) {
		options.c_cflag |= (PARENB | PARODD);
	}
	else if (parity == 3) {
		options.c_cflag |= (PARENB | CMSPAR | PARODD);
	}
	else if (parity == 4) {
		options.c_cflag |= (PARENB | CMSPAR);
		options.c_cflag &= (tcflag_t)~(PARODD);
	}
	else {
		printf("<error> invalid parity\n");
		return;
	}
	// setup flow control
	options.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
	options.c_cflag &= (unsigned long)~(CRTSCTS);

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;

	// activate settings
	::tcsetattr(fd, TCSANOW, &options);
}
char* Uart::DeviceName(void) {
	return dev;
}


IrRx::IrRx(char* device)
{
	memset(dev, 0, sizeof(dev));
	strncpy(dev, device, sizeof(dev));
	fd = -1;
}

IrRx::~IrRx()
{
	Close();
}

void IrRx::Close()
{
	if (fd > 0) {
		close(fd);
		fd = -1;
	}
}

int IrRx::Open()
{
	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		perror("Open");
		return -1;
	}
	return 0;
}


int IrRx::Open(const char* device)
{
	memset(dev, 0, sizeof(dev));
	strncpy(dev, device, sizeof(dev));
	return Open();
}

int IrRx::Read(unsigned char* pucBuf, int iLen, int iTimeOutMs)
{
	int ret;
	struct input_event in;
	struct input_event inTmp;

	if (fd < 0) {
		return -1;
	}

	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);

	struct timeval timeout;
	if (iTimeOutMs == -1) {
		timeout.tv_sec = 0x7fffffff;
		timeout.tv_usec = 0;
	}
	else {
		timeout.tv_sec = iTimeOutMs / 1000;
		timeout.tv_usec = (iTimeOutMs % 1000) * 1000;
	}

	ret = select(fd + 1, &readFds, NULL, NULL, &timeout);
	if (ret > 0) {
		ret = read(fd, &in, sizeof(in));
		if (ret < 0) {
			perror("read");
		}
		else if (ret == sizeof(in)) {
			if (in.type == EV_MSC) {
				unsigned char* p = (unsigned char*)&in.value;
				for (int i = 0; i < sizeof(in.value); i++) {
					p[i] = ((p[i] & 0x01) << 7) | ((p[i] & 0x02) << 5) | ((p[i] & 0x04) << 3) | ((p[i] & 0x08) << 1) | ((p[i] & 0x10) >> 1) | ((p[i] & 0x20) >> 3) | ((p[i] & 0x40) >> 5) | ((p[i] & 0x80) >> 7);
				}
				inTmp.value = 0;
				for (int i = 0; i < sizeof(in.value); i++) {
					inTmp.value |= p[i] << ((sizeof(in.value) - i - 1) * 8);
				}
				in.value = inTmp.value;

				//in.value = (p[0] << 24) |(p[1] << 16) | (p[2] << 8) | (p[3]);
				//printf("ir data: %x\n", in.value);
				if (iLen == sizeof(in)) {   // ��Ҫ��ȡʱ���
					memcpy(pucBuf, &in, sizeof(in));
					return sizeof(in);
				}
				else {                    // ����Ҫ��ȡʱ���
					int iRetLen = (iLen < sizeof(in.value)) ? iLen : sizeof(in.value);
					memcpy(pucBuf, p, iRetLen);
					return iRetLen;
				}
			}
			else {
				//printf("Ir data is invalid!\n");
				return -1;
			}
		}
		return -1;
	}
	else if (ret == 0) {
		return 0;
	}
	else {
		perror("select");
		return ret;
	}

	return -1;
}


#else       //  #ifndef WIN32
#include <windows.h>

Uart::Uart(char* device, int sp, char datab, char stopb, char par)
{

}
int Uart::Open()
{
	printf("Windows: %s: NOT SUPPORT NOW\r\n", __FUNCTION__);
	return 0;
}

void Uart::Close()
{

}

int Uart::Read(unsigned char* pucBuf, int iLen, int iTimeOutMs)
{
	Sleep(iTimeOutMs);
	return 0;
}

int Uart::Write(unsigned char* data, int size)
{
	printf("%s: size = %d bytes\r\n", __FUNCTION__, size);
	for (int i = 0; i < size; i++) {
		printf("%02x, ", data[i]);
	}
	printf("\r\n\r\n");

	return 0;
}

int Uart::IsOpen(void)
{
	return 0;
}

#endif      //  #ifndef WIN32

