#include "CSystemConf.h"
#include "CParamFile.h"
#include <assert.h>

#include "CSetings.h"

#include "core/bv_types.h"
#include "core/bvglobal.h"
#include "ref_inc/import_param.h"
#include "tool/util.h"

static const char* dispNames[] = {
	"CVBS","AHD","TVI","VGA","HDMI","LVDS"
};

static const char* cameraInterface[] = {
	"AHD","TVI"
};

void hdmiHpdSoftwareTrigger(int isPlugin)
{
#ifndef WIN32
	int ret = 0;
	int fd = open("/sys/class/hdmi/hdmi/attr/hpd_mask", O_RDWR);
	if (fd < 0) {
		perror("open /sys/class/hdmi/hdmi/attr/hpd_mask");
		return;
	}
	if (isPlugin) {
		ret = write(fd, "0x11", strlen("0x11"));
		if (ret <= 0) {
			perror("write /sys/class/hdmi/hdmi/attr/hpd_mask");
			return;
		}
	}
	else {
		ret = write(fd, "0x10", strlen("0x10"));
		if (ret <= 0) {
			perror("write /sys/class/hdmi/hdmi/attr/hpd_mask");
			return;
		}
	}
#endif
	printf("%s %s.\n", __func__, isPlugin ? "plug in" : "plug out");
	return;
}

