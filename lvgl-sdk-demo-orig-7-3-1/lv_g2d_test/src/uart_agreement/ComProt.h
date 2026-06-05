#ifndef __COMPORT_H
#define __COMPORT_H

//  底板与360模块通信的协议实现
#include <stdio.h>
#include <string.h>
#include "../ref_inc/osport.h"

/*******************************************************************************************************
  模块id，360模块或底板
*******************************************************************************************************/
#define CP_DEVICE_ID_360                    0                       //  360模块
#define CP_DEVICE_ID_BASE                   1                       //  底板或解码器
#define CP_DEVICE_ID_PLAY                   2                       //  核心板的播放程序
#define CP_DEVICE_ID_CARLIFE                3                       //  核心板的 CarLife 程序

/*******************************************************************************************************
  错误码
*******************************************************************************************************/
//  通用错误
#define CP_ERROR_SUCCESS                    0x00
#define CP_ERROR_UNSUPPORT                  0x01                    //  当前不支持该命令
#define CP_ERROR_CRCINVALID                 0x02                    //  CRC错误
#define CP_ERROR_FORMATINVALID              0x03                    //  命令格式不正确
#define CP_ERROR_PARAMINVALID               0x04                    //  缺乏资源文件
#define CP_ERROR_NORESOURCE                 0x05                    //  缺乏资源文件
#define CP_ERROR_UNSUPPORT_AT_CURMODE       0x06                    //  当前模式/状态下，无法执行该命令
																	//  如行车记录仪模式下发送切换全景视图模式，将返回该错误码

//  360 错误码
#define CP_ERROR_INVALIDVIEW                0x10                    //  视图不支持

//  磁盘相关错误码
#define CP_ERROR_NODISK                     0x30                    //  无U盘/SD卡
#define CP_ERROR_BADDISK                    0x31                    //  磁盘损坏（有磁盘但文件系统无法加载）
#define CP_ERROR_WRITEDISKFAILED            0x32                    //  磁盘写入失败（文件系统已加载，但无法写入文件）

//  未知错误
#define CP_ERROR_UNKONW                     0xff                    //  未知错误


/*******************************************************************************************************
  按键（按键值与BK_KEY_xxx值对应，请勿随意改动）
*******************************************************************************************************/
#define CP_KEY_RESERVE                      0x00                    //  保留
#define CP_KEY_HOME                         0x01
#define CP_KEY_MENU                         0x02
#define CP_KEY_ENTER                        0x03
#define CP_KEY_RETURN                       0x04
#define CP_KEY_UP                           0x05
#define CP_KEY_DOWN                         0x06
#define CP_KEY_LEFT                         0x07
#define CP_KEY_RIGHT                        0x08
#define CP_KEY_DISP_ONOFF                   0x09
#define CP_KEY_START_PAIRING                0x0a                    // 遥控按键启动配对

/*******************************************************************************************************
  命令号/功能号
*******************************************************************************************************/
#define CP_CMD_TEST                         0x00
#define CP_CMD_CURMODE                      0x01
#define CP_CMD_VERSION                      0x02
#define CP_CMD_KEY                          0x03
#define CP_CMD_SETVIEW                      0x04
#define CP_CMD_RECORD                       0x05
#define CP_CMD_SYNCTIME                     0x06
#define CP_CMD_TOUCHCOOR                    0x07
#define CP_CMD_ENFUNC                       0x08
#define CP_CMD_CARDATA                      0x09
#define CP_CMD_QUERYSTATE                   0x0a
#define CP_CMD_WHEEL_ANGLE                  0x0b
#define CP_CMD_WHEEL_ANGLE                  0x0b
#define CP_CMD_CALIBRATION                  0X0C
#define CP_CMD_MB_VERSION                   0X0D
#define CP_CMD_RADAR                        0X10
#define CP_CMD_RADAR_6644                   0X11		// 前6后6左4右4
#define CP_CMD_SYSTEM_RESET                 0X1E
#define CP_CMD_NOTICE                       0x20
#define CP_CMD_BUZZER                       0x22
#define CP_CMD_GP_SET2                      0x23
#define CP_CMD_GP_QUERYSTATE2               0x24
#define CP_CMD_SYS_STATUS                   0x25
#define CP_CMD_DISP_PARAM                   0x26
#define CP_CMD_DISP_ROTATE                  0x27
#define CP_CMD_PRODUCTTIME                  0x80
#define CP_CMD_UPDATA_PROGRAM               0x90

