#ifndef WIN32
#include "ExternalIoCtrl.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../ref_inc/osport.h" 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ComProtocols.h"
#include "ComProt.h"
#include "threadUart.h"
#include "CayenneCarData.h"

#define ACC_TICK_TIMES          200
#define GEAR_TICK_TIMES         200             // 200ms
#define LR_LIGHT_TICK_TIMES     200             // 200ms
#define LR_LIGHT_ON_TICK_TIMES  200             // 200ms
#define LR_LIGHT_OFF_TICK_TIMES 1500            // 1.5S

typedef struct car_signal {
	unsigned int can;
	unsigned int left;
	unsigned int right;
	unsigned int gear;
	unsigned long long left_tick;
	unsigned long long right_tick;
	unsigned long long gear_tick;
	unsigned long long tick;

} car_signal_t;

CarInfoValidFlags_t CarInfoValidFlag = { 0 };
IOCarInfo_t IOCtlCarInfo = { 0, 0, 0, 1 };

#define TICK_PERIOD(n) ((n)/3)         // unit: ms

void ClearCarInfoValidFlag()
{
	memset(&CarInfoValidFlag, 0, sizeof(CarInfoValidFlag));
}

void* threadExtIoCtrl(void* p)
{
	char extIOdev[32] = "/dev/aw_gpio";
	int fd = -1;
	car_signal_t extIoSignal = { 0 };
	car_signal_t preExtIoSignal = { 0 };
	uint32_t timeCnt = 0;
	uint32_t accCnt = 0;
	uint32_t leftLightCnt = 0;
	uint32_t rightLightCnt = 0;
	uint32_t doubleLightCnt = 0;
	uint32_t offLightCnt = 0;
	uint32_t gearCnt = 0;
	uint32_t gear_R_Cnt = 0;

	uint8_t acc = 0;
	uint8_t gear = 0;
	uint8_t light = 0;

	IOCtlCarInfo.gear = 0x03;       // default gear_D.
	CarInfoValidFlag.accFlag |= IOCTRL_MASK_BIT;
	CarInfoValidFlag.gearFlag |= IOCTRL_MASK_BIT;
	CarInfoValidFlag.lightFlag |= IOCTRL_MASK_BIT;

	printf("%s\n", __FUNCTION__);
	fd = open(extIOdev, O_RDONLY);

	if (fd <= 0) {
		printf("<ERROR>: open %s is failed.\r\n", extIOdev);
		//return NULL;
	}
	sleep(3);

	CarInfo_t CarInfo = { 0 };
	CarInfo_t PreCarInfo = { 0 };
	uint32_t isStatusUpdate = 0;
	uint32_t isRadarUpdate = 0;
	uint32_t isRadar6644Update = 0;
	DWORD VbatMeasureUpdateTime = 0;
	DWORD CarInfoUpdateTime = 0;

	IOCtlCarInfo.accValid = 1;
	
	while (1) {

		#if 0
		int iRet = read(fd, (char*)&extIoSignal, sizeof(extIoSignal));
		if (iRet != sizeof(extIoSignal)) {
			printf("<ERROR>:  %s read error.iRet=%d\r\n", extIOdev, iRet);
		}
		// can
		if (preExtIoSignal.can == extIoSignal.can) {
			if (++accCnt >= 25) {
				if (extIoSignal.can) {
					IOCtlCarInfo.can = 1;
				}
				else {
					IOCtlCarInfo.can = 0;
				}
				accCnt = 0;
			}
		}
		else {
			accCnt = 0;
			preExtIoSignal.can = extIoSignal.can;
		}


		// right & left light
		if (((extIoSignal.tick - extIoSignal.left_tick) > TICK_PERIOD(1300)) &&
			((extIoSignal.tick - extIoSignal.right_tick) > TICK_PERIOD(1300))) {      // 1.3s
			if ((extIoSignal.left == 0) && extIoSignal.right == 0) {
				if (++offLightCnt >= 3) {
					IOCtlCarInfo.lightstatus = 0x00;
					offLightCnt = 0;
				}
			}
			else {
				offLightCnt = 0;
			}
			leftLightCnt = 0;
			rightLightCnt = 0;
			doubleLightCnt = 0;
		}
		else if (((extIoSignal.tick - extIoSignal.left_tick) > TICK_PERIOD(100)) &&
			((extIoSignal.tick - extIoSignal.right_tick) > TICK_PERIOD(100))) {      // 150ms
			if ((extIoSignal.left == 1) && extIoSignal.right == 0) {
				if (++leftLightCnt >= 3) {
					IOCtlCarInfo.lightstatus = 0x01;
					leftLightCnt = 0;
				}
				rightLightCnt = 0;
				doubleLightCnt = 0;
				offLightCnt = 0;
			}
			else if ((extIoSignal.left == 0) && extIoSignal.right == 1) {
				if (++rightLightCnt >= 3) {
					IOCtlCarInfo.lightstatus = 0x02;
					rightLightCnt = 0;
				}
				leftLightCnt = 0;
				doubleLightCnt = 0;
				offLightCnt = 0;
			}
			else if ((extIoSignal.left == 1) && extIoSignal.right == 1) {
				if (++doubleLightCnt >= 3) {
					IOCtlCarInfo.lightstatus = 0x04;
					doubleLightCnt = 0;
				}
				leftLightCnt = 0;
				rightLightCnt = 0;
				offLightCnt = 0;
			}
		}
		else if (((extIoSignal.tick - extIoSignal.left_tick) <= TICK_PERIOD(100)) &&
			((extIoSignal.tick - extIoSignal.right_tick) <= TICK_PERIOD(100))) {
			if (++doubleLightCnt >= 7) {
				IOCtlCarInfo.lightstatus = 0x04;
				doubleLightCnt = 0;
			}
			leftLightCnt = 0;
			rightLightCnt = 0;
			offLightCnt = 0;
		}
		else if (((extIoSignal.tick - extIoSignal.left_tick) <= TICK_PERIOD(100)) &&
			((extIoSignal.tick - extIoSignal.right_tick) > TICK_PERIOD(300))) {
			if ((extIoSignal.right == 0) && (++leftLightCnt >= 7)) {
				IOCtlCarInfo.lightstatus = 0x01;
				leftLightCnt = 0;
			}
			rightLightCnt = 0;
			doubleLightCnt = 0;
			offLightCnt = 0;
		}
		else if (((extIoSignal.tick - extIoSignal.left_tick) > TICK_PERIOD(300)) &&
			((extIoSignal.tick - extIoSignal.right_tick) <= TICK_PERIOD(100))) {
			if ((extIoSignal.left == 0) && (++rightLightCnt >= 7)) {
				IOCtlCarInfo.lightstatus = 0x02;
				rightLightCnt = 0;
			}
			leftLightCnt = 0;
			doubleLightCnt = 0;
			offLightCnt = 0;
		}
		else {
			leftLightCnt = 0;
			rightLightCnt = 0;
			doubleLightCnt = 0;
			offLightCnt = 0;
		}

		// gear
		if (extIoSignal.tick - extIoSignal.gear_tick > TICK_PERIOD(200)) {     // 200ms
			if (preExtIoSignal.gear == extIoSignal.gear) {       // 连续2次读的都一致才
				if (++gearCnt >= 3) {
					if (extIoSignal.gear) {
						IOCtlCarInfo.gear = 0x01;       // R 
					}
					else {
						IOCtlCarInfo.gear = 0x03;       // D 
					}
					gearCnt = 0;
				}
			}
			else {
				gearCnt = 0;
				preExtIoSignal.gear = extIoSignal.gear;
			}
			gear_R_Cnt = 0;
		}
		else if ((extIoSignal.tick - extIoSignal.gear_tick) < TICK_PERIOD(100)) {
			if (++gear_R_Cnt >= 7) {
				IOCtlCarInfo.gear = 0x01;       // R 
			}
		}
		else {
			gear_R_Cnt = 0;
		}
		#endif
		
		// if (!cmdfilter.acc) {
		// 	IOCtlCarInfo.can = 1;
		// }
		// if (!cmdfilter.led) {
		// 	IOCtlCarInfo.lightstatus &= (~0x03);;
		// }
		// if (!cmdfilter.doubleled) {
		// 	IOCtlCarInfo.lightstatus &= (~0x04);;
		// }
		// if (cmdfilter.tap) {
		// 	IOCtlCarInfo.gear = (IOCtlCarInfo.gear == tappos_R ? tappos_R : tappos_D);
		// }
		// if (cmdfilter.shieldgear) {
		// 	IOCtlCarInfo.gear = tappos_P;
		// }

		// if (GPortConf.GetDebugEnable(CAN_DATA_IO_ID))
		// {
		// 	int len = sizeof(IOCarInfo_t);
		// 	uint8_t ucData[len];

		// 	memcpy(ucData, &IOCtlCarInfo, len);

		// 	GPortConf.PushData(ucData, len);
		// }

		// com1 ioctrl数据融合暂时放在这里
		CarInfo.accValid = 1;
		if (CanCarInfo.accValid) {
			CarInfo.cansleep = (!IOCtlCarInfo.can) && (CanCarInfo.cansleep); // MCU(有数据的前提)和IO两者都休眠才认为休眠
		}
		else {
			CarInfo.cansleep = (!IOCtlCarInfo.can);
		}
		// IO作为CAN信号，当CAN sleep 时acc也强制设为off
		if (((!Com1CarInfo.accValid) && (!Com2CarInfo.accValid) && (!CanCarInfo.accValid))) {
			// MCU及串口无车身数据或CVBS同显模式下，ACC以CAN为准
			CarInfo.acc = !CarInfo.cansleep;
		}
		else {
			if (!CarInfo.cansleep && (Com1CarInfo.acc || CanCarInfo.acc || Com2CarInfo.acc )) {
				CarInfo.acc = 1;
			}
			else {
				CarInfo.acc = 0;
			}
		}

		if ((CanCarInfo.gear == 0x01) || (Com1CarInfo.gear == 0x01) || (IOCtlCarInfo.gear == 0x01) || (Com2CarInfo.gear == 0x01)) {
			CarInfo.gear = 0x01;
		}
		else {
			if (CarInfoValidFlag.gearFlag & CAN_MASK_BIT) {
				CarInfo.gear = CanCarInfo.gear;
			}
			else if (CarInfoValidFlag.gearFlag & COM1_MASK_BIT) {
				CarInfo.gear = Com1CarInfo.gear;
			}
			else if (CarInfoValidFlag.gearFlag & COM2_MASK_BIT) {
				CarInfo.gear = Com2CarInfo.gear;
			}
			else {
				CarInfo.gear = 0x0;
				if (CanCarInfo.gear == 0x02 || CanCarInfo.gear == 0x03) {
					CarInfoValidFlag.gearFlag |= CAN_MASK_BIT;
					CarInfo.gear = CanCarInfo.gear;
				}
				if (Com1CarInfo.gear == 0x02 || Com1CarInfo.gear == 0x03) {
					CarInfoValidFlag.gearFlag |= COM1_MASK_BIT;
					CarInfo.gear = Com1CarInfo.gear;
				}
				if (Com2CarInfo.gear == 0x02 || Com2CarInfo.gear == 0x03) {
					CarInfoValidFlag.gearFlag |= COM2_MASK_BIT;
					CarInfo.gear = Com2CarInfo.gear;
				}
			}
		}

		if (CarInfoValidFlag.speedFlag & CAN_MASK_BIT) {
			CarInfo.speed = CanCarInfo.speed;
		}
		else if (CarInfoValidFlag.speedFlag & COM1_MASK_BIT) {
			CarInfo.speed = Com1CarInfo.speed;
		}
		else if (CarInfoValidFlag.speedFlag & COM2_MASK_BIT) {
			CarInfo.speed = Com2CarInfo.speed;
		}
		else {
			CarInfo.speed = 0;      //default 5km/h.
			if (CanCarInfo.speed > 0 && CanCarInfo.speed < 200) {
				CarInfoValidFlag.speedFlag |= CAN_MASK_BIT;
			}
			if (Com1CarInfo.speed > 0 && Com1CarInfo.speed < 200) {
				CarInfoValidFlag.speedFlag |= COM1_MASK_BIT;
			}
			if (Com2CarInfo.speed > 0 && Com2CarInfo.speed < 200) {
				CarInfoValidFlag.speedFlag |= COM2_MASK_BIT;
			}
		}

		if ((IOCtlCarInfo.lightstatus & 0x04) || \
			(CanCarInfo.lightstatus & 0x04) || \
			(Com1CarInfo.lightstatus & 0x04) || \
			(Com2CarInfo.lightstatus & 0x04)) {
			CarInfo.lightstatus = 0x04;
		}
		else if ((IOCtlCarInfo.lightstatus & 0x01) || \
			(CanCarInfo.lightstatus & 0x01) || \
			(Com1CarInfo.lightstatus & 0x01) || \
			(Com2CarInfo.lightstatus & 0x01)) {
			CarInfo.lightstatus = 0x01;
		}
		else if ((IOCtlCarInfo.lightstatus & 0x02) || \
			(CanCarInfo.lightstatus & 0x02) || \
			(Com1CarInfo.lightstatus & 0x02) || \
			(Com2CarInfo.lightstatus & 0x02)) {
			CarInfo.lightstatus = 0x02;
		}
		else if (!(IOCtlCarInfo.lightstatus & 0x07) && \
			!(CanCarInfo.lightstatus & 0x07) && \
			!(Com1CarInfo.lightstatus & 0x07) && \
			!(Com2CarInfo.lightstatus & 0x07)) {
			CarInfo.lightstatus = 0x00;
		}

		unsigned char pKey = (IOCtlCarInfo.lightstatus | CanCarInfo.lightstatus | Com1CarInfo.lightstatus | Com2CarInfo.lightstatus) & 0x40;
		CarInfo.lightstatus = (CarInfo.lightstatus & (~0x40)) | pKey;
		CarInfo.autopark = Com1CarInfo.autopark | Com2CarInfo.autopark | CanCarInfo.autopark;

		//CarInfo.corner = Com1CarInfo.corner;
		/*CarInfo.doorstatus = Com1CarInfo.doorstatus;*/

		if (CarInfoValidFlag.connerFlag & CAN_MASK_BIT) {
			CarInfo.corner = CanCarInfo.corner;
		}
		else if (CarInfoValidFlag.connerFlag & COM1_MASK_BIT) {
			CarInfo.corner = Com1CarInfo.corner;
		}
		else if (CarInfoValidFlag.connerFlag & COM2_MASK_BIT) {
			CarInfo.corner = Com2CarInfo.corner;
		}
		else {
			if (CanCarInfo.corner != 0) {
				CarInfoValidFlag.connerFlag |= CAN_MASK_BIT;
			}
			if (Com1CarInfo.corner != 0) {
				CarInfoValidFlag.connerFlag |= COM1_MASK_BIT;
			}
			if (Com2CarInfo.corner != 0) {
				CarInfoValidFlag.connerFlag |= COM2_MASK_BIT;
			}

			if (CarInfoValidFlag.connerFlag & CAN_MASK_BIT) {
				CarInfo.corner = CanCarInfo.corner;
			}
			else if (CarInfoValidFlag.connerFlag & COM1_MASK_BIT) {
				CarInfo.corner = Com1CarInfo.corner;
			}
			else if (CarInfoValidFlag.connerFlag & COM2_MASK_BIT) {
				CarInfo.corner = Com2CarInfo.corner;
			}
			else {
				CarInfo.corner = 0;
			}
		}

		if (CarInfoValidFlag.doorFlag & CAN_MASK_BIT) {
			CarInfo.doorstatus = CanCarInfo.doorstatus;
		}
		else if (CarInfoValidFlag.doorFlag & COM1_MASK_BIT) {
			CarInfo.doorstatus = Com1CarInfo.doorstatus;
		}
		else if (CarInfoValidFlag.doorFlag & COM2_MASK_BIT) {
			CarInfo.doorstatus = Com2CarInfo.doorstatus;
		}
		else {
			if (CanCarInfo.doorstatus != 0) {
				CarInfoValidFlag.doorFlag |= CAN_MASK_BIT;
			}
			if (Com1CarInfo.doorstatus != 0) {
				CarInfoValidFlag.doorFlag |= COM1_MASK_BIT;
			}
			if (Com2CarInfo.doorstatus != 0) {
				CarInfoValidFlag.doorFlag |= COM2_MASK_BIT;
			}

			if (CarInfoValidFlag.doorFlag & CAN_MASK_BIT) {
				CarInfo.doorstatus = CanCarInfo.doorstatus;
			}
			else if (CarInfoValidFlag.doorFlag & COM1_MASK_BIT) {
				CarInfo.doorstatus = Com1CarInfo.doorstatus;
			}
			else if (CarInfoValidFlag.doorFlag & COM2_MASK_BIT) {
				CarInfo.doorstatus = Com2CarInfo.doorstatus;
			}
			else {
				CarInfo.doorstatus = 0;
			}
		}

		if (CarInfoValidFlag.radarFlag & CAN_MASK_BIT) {
			if (memcmp((void*)&CarInfo.Radar6644, (void*)&CanCarInfo.Radar6644, sizeof(Radar6644_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&CanCarInfo.Radar, sizeof(Radar_t));
				memcpy((void*)&CarInfo.Radar6644, (void*)&CanCarInfo.Radar6644, sizeof(Radar6644_t));
				isStatusUpdate = 1;
				isRadarUpdate = 0;
				isRadar6644Update = 1;
			}
			else if (memcmp((void*)&CarInfo.Radar, (void*)&CanCarInfo.Radar, sizeof(Radar_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&CanCarInfo.Radar, sizeof(Radar_t));
				isStatusUpdate = 1;
				isRadarUpdate = 1;
			}
		}
		else if (CarInfoValidFlag.radarFlag & COM1_MASK_BIT) {
			if (memcmp((void*)&CarInfo.Radar6644, (void*)&Com1CarInfo.Radar6644, sizeof(Radar6644_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&Com1CarInfo.Radar, sizeof(Radar_t));
				memcpy((void*)&CarInfo.Radar6644, (void*)&Com1CarInfo.Radar6644, sizeof(Radar6644_t));
				isStatusUpdate = 1;
				isRadarUpdate = 0;
				isRadar6644Update = 1;
			}
			else if (memcmp((void*)&CarInfo.Radar, (void*)&Com1CarInfo.Radar, sizeof(Radar_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&Com1CarInfo.Radar, sizeof(Radar_t));
				isStatusUpdate = 1;
				isRadarUpdate = 1;
			}
		}
		else if (CarInfoValidFlag.radarFlag & COM2_MASK_BIT) {
			if (memcmp((void*)&CarInfo.Radar6644, (void*)&Com2CarInfo.Radar6644, sizeof(Radar6644_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&Com2CarInfo.Radar, sizeof(Radar_t));
				memcpy((void*)&CarInfo.Radar6644, (void*)&Com2CarInfo.Radar6644, sizeof(Radar6644_t));
				isStatusUpdate = 1;
				isRadarUpdate = 0;
				isRadar6644Update = 1;
			}
			else if (memcmp((void*)&CarInfo.Radar, (void*)&Com2CarInfo.Radar, sizeof(Radar_t)) != 0) {
				memcpy((void*)&CarInfo.Radar, (void*)&Com2CarInfo.Radar, sizeof(Radar_t));
				isStatusUpdate = 1;
				isRadarUpdate = 1;
			}
		}
		else {
			bool isRadar6644 = false;
			uint8_t* pData = (uint8_t*)&(Com1CarInfo.Radar6644);
			for (uint32_t i = 1; i < sizeof(Radar6644_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= COM1_MASK_BIT;
					isRadar6644 = true;
					break;
				}
			}
			pData = (uint8_t*)&(Com1CarInfo.Radar);
			for (uint32_t i = 1; i < sizeof(Radar_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= COM1_MASK_BIT;
					break;
				}
			}

			pData = (uint8_t*)&(Com2CarInfo.Radar6644);
			for (uint32_t i = 1; i < sizeof(Radar6644_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= COM2_MASK_BIT;
					isRadar6644 = true;
					break;
				}
			}
			pData = (uint8_t*)&(Com2CarInfo.Radar);
			for (uint32_t i = 1; i < sizeof(Radar_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= COM2_MASK_BIT;
					break;
				}
			}

			pData = (uint8_t*)&(CanCarInfo.Radar6644);
			for (uint32_t i = 1; i < sizeof(Radar6644_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= CAN_MASK_BIT;
					isRadar6644 = true;
					break;
				}
			}
			pData = (uint8_t*)&(CanCarInfo.Radar);
			for (uint32_t i = 1; i < sizeof(Radar_t); i++) {
				if (pData[i] != 0) {
					CarInfoValidFlag.radarFlag |= CAN_MASK_BIT;
					break;
				}
			}

			if (CarInfoValidFlag.radarFlag & CAN_MASK_BIT) {
				if (isRadar6644) {
					memcpy((void*)&CarInfo.Radar, (void*)&CanCarInfo.Radar, sizeof(Radar_t));
					memcpy((void*)&CarInfo.Radar6644, (void*)&CanCarInfo.Radar6644, sizeof(Radar6644_t));
					isStatusUpdate = 1;
					isRadarUpdate = 0;
					isRadar6644Update = 1;
				}
				else {
					memcpy((void*)&CarInfo.Radar, (void*)&CanCarInfo.Radar, sizeof(Radar_t));
					isStatusUpdate = 1;
					isRadarUpdate = 1;
				}
			}
			else if (CarInfoValidFlag.radarFlag & COM1_MASK_BIT) {
				if (isRadar6644) {
					memcpy((void*)&CarInfo.Radar, (void*)&Com1CarInfo.Radar, sizeof(Radar_t));
					memcpy((void*)&CarInfo.Radar6644, (void*)&Com1CarInfo.Radar6644, sizeof(Radar6644_t));
					isStatusUpdate = 1;
					isRadarUpdate = 0;
					isRadar6644Update = 1;
				}
				else {
					memcpy((void*)&CarInfo.Radar, (void*)&Com1CarInfo.Radar, sizeof(Radar_t));
					isStatusUpdate = 1;
					isRadarUpdate = 1;
				}
			}
			else if (CarInfoValidFlag.radarFlag & COM2_MASK_BIT) {
				if (isRadar6644) {
					memcpy((void*)&CarInfo.Radar, (void*)&Com2CarInfo.Radar, sizeof(Radar_t));
					memcpy((void*)&CarInfo.Radar6644, (void*)&Com2CarInfo.Radar6644, sizeof(Radar6644_t));
					isStatusUpdate = 1;
					isRadarUpdate = 0;
					isRadar6644Update = 1;
				}
				else {
					memcpy((void*)&CarInfo.Radar, (void*)&Com2CarInfo.Radar, sizeof(Radar_t));
					isStatusUpdate = 1;
					isRadarUpdate = 1;
				}
			}
		}

		if (PreCarInfo.gear != CarInfo.gear) {
			isStatusUpdate = 1;
			PreCarInfo.gear = CarInfo.gear;
		}

		if (PreCarInfo.lightstatus != CarInfo.lightstatus) {
			isStatusUpdate = 1;
			PreCarInfo.lightstatus = CarInfo.lightstatus;
		}
		if (PreCarInfo.autopark != CarInfo.autopark) {
			isStatusUpdate = 1;
			PreCarInfo.autopark = CarInfo.autopark;
		}
		if (PreCarInfo.doorstatus != CarInfo.doorstatus) {
			isStatusUpdate = 1;
			PreCarInfo.doorstatus = CarInfo.doorstatus;
		}
		if (PreCarInfo.cansleep != CarInfo.cansleep) {
			isStatusUpdate = 1;
			PreCarInfo.cansleep = CarInfo.cansleep;
		}
		if (isStatusUpdate || GetTickCount() - CarInfoUpdateTime >= (200)) {
			ComProtocolSendCmd(AvmMsgQueueRx, CP_CMD_CARDATA, (uint8_t*)&CarInfo, 8);
			if (isRadar6644Update) {
				ComProtocolSendCmd(AvmMsgQueueRx, CP_CMD_RADAR_6644, (uint8_t*)&CarInfo.Radar6644, sizeof(CarInfo.Radar6644));
				isRadar6644Update = 0;
			}
			else if (isRadarUpdate) {
				ComProtocolSendCmd(AvmMsgQueueRx, CP_CMD_RADAR, (uint8_t*)&CarInfo.Radar, sizeof(CarInfo.Radar));
				isRadarUpdate = 0;
			}
			isStatusUpdate = 0;
			CarInfoUpdateTime = GetTickCount();
		}

		Sleep(10);       // 20ms
	}
}

#endif