CSystemConf::CSystemConf()
{
#if WIN32
	strcpy(m_fileName, "systemconf.ini");
#else
	strcpy(m_fileName, "/mnt/UDISK/systemconf.ini");
#endif

	CMyFile file;
	file.open(m_fileName, "rb");

	char pcFileBuf[1024];

	file.read(pcFileBuf, file.getFileSize());
	file.close();

	char str[128] = { 0 };
	m_enInterface = disp_cvbs;
	
	getNameValueStr(pcFileBuf, "system", "interfacetype", "CVBS", m_dDispParam.iface);
	
	if (!strcmp(m_dDispParam.iface, "RGB") || !strcmp(m_dDispParam.iface, "LCD"))
		m_enInterface = disp_vga;
	else
		util::str2Enum<disp_type>(dispNames, COUNTOF(dispNames), m_dDispParam.iface, m_enInterface);

	getNameValueStr(pcFileBuf, "system", "interfacetype_locked", "0", str);
	m_interfacetypeLocked = atoi(str);

	getNameValueStr(pcFileBuf, "system", "interfaceparam", "PAL", m_dDispParam.param);

	getNameValueStr(pcFileBuf, "system", "lcd_dclk_freq", "0", str);
	m_dDispParam.lcd_dclk_freq = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_x", "0", str);
	m_dDispParam.lcd_x = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_y", "0", str);
	m_dDispParam.lcd_y = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_vt", "0", str);
	m_dDispParam.lcd_vt = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_ht", "0", str);
	m_dDispParam.lcd_ht = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_vbp", "0", str);
	m_dDispParam.lcd_vbp = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_hbp", "0", str);
	m_dDispParam.lcd_hbp = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_vspw", "0", str);
	m_dDispParam.lcd_vspw = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_hspw", "0", str);
	m_dDispParam.lcd_hspw = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_fps", "0", str);
	m_dDispParam.lcd_fps = atoi(str);

    getNameValueStr(pcFileBuf, "system", "lcd_lvds_mode", "0", str);
    m_dDispParam.lcd_lvds_mode = atoi(str);
	getNameValueStr(pcFileBuf, "system", "lcd_lvds_mode_locked", "0", str);
	m_lvdsModeLocked = atoi(str);

    getNameValueStr(pcFileBuf, "system", "lcd_lvds_link", "0", str);
    m_dDispParam.lcd_lvds_link = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_hv_clk_phase", "2", str);
	m_dDispParam.lcd_hv_clk_phase = atoi(str);
	getNameValueStr(pcFileBuf, "system", "lcd_hv_clk_phase_locked", "0", str);
	m_clkPhaseLocked = atoi(str);

	getNameValueStr(pcFileBuf, "system", "lcd_hv_sync_polarity", "100", str);
	m_dDispParam.lcd_hv_sync_polarity = atoi(str);

	getNameValueStr(pcFileBuf, "system", "id", "3", str);
	m_dDispParam.id = atoi(str);

	getNameValueStr(pcFileBuf, "system", "decorder_id", "0", str);
	m_decorderID = atoi(str);

	getNameValueStr(pcFileBuf, "system", "render_x", "0", str);
	m_nRenderX = atoi(str);
	getNameValueStr(pcFileBuf, "system", "render_y", "0", str);
	m_nRenderY = atoi(str);
	getNameValueStr(pcFileBuf, "system", "render_w", "0", str);
	m_nRenderWidth = atoi(str);
	getNameValueStr(pcFileBuf, "system", "render_h", "0", str);
	m_nRenderHeight = atoi(str);

	getNameValueStr(pcFileBuf, "system", "dual", "0", str);
	m_nDual = atoi(str);

	if ((!m_dDispParam.lcd_dclk_freq) && (!m_dDispParam.lcd_fps)) {
		char* p = strrchr(m_dDispParam.param, '@');
		if (p) {
			p = p + 1;
			m_dDispParam.lcd_fps = atoi(p);
		}
	}
	else if ((!m_dDispParam.lcd_dclk_freq) && (m_dDispParam.lcd_fps)) {
		m_dDispParam.lcd_dclk_freq = m_dDispParam.lcd_ht * m_dDispParam.lcd_vt * m_dDispParam.lcd_fps / 1e6;
	}
	else if ((m_dDispParam.lcd_dclk_freq) && (!m_dDispParam.lcd_fps)) {
		m_dDispParam.lcd_fps = (m_dDispParam.lcd_dclk_freq * 1e6) / (m_dDispParam.lcd_ht * m_dDispParam.lcd_vt);
	}

	m_CamSize = getNameValue(pcFileBuf, "video", "cam_size", 1080);
	printf("lcd_dclk_freq=%d, lcd_fps=%d, cam_size=%d\n", m_dDispParam.lcd_dclk_freq, m_dDispParam.lcd_fps, m_CamSize);

	char camIface[32] = { 0 };
	getNameValueStr(pcFileBuf, "video", "cam_iface", "AHD", camIface);
	util::str2Enum<camera_interface>(cameraInterface, COUNTOF(cameraInterface), camIface, m_enCamInterface);

	m_swichValue = getNameValue(pcFileBuf, "switch", "value", 0x3F);

	m_bCadillacDistort = !!getNameValue(pcFileBuf, "system", "cadillac_distort", 0);
	m_enUartDefine = (uart_define)getNameValue(pcFileBuf, "pins", "uart2key", uart_def_uart);

	m_bChange = false; 
	m_bRender = false;

	if(m_enInterface == disp_hdmi) {
		hdmiHpdSoftwareTrigger(1);
    }
	else {
		// ����DE02-GPP���Ҳ�ΪCVBS/AHD/TVI���ʱ��Ӧǿ�ƴ���HPD������HDMIתVGAʱδ��HDMI��ʾ��ʱ�������?
		CMyFile lt8612sx;
		lt8612sx.open("/lt8612sx.ini", "rb");
		memset(pcFileBuf, 0, sizeof(pcFileBuf));
		lt8612sx.read(pcFileBuf, lt8612sx.getFileSize());
		lt8612sx.close();
		if (strstr(pcFileBuf, "in=HDMI")) {
			if ((m_enInterface != disp_cvbs) && (m_enInterface != disp_ahd) && (m_enInterface != disp_tvi)) {
				hdmiHpdSoftwareTrigger(1);
			}
		}
	}
}

CSystemConf& CSystemConf::instance()
{
	static CSystemConf _instance;
	return _instance;
}

int CSystemConf::GetCameraSize()
{
	return m_CamSize;
}

void CSystemConf::SetCameraSize(int size)
{
	m_CamSize = size;
	m_bChange = true;
}

camera_interface CSystemConf::GetCameraInterface()
{
	return m_enCamInterface;
}

void CSystemConf::SetCameraInterface(camera_interface iface)
{
	if (m_enCamInterface != iface) {
		m_bChange = true;
	}
	m_enCamInterface = iface;
}

uart_define CSystemConf::GetUartDefine() {
	return m_enUartDefine;
}

void CSystemConf::SetUartDefine(uart_define func) {
	if (m_enUartDefine != func) {
		m_bChange = true;
	}
	m_enUartDefine = func;
}

disp_type CSystemConf::GetInterface()
{
	return m_enInterface;
}

void CSystemConf::GetDisplayinfo(disp_param_t& dispParam)
{
	memcpy(&dispParam, &m_dDispParam, sizeof(disp_param_t));
}