#define CP_CMD_YUANCHANG_DEVICE_CHECK		0xEF
#define CP_CMD_IN_TEST_MODE                 0xFA
#define CP_CMD_GET_MEASURE                  0xB0
#define CAR_DATA_GEAR_INVALID               0xF
#define CAR_DATA_CAN_SLEEP                  1

//  返回值为实际发送或接收的数据长度, pvData 为 RegisterTxRxFuc() 中的 pvData, flag 用于指示接收或发送前是否要强制清除串口的接收/发送缓冲区(用于数据发生错乱时)
typedef int (*pfun_com_txrx)(unsigned char* pucBuf, int len, int flag, void* pvData, int iTimeOut);

class ComProt
{
public:
	ComProt(unsigned char ucSelfId /* CP_DEVICE_ID_360 or CP_DEVICE_ID_BASE */, unsigned char ucDstId) {
		m_pfun_tx = NULL;
		m_pfun_rx = NULL;
		m_pvDataTxRx = NULL;
		m_pucBufTx = new unsigned char[256 + 8];       //  协议内容最长256字节，外加4字节帧头和CRC共5字节
		m_pucBufRx = new unsigned char[256 + 8];

		m_ucSelfId = ucSelfId;
		m_ucDstId = ucDstId;
	}

	~ComProt() {
		if (m_pucBufTx) delete[] m_pucBufTx;
		if (m_pucBufRx) delete[] m_pucBufRx;
	}

	//  注册串口发送和接收函数
	void RegisterTxRxFuc(pfun_com_txrx tx, pfun_com_txrx rx, void* pvData) {
		m_pfun_tx = tx;
		m_pfun_rx = rx;
		m_pvDataTxRx = pvData;
	}

	int WriteCmd(unsigned char ucCmd, unsigned char* pucData = NULL, unsigned char ucDataLen = 0) {
		int iRt = 0;
		//  GsComPortCarLife.WriteCmd() 会卡在这里，因为有另外一个专门的线程在ReadAck()，而ReadAck()内部会一直等数据，导致GsComPortCarLife.WriteCmd()卡死。因此不能用 Mutex
		m_MutexWrite.Enter();
		iRt = Send(ucCmd, pucData, ucDataLen, 0, 0);
		m_MutexWrite.Exit();
		return iRt;
	}

	int ReadCmd(unsigned char* pucCmd, unsigned char* pucData, unsigned char* pucDatalen) {
		int iRt = 0;
		m_MutexRead.Enter();
		iRt = Recv(pucCmd, pucData, pucDatalen, NULL, 0);
		m_MutexRead.Exit();
		return iRt;
	}

	int WriteAck(unsigned char ucCmd, unsigned char ucStatus, unsigned char* pucData = NULL, unsigned ucDataLen = 0) {
		int iRt = 0;
		m_MutexWrite.Enter();
		iRt = Send(ucCmd, pucData, ucDataLen, ucStatus, 1);
		m_MutexWrite.Exit();
		return iRt;
	}

	int ReadAck(unsigned char* pucCmd, unsigned char* pucStatus, unsigned char* pucData, unsigned char* pucDataLen) {
		int iRt = 0;
		m_MutexRead.Enter();
		iRt = Recv(pucCmd, pucData, pucDataLen, pucStatus, 1);
		m_MutexRead.Exit();
		return iRt;
	}

