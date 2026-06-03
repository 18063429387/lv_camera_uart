#ifndef __PROCESS_COM_H
#define __PROCESS_COM_H
#include <stdio.h>
#include <string.h>


#define COMTYPE_KEY					    1 		//  ??????
#define COMTYPE_QUERY				    2		//  ?????
#define COMTYPE_SETSTATE				3		//  ??????

#define COMTYPE_KEY_UP			        1
#define COMTYPE_KEY_DOWN		        2
#define COMTYPE_KEY_LEFT			    3
#define COMTYPE_KEY_RIGHT		        4
#define COMTYPE_KEY_ENTER		        5
#define COMTYPE_KEY_RETURN		        6
#define COMTYPE_KEY_MENU		        7


#define COMTYPE_SETSTATE_START		    1       //  ???????????
#define COMTYPE_SETSTATE_ACTIVE		    2       //  ??????????????????????—¥
#define COMTYPE_SETSTATE_PAUSE		    3       //  ????????????????????????§Ý????????—¥
#define COMTYPE_SETSTATE_STOP			4       //  ???????????

#define SOCKET_BUFFER_SIZE_DEFAULT      (32768U)

// #ifndef WIN32
#if 0
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/reboot.h>

#include "t5/vr_processing.h"
#include "t5/enc_api.h"

#include <sys/types.h>
#include <sys/socket.h>

class ProcessComBase {
public:
	ProcessComBase() {
		m_iSockets[0] = -1;
		m_iSockets[1] = -1;
		strcpy(m_cName, "anony");
		Create(0);
	}

	ProcessComBase(int bufferSize, int iIsBlock = 0, char* pName = NULL) {
		m_iSockets[0] = -1;
		m_iSockets[1] = -1;
		if (!pName) {
			strcpy(m_cName, "anony");
		}
		else {
			memcpy(m_cName, pName, strlen(pName) >= sizeof(m_cName) - 1 ? sizeof(m_cName) - 2 : strlen(pName));
			m_cName[strlen(m_cName)] = 0;
		}

		Create(bufferSize, iIsBlock);
	}

	~ProcessComBase() {

	}

	void Create(int bufferSize = 0, int iIsBlock = 0) {
		if (bufferSize <= 0) {
			bufferSize = SOCKET_BUFFER_SIZE_DEFAULT;
		}
		printf("%s: iIsBlock = %d, 0x%08x\r\n", __FUNCTION__, iIsBlock, (iIsBlock ? 0 : SOCK_NONBLOCK));
		socketpair(AF_UNIX, SOCK_SEQPACKET | (iIsBlock ? 0 : SOCK_NONBLOCK) /* ????? block */, 0, m_iSockets);
		setsockopt(m_iSockets[0], SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
		setsockopt(m_iSockets[0], SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
		setsockopt(m_iSockets[1], SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
		setsockopt(m_iSockets[1], SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
		printf("%s: m_iSockets = (%d, %d)\r\n", __FUNCTION__, m_iSockets[0], m_iSockets[1]);
	}

	int WriteCom(unsigned char* pucBuf, int len) {
		return write(m_iSockets[0], pucBuf, len);
	}

	int ReadCom(unsigned char* pucBuf, int len) {
		return read(m_iSockets[0], pucBuf, len);
	}

	void GetId(int* piId) {
		if (piId) {
			piId[0] = m_iSockets[0];
			piId[1] = m_iSockets[1];
		}
	}

public:
	int m_iSockets[2];
	char m_cName[32];
};
#else
class ProcessComBase {
public:
	ProcessComBase() {
	}

	ProcessComBase(int bufferSize, int iIsBlock = 0, char* pName = NULL) {

	}

	~ProcessComBase() {

	}

	void Create(int bufferSize = 0) {

	}

	int WriteCom(unsigned char* pucBuf, int len) {
		return -1;
	}

	int ReadCom(unsigned char* pucBuf, int len) {
		return -1;
	}

	void GetId(int* piId) {
		if (piId) {
			piId[0] = m_iSockets[0];
			piId[1] = m_iSockets[1];
		}
	}

public:
	int m_iSockets[2];
	char m_cName[32];
};

#endif

class ProcessCom : public ProcessComBase {
public:
	ProcessCom() {
		m_type = 0;
		m_sid = 0;
		m_pucBufT = new unsigned char[256]();
	}

	ProcessCom(char type, unsigned char sid) {
		m_type = type;
		m_sid = sid;
		m_pucBufT = new unsigned char[256]();
	}

	~ProcessCom() {
		if (m_pucBufT) {
			delete[] m_pucBufT;
		}
	}

	int Write(unsigned char* pucBuf, int len) {
		if (len >= 250) {       //  ??????? 250 ???
			return -1;
		}
		m_pucBufT[0] = (unsigned char)m_type;
		m_pucBufT[1] = len + 3;
		m_pucBufT[2] = (unsigned char)m_sid;
		memcpy(&m_pucBufT[3], pucBuf, len);
		return WriteCom(&m_pucBufT[0], len + 3);
	}

	int Read(unsigned char* pucBuf, int len) {
		if (len >= 250) {       //  ??????? 250 ???
			return -1;
		}
		pucBuf[0] = 0;
		m_pucBufT[0] = 0;
		m_pucBufT[1] = 0;

		int iRt = ReadCom(&m_pucBufT[0], len + 3);
		if (iRt < 3) {
			return -1;
		}
		printf("%s: ", __FUNCTION__);
		for (int i = 0; i < iRt; i++) {
			printf("%02x ", m_pucBufT[i]);
		}
		printf("\r\n");

		memcpy(&pucBuf[0], &m_pucBufT[3], iRt - 3);

		return iRt - 3;
	}

public:
	char            m_type;
	unsigned char   m_sid;
	unsigned char* m_pucBufT;
};

#endif