#ifndef WIN32
void CSystemConf::GetDisplayinfo(DispParam_t *pDispParam)
{
	if (pDispParam == NULL) {
		return;
	}
	if (m_enInterface == disp_ahd)
		pDispParam->dispmode = Mode_AHD;
	else if (m_enInterface == disp_tvi)
		pDispParam->dispmode = Mode_AHD;
	else if (m_enInterface == disp_vga)
		pDispParam->dispmode = Mode_VGA;
	else if (m_enInterface == disp_hdmi)
		pDispParam->dispmode = Mode_HDMI;
	else if (m_enInterface == disp_lvds)
		pDispParam->dispmode = Mode_LVDS;
	else
		pDispParam->dispmode = Mode_AV;

	pDispParam->pclk = m_dDispParam.lcd_dclk_freq * 1000;
	pDispParam->x = m_dDispParam.lcd_x;
	pDispParam->y = m_dDispParam.lcd_y;

	pDispParam->ht = m_dDispParam.lcd_ht;
	pDispParam->hspw = m_dDispParam.lcd_hspw;
	pDispParam->hbp = m_dDispParam.lcd_hbp - m_dDispParam.lcd_hspw;
	pDispParam->hfp = m_dDispParam.lcd_ht - m_dDispParam.lcd_x - m_dDispParam.lcd_hbp;

	pDispParam->vt = m_dDispParam.lcd_vt;
	pDispParam->vspw = m_dDispParam.lcd_vspw;
	pDispParam->vbp = m_dDispParam.lcd_vbp - m_dDispParam.lcd_vspw;
	pDispParam->vfp = m_dDispParam.lcd_vt - m_dDispParam.lcd_y - m_dDispParam.lcd_vbp;

	pDispParam->hpolarity = !!(m_dDispParam.lcd_hv_sync_polarity & 0x01);
	pDispParam->vpolarity = !!(m_dDispParam.lcd_hv_sync_polarity & 0x02);
	pDispParam->clk_phase = m_dDispParam.lcd_hv_clk_phase;
	pDispParam->rotate = 0;
	pDispParam->lvds_link = m_dDispParam.lcd_lvds_link;
	pDispParam->lvds_mode = m_dDispParam.lcd_lvds_mode;
}
#endif

bool CSystemConf::CheckDispParam(disp_param_t& pDispParam)
{
    int vbp = (int)(pDispParam.lcd_vbp) - (int)(pDispParam.lcd_vspw);
    int hbp = (int)(pDispParam.lcd_hbp) - (int)(pDispParam.lcd_hspw);
    int hfp = (int)(pDispParam.lcd_ht) - (int)(pDispParam.lcd_x) - (int)(pDispParam.lcd_hbp);
    int vfp = (int)(pDispParam.lcd_vt) - (int)(pDispParam.lcd_y) - (int)(pDispParam.lcd_vbp);

    if ((pDispParam.lcd_dclk_freq < 7000) || (pDispParam.lcd_dclk_freq > 400000)) {
        printf("lcd_dclk_freq = %d is invalid!", pDispParam.lcd_dclk_freq);
        goto err;
    }
    if ((pDispParam.lcd_x < 120) || (pDispParam.lcd_x > 4096)) {
        printf("lcd_x = %d is invalid!", pDispParam.lcd_x);
        goto err;
    }
    if ((pDispParam.lcd_y < 120) || (pDispParam.lcd_y > 4096)) {
        printf("lcd_y = %d is invalid!", pDispParam.lcd_y);
        goto err;
    }

    if ((pDispParam.lcd_vspw < 1) || (pDispParam.lcd_hspw < 1)) {
        printf("vspw = % d or hspw = % d is invalid!\n", pDispParam.lcd_vspw, pDispParam.lcd_hspw);
        goto err;
    }
    if ((vbp < 1) || (hbp < 1)) {
        printf("vbp = %d , hbp = %d is invalid!", vbp, hbp);
        goto err;
    }
    if ((hfp < 1) || (vfp < 1)) {
        printf("hfp = %d or vfp = %d is invalid!", hfp, vfp);
        goto err;
    }
    return true;
err:
    return false;
}

void CSystemConf::HdmiParamCheckModify(disp_param_t& dispParam)
{
	if (!strcmp(dispParam.iface, "HDMI")) {
		int hBlank = dispParam.lcd_ht - dispParam.lcd_x;
		if (hBlank < 50) {
			printf("%s: lcd_ht=%d, lcd_hbp = %d modified to ", __FUNCTION__, dispParam.lcd_ht, dispParam.lcd_hbp);
			dispParam.lcd_ht += (80 - hBlank);
			dispParam.lcd_hbp += ((80 - hBlank) / 2);
			printf("lcd_ht=%d, lcd_hbp = %d\n", dispParam.lcd_ht, dispParam.lcd_hbp);
		}
	}
}