	void PrintStatus(unsigned char ucStatus, char* pcPrefix = NULL) {
		if (pcPrefix) printf(pcPrefix);
		switch (ucStatus) {
		case CP_ERROR_SUCCESS:
			printf("CP_ERROR_SUCCESS.\r\n");
			break;

		case CP_ERROR_UNSUPPORT:
			printf("CP_ERROR_UNSUPPORT.\r\n");
			break;

		case CP_ERROR_CRCINVALID:
			printf("CP_ERROR_CRCINVALID.\r\n");
			break;

		case CP_ERROR_FORMATINVALID:
			printf("CP_ERROR_FORMATINVALID.\r\n");
			break;

		case CP_ERROR_PARAMINVALID:
			printf("CP_ERROR_PARAMINVALID.\r\n");
			break;

		case CP_ERROR_NORESOURCE:
			printf("CP_ERROR_NORESOURCE.\r\n");
			break;

		case CP_ERROR_INVALIDVIEW:
			printf("CP_ERROR_INVALIDVIEW.\r\n");
			break;

		case CP_ERROR_NODISK:
			printf("CP_ERROR_NODISK.\r\n");
			break;

		case CP_ERROR_BADDISK:
			printf("CP_ERROR_BADDISK.\r\n");
			break;

		case CP_ERROR_WRITEDISKFAILED:
			printf("CP_ERROR_WRITEDISKFAILED.\r\n");
			break;

		case CP_ERROR_UNKONW:
			printf("CP_ERROR_UNKONW.\r\n");
			break;

		default:
			printf("undefined error!\r\n");
			break;
		}
	}

private:

	int Send(unsigned char ucCmd, unsigned char* pucData, unsigned char ucDataLen, unsigned char ucStatus, unsigned char ucIsAck) {
		if (!m_pfun_tx) {
			printf("[%d]<WARNING> %s: m_pfun_rx is NULL\r\n",__LINE__, __FUNCTION__);
			return -1;
		}
		int iRt = pack(m_pucBufTx, ucCmd, pucData, ucDataLen, ucStatus, ucIsAck);
		return m_pfun_tx(m_pucBufTx, iRt, 0, m_pvDataTxRx, -1);
	}

	int Recv(unsigned char* pucCmd, unsigned char* pucData, unsigned char* pucDataLen, unsigned char* pucStatus, unsigned char ucIsAck) {
		if (!m_pfun_rx) {
			// printf("[%d]<WARNING> %s: m_pfun_rx is NULL\r\n",__LINE__, __FUNCTION__);
			return -1;
		}
		int iRt = 0;
		do {
			int iLen = m_pfun_rx(m_pucBufRx,
				0,                     //  rx 时 len 无意义 
				iRt >= 0 ? 0 : 1,      //  若 iRt < 0, 说明数据错乱，应先清除串口接收缓冲区 
				m_pvDataTxRx,
				-1);
			if (iLen < 0) {
				continue;
			}
			//printf("%s: iLen = %d\r\n", __FUNCTION__, iLen);
			//for (int i = 0; i < iLen; i++) {
			//    printf("%02x, ", m_pucBufRx[i]);
			//}
			//printf("\r\n");
			iRt = unpack(m_pucBufRx, iLen, pucCmd, pucData, pucDataLen, pucStatus, ucIsAck);
			if (iRt != CP_ERROR_SUCCESS) {
				printf("<WARNING> %s: unpack() failed! iRt = %d\r\n", __FUNCTION__, iRt);
				if (iRt > 0) {
					Send(*pucCmd, NULL, 0, (unsigned char)iRt, 1);  //  应答，指示错误帧
				}
				else {
					//  iRt < 0 时，收到的帧头不是 'S' 'W'，不做处理
				}
			}
		} while (iRt != CP_ERROR_SUCCESS);      //  若数据帧有问题，则重复读，直到数据正确
		return iRt;
	}

	int pack(unsigned char* pucComProtBuf,                 //  发送缓冲区
		unsigned char  ucCmd,
		unsigned char* pucData,                       //  D0, D1....Dn 
		unsigned char  ucDataLen,                     //  pucData 的长度 
		unsigned char  ucStatus,                      //  状态。仅在 ACK 时有效
		unsigned char  ucIsAck                        //  0: 发起包, 1: 应答包
	) {
		pucComProtBuf[0] = 'S';
		pucComProtBuf[1] = 'W';
		pucComProtBuf[2] = (m_ucDstId & 0x07) | ((m_ucSelfId & 0x07) << 4);
		if (ucIsAck == 0) {
			pucComProtBuf[3] = ucDataLen + 1;                   //  +1 指 CMD 字节
			pucComProtBuf[4] = ucCmd;
			if (pucData && ucDataLen > 0) {
				memcpy(&pucComProtBuf[5], pucData, ucDataLen);
			}
			pucComProtBuf[5 + ucDataLen] = crc7(0, pucComProtBuf, 5 + ucDataLen);
			return 6 + ucDataLen;
		}
		else {
			pucComProtBuf[3] = ucDataLen + 2;                   //  +1 指 CMD 字节
			pucComProtBuf[4] = ucCmd;
			pucComProtBuf[5] = ucStatus;
			if (pucData && ucDataLen > 0) {
				memcpy(&pucComProtBuf[6], pucData, ucDataLen);
			}
			pucComProtBuf[6 + ucDataLen] = crc7(0, pucComProtBuf, 6 + ucDataLen);
			return 7 + ucDataLen;
		}
	}

