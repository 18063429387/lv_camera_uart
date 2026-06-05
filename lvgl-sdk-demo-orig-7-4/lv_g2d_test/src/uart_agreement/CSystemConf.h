#ifndef CSYSTEMCONF_H
#define CSYSTEMCONF_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/bv_types.h"
#ifndef WIN32
#include "ComProtocols.h"
#endif

class CSystemConf;
#define GCSystemConf CSystemConf::instance()

class CSystemConf
{
protected:
	CSystemConf();
	CSystemConf(const CSystemConf&) = delete;
	CSystemConf& operator=(const CSystemConf&) = delete;

public:
	static CSystemConf& instance();
	virtual ~CSystemConf() {};

	int GetCameraSize();
	void SetCameraSize(int size);

	camera_interface GetCameraInterface();
	void SetCameraInterface(camera_interface iface);

	uart_define GetUartDefine();
	void SetUartDefine(uart_define func);

	disp_type GetInterface();

	void GetDisplayinfo(disp_param_t& dispParam);
#ifndef WIN32
	void GetDisplayinfo(DispParam_t * pDispParam);
#endif

	void HdmiParamCheckModify(disp_param_t& dispParam);
	bool SetDisplayinfo(disp_param_t& dispParam);
	bool CheckDispParam(disp_param_t& pDispParam);

	bool SetDisplayinfoPreset(const char* pIface, const char* pParam);
	bool GetInterfacetypeLocked();
	void SetInterfacetypeLocked(bool isLocked);

	void GetLvdsMode(bool &isLocked, uint &lvdsMode);
	bool SetLvdsMode(bool isLocked, uint lvdsMode);

	void GetClockPhase(bool& isLocked, uint& mode);
	bool SetClockPhase(bool isLocked, uint mode);

	int GetDispID();

	bool SetDispID(int id);
	int SetSwitchValue(unsigned char swichValue);
	unsigned char GetSwitchValue(void);

	int SetDualMode(int st);

	int GetDualMode();

	int SetCadillacDistort(bool isCadillacDistort);
	bool GetCadillacDistort();

	int IsRender();

	void GetRenderROI(int& x, int& y, int& w, int& h);
	void SetRenderROI(int x, int y, int w, int h);
	bool GetRenderEnable();
	void SetRenderEnable(bool en);
	int GetDispWidth() {
		return m_dDispParam.lcd_x;
	}
	int GetDispHeight() {
		return m_dDispParam.lcd_y;
	}

	void Wirte2File();

protected:
	bool DispInfoCmp(const disp_param_t& d1, const disp_param_t& d2);
	void ChangeResolutionStyle(int width, int height);

private:
	bool                m_bChange;
	bool				m_bRender;
	bool                m_interfacetypeLocked;
	bool                m_clkPhaseLocked;
	bool                m_lvdsModeLocked;
	int                 m_decorderID;
	unsigned char       m_swichValue;
	int                 m_nDual;
	int                 m_CamSize;
	int                 m_nRenderX;
	int                 m_nRenderY;
	int                 m_nRenderWidth;
	int                 m_nRenderHeight;
	char                m_fileName[256];
	disp_param_t        m_dDispParam;
	disp_type           m_enInterface;
	camera_interface    m_enCamInterface;
	bool                m_bCadillacDistort;
	uart_define         m_enUartDefine;
};

#endif // CSYSTEMCONF_H