bool CSystemConf::SetDisplayinfo(disp_param_t& dispParam)
{
	if (m_interfacetypeLocked)
	{
		strcpy(dispParam.iface, m_dDispParam.iface);
	}
	if (m_clkPhaseLocked)
	{
		dispParam.lcd_hv_clk_phase = m_dDispParam.lcd_hv_clk_phase;
	}

	if (m_lvdsModeLocked)
	{
		dispParam.lcd_lvds_mode = m_dDispParam.lcd_lvds_mode;
	}

	HdmiParamCheckModify(dispParam);

	if (DispInfoCmp(dispParam, m_dDispParam))
	{
		m_nDual = 0;
		m_nRenderX = 0;
		m_nRenderY = 0;
		m_nRenderWidth = 0;
		m_nRenderHeight = 0;

		memcpy(&m_dDispParam, &dispParam, sizeof(m_dDispParam));
		/* ͬ���޸�m_enInterface������m_enInterface��m_dDispParam.iface��ͬ����
		*  ������GetDisplayinfo������ȡ��dispmode��������ֵ�������ͬ�������Σ���������������ָ���Ҳ�ָ���ӿڣ���?xFD
		*  ������������һ�£�Ҳ����Ϊ��ȡ��m_enInterface�ӿ���ǰһ��ͬ���Ľӿڲ�һ�¶���������
		*/
		if (!strcmp(m_dDispParam.iface, "RGB") || !strcmp(m_dDispParam.iface, "LCD"))
			m_enInterface = disp_vga;
		else
			util::str2Enum<disp_type>(dispNames, COUNTOF(dispNames), m_dDispParam.iface, m_enInterface);

#if DISP_AUTO_WEIGET_STYLE
		ChangeResolutionStyle(m_dDispParam.lcd_x, m_dDispParam.lcd_y);
#endif
		m_bChange = true;
		return true;
	}
	return false;
}

bool CSystemConf::SetDisplayinfoPreset(const char *pIface, const char *pParam)
{
	#if 0
	std::vector<DispFactory> vecDisp = DispTableOps::instance().getTable();
	disp_param_t udtParam;
	bool bFound = false;

	if (pIface == NULL || pParam == NULL) {
		return false;
	}

	for (DispFactory& disp : vecDisp)
	{
		if ((!strcmp(disp.param.iface, pIface)) && (!strcmp(disp.param.param, pParam))) {
			memcpy(&udtParam, &disp.param, sizeof(disp_param_t));
			bFound = true;
			break;
		}
	}

	if (bFound == false) {
		return false;
	}
	return SetDisplayinfo(udtParam);
	#else
	return false;
	#endif
}

bool CSystemConf::GetInterfacetypeLocked()
{
	return m_interfacetypeLocked;
}

void CSystemConf::SetInterfacetypeLocked(bool isLocked)
{
	m_bChange = (m_interfacetypeLocked == isLocked) ? m_bChange : true;
	m_interfacetypeLocked = isLocked;
}

void CSystemConf::GetLvdsMode(bool &isLocked, uint &lvdsMode)
{
	lvdsMode = m_dDispParam.lcd_lvds_mode;
	isLocked = m_lvdsModeLocked;
}

bool CSystemConf::SetLvdsMode(bool isLocked, uint lvdsMode)
{
	if (lvdsMode > 1) {
		return false;
	}

	if ((isLocked != m_lvdsModeLocked) || (lvdsMode != m_dDispParam.lcd_lvds_mode)) {
		m_lvdsModeLocked = isLocked;
		m_dDispParam.lcd_lvds_mode = lvdsMode;
		m_bChange = true;
		return true;
	}
	return false;
}

void CSystemConf::GetClockPhase(bool& isLocked, uint& mode)
{
	mode = m_dDispParam.lcd_hv_clk_phase;
	isLocked = m_clkPhaseLocked;
}

bool CSystemConf::SetClockPhase(bool isLocked, uint mode)
{
	if (mode > 4) {
		return false;
	}

	if ((isLocked != m_clkPhaseLocked) || (mode != m_dDispParam.lcd_hv_clk_phase)) {
		m_clkPhaseLocked = isLocked;
		m_dDispParam.lcd_hv_clk_phase = mode;
		m_bChange = true;
		return true;
	}
	return false;
}