	int unpack(unsigned char* pucComProtBuf,               //  发送缓冲区
		int            iBufLen,                     //  pucComProtBuf 的长度
		unsigned char* pucCmd,                      //  解析出来的 ucCmd
		unsigned char* pucData,                     //  解析出来的 D0, D1....Dn 
		unsigned char* pucDataLen,                  //  解析出来的 pucData 长度
		unsigned char* pucStatus,                   //  解析出来的 STATUS. 注意是从数据包中解析出来的 STATUS，而非数据包本身的状态码，
													//  数据包本身的状态码（如数据包CRC不正确）通过返回值返回
		unsigned char  ucIsAck                      //  0: 发起包, 1: 应答包
	) {

		if (pucCmd)      *pucCmd = 0;
		if (pucData)     *pucData = 0;
		if (pucDataLen)  *pucDataLen = 0;
		if (pucStatus)   *pucStatus = 0;

		if (pucComProtBuf[0] != 'S' ||
			pucComProtBuf[1] != 'W' ||
			iBufLen < 6) {                              //  不合法的帧，直接丢弃
			printf("<WARNING> %s: invalid comprot frame!\r\n", __FUNCTION__);
			return -1;
		}
		*pucCmd = pucComProtBuf[4];                     //  应先将 CMD 解析出，因有错误返回时，上层应答时会使用该 CMD
		if (iBufLen != (int)(unsigned int)pucComProtBuf[3] + 5) {
			printf("<WARNING> %s: buf len invalid!\r\n", __FUNCTION__);
			return CP_ERROR_FORMATINVALID;
		}

		if (pucComProtBuf[iBufLen - 1] != crc7(0, pucComProtBuf, iBufLen - 1)) {
			printf("<WARNING> %s: crc incorrent!\r\n", __FUNCTION__);
			return CP_ERROR_CRCINVALID;
		}
		if (pucComProtBuf[3] < (ucIsAck ? 2 : 1)) {                     //  应答时 LEN 至少是 2 （CMD + STATUS），否则至少是1 （CMD）
			printf("<WARNING> %s: LEN invalid!\r\n", __FUNCTION__);
			return CP_ERROR_FORMATINVALID;
		}

		if (ucIsAck == 0) {
			*pucDataLen = pucComProtBuf[3] - 1;
			if (*pucDataLen > 0) {
				memcpy(pucData, &pucComProtBuf[5], *pucDataLen);
			}
		}
		else {
			*pucDataLen = pucComProtBuf[3] - 2;
			*pucStatus = pucComProtBuf[5];
			if (*pucDataLen > 0) {
				memcpy(pucData, &pucComProtBuf[6], *pucDataLen);
			}
		}
		return CP_ERROR_SUCCESS;
	}

	unsigned char crc7(unsigned char ucOri, unsigned char* pucData, unsigned int uiLen) {
		unsigned int  i, j;
		unsigned char ucRlst = ucOri;

		for (i = 0; i < uiLen; i++) {
			for (j = 0; j < 8; j++) {
				ucRlst <<= 1;
				ucRlst ^= ((((pucData[i] << j) ^ ucRlst) & 0x80) ? 0x9 : 0);
			}
		}
		return ((ucRlst & 0x7F));
	}


private:
	pfun_com_txrx  m_pfun_tx;
	pfun_com_txrx  m_pfun_rx;
	void* m_pvDataTxRx;
	unsigned char* m_pucBufTx;
	unsigned char* m_pucBufRx;

	unsigned char  m_ucSelfId;
	unsigned char  m_ucDstId;

	CMutex         m_MutexRead;
	CMutex         m_MutexWrite;
};

#endif