int CSystemConf::GetDispID()
{
	return m_dDispParam.id;
}

bool CSystemConf::SetDispID(int id)
{
	// std::vector<DispFactory> vecDisp = DispTableOps::instance().getTable();
	// disp_param_t udtParam;

	// bool bExits = false;
	// for (DispFactory& disp : vecDisp)
	// {
	// 	if (disp.param.id == id)
	// 	{
	// 		memcpy(&udtParam, &disp.param, sizeof(disp_param_t));
	// 		bExits = true;
	// 		break;
	// 	}

	// 	if (bExits)
	// 		break;
	// }

	// if (bExits)
	// {
	// 	SetDisplayinfo(udtParam);
	// 	Wirte2File();

		return true;
	// }

	// return false;
}

void CSystemConf::Wirte2File()
{
	if (!m_bChange)
		return;
	m_bChange = false;

	char pcFileBuf[1024] = {0};
	char str[128] = { 0 };
	int fbAdjEn = 0;
	int renderAdjEn = 0;

	/*
		1. ֻ��RGB/LCD/VGA��֧������������֧����Ⱦ�������ã�
		2. AHD��CVBS��֧����Ⱦ�������ã���ע�⣺���ӿ�ΪAHD��CVBSʱ��������������������?
	 */

	sprintf(pcFileBuf, "[system]\ninterfacetype=%s\ninterfacetype_locked=%d\ninterfaceparam=%s\n", m_dDispParam.iface, m_interfacetypeLocked, m_dDispParam.param);
	if (m_dDispParam.lcd_dclk_freq) {
		sprintf(str, "lcd_dclk_freq=%d\n", m_dDispParam.lcd_dclk_freq);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_x) {
		sprintf(str, "lcd_x=%d\n", m_dDispParam.lcd_x);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_y) {
		sprintf(str, "lcd_y=%d\n", m_dDispParam.lcd_y);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_vt) {
		sprintf(str, "lcd_vt=%d\n", m_dDispParam.lcd_vt);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_ht) {
		sprintf(str, "lcd_ht=%d\n", m_dDispParam.lcd_ht);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_vbp) {
		sprintf(str, "lcd_vbp=%d\n", m_dDispParam.lcd_vbp);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_hbp) {
		sprintf(str, "lcd_hbp=%d\n", m_dDispParam.lcd_hbp);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_vspw) {
		sprintf(str, "lcd_vspw=%d\n", m_dDispParam.lcd_vspw);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_hspw) {
		sprintf(str, "lcd_hspw=%d\n", m_dDispParam.lcd_hspw);
		strcat(pcFileBuf, str);
	}

	if (m_dDispParam.lcd_fps) {
		sprintf(str, "lcd_fps=%d\n", m_dDispParam.lcd_fps);
		strcat(pcFileBuf, str);
	}

	if ((0 <= m_dDispParam.lcd_hv_clk_phase) && (m_dDispParam.lcd_hv_clk_phase <= 3)) {
		sprintf(str, "lcd_hv_clk_phase=%d\n", m_dDispParam.lcd_hv_clk_phase);
		strcat(pcFileBuf, str);
	}
	sprintf(str, "lcd_hv_clk_phase_locked=%d\n", m_clkPhaseLocked);
	strcat(pcFileBuf, str);

	if ((0 <= m_dDispParam.lcd_hv_sync_polarity) && (m_dDispParam.lcd_hv_sync_polarity <= 3)) {
		sprintf(str, "lcd_hv_sync_polarity=%d\n", m_dDispParam.lcd_hv_sync_polarity);
		strcat(pcFileBuf, str);
	}

	if ((0 == m_dDispParam.lcd_lvds_mode) || (m_dDispParam.lcd_lvds_mode == 1)) {
		sprintf(str, "lcd_lvds_mode=%d\n", m_dDispParam.lcd_lvds_mode);
		strcat(pcFileBuf, str);
	}
	sprintf(str, "lcd_lvds_mode_locked=%d\n", m_lvdsModeLocked);
	strcat(pcFileBuf, str);

	if ((0 == m_dDispParam.lcd_lvds_link) || (m_dDispParam.lcd_lvds_link == 2)) {
		sprintf(str, "lcd_lvds_link=%d\n", m_dDispParam.lcd_lvds_link);
		strcat(pcFileBuf, str);
    }

	if (m_nDual) {
		sprintf(str, "dual=%d\n", 1);
		strcat(pcFileBuf, str);
	}

	sprintf(str, "cadillac_distort=%d\n", m_bCadillacDistort);
	strcat(pcFileBuf, str);

	if (IsRender())
	{
		sprintf(str, "\nrender_x=%d\nrender_y=%d\nrender_w=%d\nrender_h=%d\n",
			m_nRenderX, m_nRenderY, m_nRenderWidth, m_nRenderHeight);
		strcat(pcFileBuf, str);
	}

	sprintf(str, "id=%d\n", m_dDispParam.id);
	strcat(pcFileBuf, str);

	sprintf(str, "decorder_id=%d\n", m_decorderID);
	strcat(pcFileBuf, str);

	sprintf(str, "\n[switch]\nvalue=%d\n", m_swichValue);
	strcat(pcFileBuf, str);

	sprintf(str, "\n[video]\ncam_size=%d\n", m_CamSize);
	strcat(pcFileBuf, str);

	sprintf(str, "cam_iface=%s\n", (m_enCamInterface == interface_tvi) ? "TVI":"AHD");
	strcat(pcFileBuf, str);

	sprintf(str, "\n[pins]\nuart2key=%d\n", m_enUartDefine);
	strcat(pcFileBuf, str);

	strcat(pcFileBuf, "\n[end]\n");

	CMyFile file;
	file.open(m_fileName, "wb+");

	file.write(pcFileBuf, strlen(pcFileBuf));
	file.close();

	printf("[tip] : write systemconf.ini,type = %s, cam = %d, dual = %d\n", m_dDispParam.iface, m_CamSize, m_nDual);
}

bool CSystemConf::DispInfoCmp(const disp_param_t& d1, const disp_param_t& d2)
{
	// ����CVBS������iface��param������ͬ�����ʾ����һ��?
	if ((!strcmp(d1.iface, "CVBS")) && 
		(!strcmp(d1.iface, d2.iface)) &&
		(!strcmp(d1.param, d2.param))) {
		return false;
	}

	if (memcmp(&d1.id, &d2.id, (long)(&d1.disp_priority) - (long)(&d1))) {
		return true;
	}
	return false;
}

void CSystemConf::ChangeResolutionStyle(int width, int height)
{
	GCSetings.SetResolution(width, height);
}

int CSystemConf::SetDualMode(int st)
{
	if (!GCSetings.GetDualSwitch())
		return 0;

	if (!strcmp(m_dDispParam.iface, "CVBS"))
		return 0;

	static bool bFrist = false;
	if (bFrist)
		return 0;

	bFrist = true;

	// ״̬�л�ʱ�ŷ���1�����򷵻�0
	if (m_nDual != st) {
		m_nDual = st;
		m_bChange = true;
		return 1;
	}
	return 0;
}

int CSystemConf::GetDualMode()
{
	return m_nDual;
}

int CSystemConf::SetCadillacDistort(bool isCadillacDistort)
{
	if (isCadillacDistort != m_bCadillacDistort) {
		m_bCadillacDistort = isCadillacDistort;
		m_bChange = true;
		return 1;
	}
	return 0;
}

bool CSystemConf::GetCadillacDistort()
{
	return m_bCadillacDistort;
}

int CSystemConf::SetSwitchValue(unsigned char swichValue)
{
	if (m_swichValue != swichValue) {
		m_bChange = true;
		m_swichValue = swichValue;
		return 1;
	}
	return 0;
}

unsigned char CSystemConf::GetSwitchValue(void)
{
	return m_swichValue;
}

int CSystemConf::IsRender()
{
	if ((m_nRenderX >= 0) && (m_nRenderX < 2048) &&
		(m_nRenderY >= 0) && (m_nRenderY < 2048) &&
		(m_nRenderWidth > 0) && (m_nRenderWidth < 2048) &&
		(m_nRenderHeight > 0) && (m_nRenderHeight < 2048))
	{
		return 1;
	}

	return 0;
}

void CSystemConf::GetRenderROI(int& x, int& y, int& w, int& h)
{
	x = m_nRenderX;
	y = m_nRenderY;
	w = m_nRenderWidth;
	h = m_nRenderHeight;
}

void CSystemConf::SetRenderROI(int x, int y, int w, int h)
{
	m_nRenderX = x;
	m_nRenderY = y;
	m_nRenderWidth = w;
	m_nRenderHeight = h;
	m_bChange = true;
	m_bRender = true;
}

bool CSystemConf::GetRenderEnable()
{
	return m_bRender;
}

void CSystemConf::SetRenderEnable(bool en)
{
	m_bRender = en;
}

