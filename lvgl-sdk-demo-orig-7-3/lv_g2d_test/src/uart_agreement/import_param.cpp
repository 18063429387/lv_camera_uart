#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <string>
//#include "DbgPrint.h"
//#include "merge_location.h"
//#include "bv_util.h"
//#include "bv_param.h"

#define  __PI__             3.1415926f
#define RAD2DGREE(x)      ((x) * 180.0f / __PI__)
#define DGREE2RAD(x)      ((x) * __PI__ / 180.0f)

using namespace std;


static int str2num (char *pcStr, int *piData, int iMaxCnt)
{
    int n, tmp;
    char *pcTmp1, *pcTmp2;
    char cNum[16];
    n = 0;
    pcTmp1 = pcStr;
    do {
        //  �����ָ���
        while (*pcTmp1 && (*pcTmp1 < '0' || *pcTmp1 > '9') && *pcTmp1 != '-') pcTmp1++; //  '-'Ϊ����
        if (*pcTmp1 == 0) return n;
        pcTmp2 = pcTmp1;

        //  ֱ����һ���ָ���
        while (*pcTmp2 && ((*pcTmp2 >= '0' && *pcTmp2 <= '9') || *pcTmp2 == '-')) pcTmp2++;
        if (pcTmp2 == pcTmp1) return n; //  ����

        cNum[0] = 0;
        tmp = pcTmp2 - pcTmp1;
        memcpy(cNum, pcTmp1, tmp);
        cNum[tmp] = 0;
        tmp = atoi(cNum);
        piData[n++] = tmp;
        if (n >= iMaxCnt) {
            return n;
        }
        pcTmp1 = pcTmp2;
    } while (*pcTmp1);
    return n;
}

static int str2num (char *pcStr, double *pfData, int iMaxCnt)
{
    int n, tmp;
    char *pcTmp1, *pcTmp2;
    char cNum[64];
    double DataT = 0;

    n = 0;
    pcTmp1 = pcStr;
    do {
        //  �����ָ���
        while (*pcTmp1 && (*pcTmp1 < '0' || *pcTmp1 > '9') && *pcTmp1 != '-' && *pcTmp1 != '.' && *pcTmp1 != 'e' && *pcTmp1 != 'E') pcTmp1++; //  '-'Ϊ����
        if (*pcTmp1 == 0) return n;
        pcTmp2 = pcTmp1;

        //  ֱ����һ���ָ���
        while (*pcTmp2 && ((*pcTmp2 >= '0' && *pcTmp2 <= '9') || *pcTmp2 == '-' || *pcTmp2 == '.' || *pcTmp2 == 'e' || *pcTmp2 == 'E')) pcTmp2++;
        if (pcTmp2 == pcTmp1) return n; //  ����

        cNum[0] = 0;
        tmp = pcTmp2 - pcTmp1;
        memcpy(cNum, pcTmp1, tmp);
        cNum[tmp] = 0;
        DataT = atof(cNum);
        pfData[n++] = DataT;
        if (n >= iMaxCnt) {
            return n;
        }
        pcTmp1 = pcTmp2;
    } while (*pcTmp1);
    return n;
}

int myGetPrivateProfileString(char *pcSeg, char *pcName, char *pcDefault, char *pcOutStr, int iOutStrLen, char *pcFile, int IsDeleteSpace = 1)
{
    string FileBuf;
    FileBuf.clear();
    FileBuf.append(pcFile);

    string Seg, Name;
    Seg.clear();
    Name.clear();

    char cTmp[128];
    int len = FileBuf.length();
    string::size_type position1, position2, position_t; 
    string::size_type pos_start, pos_end;

    //  �ҵ� pcSeg ��
    pos_start = -1;
    sprintf(cTmp, "[%s]", pcSeg);
    while (1) {
        pos_start = FileBuf.find(cTmp, pos_start + 1);
        if (pos_start == FileBuf.npos) {
            //PRINTF("myGetPrivateProfileString: No seg %s\r\n", pcSeg);
            strcpy(pcOutStr, pcDefault);
            return 0;
        }
        //  �ж� pcSeg �Ƿ񶥸�д
        if (pos_start > 0 && (FileBuf.at(pos_start - 1) != '\r' && FileBuf.at(pos_start - 1) != '\n')) {
            continue;
        }
        break;
    }

    pos_end = pos_start + strlen(cTmp) -1;
    while (1) {
        pos_end = FileBuf.find("[", pos_end + 1);
        if (pos_end == FileBuf.npos) {
            pos_end = len;
            break;
        }
        //  �ж��Ƿ񶥸�д
        if (pos_end > 0 && (FileBuf.at(pos_end - 1) != '\r' && FileBuf.at(pos_end - 1) != '\n')) {
            continue;
        }
        break;
    }

    Seg = FileBuf.substr(pos_start+strlen(cTmp), pos_end - pos_start);
    if (Seg.empty()) {
        //PRINTF("myGetPrivateProfileString: Seg %s No name %s\r\n", pcSeg, pcName);
        strcpy(pcOutStr, pcDefault);
        return 0;
    }

    //  ��������д��pcName, ���Ƕ���д������ԣ������ҵ�ע�Ͷ�
    position1 = -1;
    char name2[128] = {0};
    sprintf(name2, "%s=", pcName);  //  ���� =
    while (1) {
        position1 = Seg.find(name2, position1 + 1);
        if (position1 == FileBuf.npos) {
            //PRINTF("myGetPrivateProfileString: Seg %s No name %s\r\n", pcSeg, pcName);
            strcpy(pcOutStr, pcDefault);
            return 0;
        }
        //  �ж� pcName �Ƿ񶥸�д
        if (position1 > 0 && (Seg.at(position1 - 1) != '\r' && Seg.at(position1 - 1) != '\n')) {
            continue;
        }
        break;
    }
    len = Seg.length();
    pos_end = len;
    //  �ҵ��н�β
    position2 = Seg.find("\r", position1 + strlen(pcName));
    if (position2 == Seg.npos) {
        position2 = len;
    }
    position_t = Seg.find("\n", position1 + strlen(pcName));
    if (position_t == Seg.npos) {
        position_t = len;
    }
    position2 = position2 > position_t ? position_t : position2;

    //  Ѱ�� = ��
    Name = Seg.substr(position1, position2 - position1);
    position1 = 0;
    position1 = Name.find("=", position1);
    if (position1 == Name.npos) {
        //PRINTF("myGetPrivateProfileString: Seg [%s] name %s is EMPTY\r\n", pcSeg, pcName);
        strcpy(pcOutStr, pcDefault);
        return 0;
    }
    len = Name.length();
    int i, n;
    n = 0;
    for (i = position1 + 1; i < len; i++) {
        if (IsDeleteSpace) {                //  ɾ����ؿո�
            if (Name.at(i) != ' ' && n < iOutStrLen) {
                pcOutStr[n++] = Name.at(i);
            }
        } else {                            //  ��ɾ���ո�ֱ�Ӱ�ԭ�����
            if (n < iOutStrLen) {
                pcOutStr[n++] = Name.at(i);
            }
        }
    }
    pcOutStr[n] = 0;
    return n;
}

#define GETPRIVATEPROFILESTRING    myGetPrivateProfileString
//#define GETPRIVATEPROFILESTRING    GetPrivateProfileString


int mySetPrivateProfileString(char *pcSeg, char *pcName, char *pcNewStr, char *pcFile, int iFileLenMax)
{
    string FileBuf, NewFileBuf;
    FileBuf.clear();
    FileBuf.append(pcFile);
    NewFileBuf.clear();
    
    string Seg, Name;
    Seg.clear();
    Name.clear();

    char cTmp[128];
    int len = FileBuf.length();
    string::size_type position1, position2, position_t; 
    string::size_type pos_start, pos_end;

    //  �ҵ� pcSeg ��
    pos_start = -1;
    sprintf(cTmp, "[%s]", pcSeg);
    while (1) {
        pos_start = FileBuf.find(cTmp, pos_start + 1);
        if (pos_start == FileBuf.npos) {
            printf("mySetPrivateProfileString: No seg %s, create it.\r\n", pcSeg);
            break;
        }
        //  �ж� pcSeg �Ƿ񶥸�д
        if (pos_start > 0 && (FileBuf.at(pos_start - 1) != '\r' && FileBuf.at(pos_start - 1) != '\n')) {
            continue;
        }
        break;
    }

    //  û�иöΣ�ֱ�����ļ���ĩβ����
    if (pos_start == FileBuf.npos) {
        NewFileBuf.append(FileBuf);
        sprintf(cTmp, "\r\n[%s]\r\n", pcSeg);
        NewFileBuf.append(cTmp);

        sprintf(cTmp, "%s=%s\r\n", pcName, pcNewStr);
        NewFileBuf.append(cTmp);
        goto __set_end;
    } else if (pos_start != 0) {
        //  ��������ǰ��Ķ�
        NewFileBuf.append(FileBuf, 0, pos_start);
    }

    pos_end = pos_start + strlen(cTmp) -1;
    while (1) {
        pos_end = FileBuf.find("[", pos_end + 1);
        if (pos_end == FileBuf.npos) {
            pos_end = len;
            break;
        }
        //  �ж��Ƿ񶥸�д
        if (pos_end > 0 && (FileBuf.at(pos_end - 1) != '\r' && FileBuf.at(pos_end - 1) != '\n')) {
            continue;
        }
        break;
    }

    NewFileBuf.append(FileBuf, pos_start, strlen(cTmp));        //  ��������

    Seg = FileBuf.substr(pos_start+strlen(cTmp), pos_end - pos_start - strlen(cTmp));
    if (Seg.empty()) {
        //  ���ж������Ҹö��������
        printf("myGetPrivateProfileString: Seg %s Only seg name, no any key item, now create key: %s\r\n", pcSeg, pcName);
        NewFileBuf.append(FileBuf);

        sprintf(cTmp, "%s=%s\r\n", pcName, pcNewStr);
        NewFileBuf.append(cTmp);
        goto __set_end;

        //  ������ע��û�д��������������������������У�����һ���γ��˶�����û���κ��ַ��������ո񣩵������
    }

    //  ��������д��pcName, ���Ƕ���д������ԣ������ҵ�ע�Ͷ�
    position1 = -1;
    while (1) {
        position1 = Seg.find(pcName, position1 + 1);
        if (position1 == FileBuf.npos) {
            printf("mySetPrivateProfileString: Seg %s No name %s, now create it.\r\n", pcSeg, pcName);
            break;
        }
        //  �ж� pcName �Ƿ񶥸�д
        if (position1 > 0 && (Seg.at(position1 - 1) != '\r' && Seg.at(position1 - 1) != '\n')) {
            continue;
        }
        break;
    }
    len = Seg.length();

    if (position1 == FileBuf.npos) {        //  ����û���ҵ���Ӧ�������ڶ�ĩβ����֮
        NewFileBuf.append(Seg);
        sprintf(cTmp, "\r\n%s=%s\r\n", pcName, pcNewStr);
        NewFileBuf.append(cTmp);
        goto __set_end1;
    } else if (position1 != 0) {
        NewFileBuf.append(Seg, 0, position1);
    }

    //  �ҵ��н�β
    position2 = Seg.find("\r", position1 + strlen(pcName));
    if (position2 == Seg.npos) {
        position2 = len - 1;
    }
    position_t = Seg.find("\n", position1 + strlen(pcName));
    if (position_t == Seg.npos) {
        position_t = len - 1;
    }
    position2 = position2 > position_t ? position2 : position_t;

    //  �µ�����
    sprintf(cTmp, "%s=%s\r\n", pcName, pcNewStr);
    NewFileBuf.append(cTmp);

    //  ���� Seg ������ַ�
    if (position2 != Seg.npos && position2 < len - 1) {
        NewFileBuf.append(Seg, position2 + 1, Seg.length() - position2);
    }

__set_end1:
    if (pos_end != FileBuf.npos) {
        NewFileBuf.append(FileBuf, pos_end, FileBuf.length() - pos_end);
    }
    NewFileBuf.append("\0");

__set_end:
    if (iFileLenMax > 0) {
        iFileLenMax -= 1;   //  ����һ���ֽ� \0
        iFileLenMax = iFileLenMax > NewFileBuf.length() ? NewFileBuf.length() : iFileLenMax;
        strncpy(pcFile, NewFileBuf.c_str(), iFileLenMax);
        pcFile[iFileLenMax] = 0;
    } else {
        len = NewFileBuf.length();
        const char *p = NewFileBuf.c_str();
        printf("len = %d:\r\n%s\r\n", len, NewFileBuf.c_str());
        strcpy(pcFile, NewFileBuf.c_str());
        iFileLenMax = strlen(pcFile);
    }
    return iFileLenMax;         //  �µ��ļ�����
}

void createSeg (char *pcFile, char *pcSeg)
{
    strcat(pcFile, "\r\n[");
    strcat(pcFile, pcSeg);
    strcat(pcFile, "]\r\n");
}

void appendName(char *pcFile, char *pcName, int data)
{
    char strtmp[100] = {0}; 
    sprintf(strtmp, "%s=%d\r\n", pcName, data);
    strcat(pcFile, strtmp);
}

void appendName(char *pcFile, char *pcName, float data)
{
    char strtmp[100] = {0};     //  �������п��ܺܶ�λ���֣������Ҫ�����Щ
    if (abs(data) >= 0.01) {
        sprintf(strtmp, "%s=%.10f\r\n", pcName, data);
    } else {
        sprintf(strtmp, "%s=%.10e\r\n", pcName, data);
    }
    strcat(pcFile, strtmp);
}

void appendName(char *pcFile, char *pcName, char *pdata)
{
    char strtmp[512] = {0}; 
    sprintf(strtmp, "%s=%s\r\n", pcName, pdata);
    strcat(pcFile, strtmp);
}


int getNameValue (char *pcBuf, char *pcSeg, char *pcName, int iDefaultValue)
{
    char cDefault[100] = {0};
    char cOutStr[100] = {0};
    int  iValue = iDefaultValue;

    GETPRIVATEPROFILESTRING(pcSeg, pcName, cDefault, cOutStr, sizeof(cOutStr), pcBuf);
    if (cOutStr[0]) {
        iValue = atoi(cOutStr);
    }
    return iValue;
}

float getNameValue (char *pcBuf, char *pcSeg, char *pcName, float fDefaultValue)
{
    char cDefault[100] = {0};
    char cOutStr[100] = {0};
    float fValue = fDefaultValue;

    GETPRIVATEPROFILESTRING(pcSeg, pcName, cDefault, cOutStr, sizeof(cOutStr), pcBuf);
    if (cOutStr[0]) {
        fValue = atof(cOutStr);
    }
    return fValue;
}

int getNameValueStr(char * pcBuf, char * pcSeg, char * pcName, char *pcDefaultStr, char *pcStrOut)
{
    char cDefault[64] = "default";
    char cOutStr[256] = {0};
    int iRt = 0;

    if (pcDefaultStr && pcDefaultStr[0]) {
        strcpy(cDefault, pcDefaultStr);
    }
    if (!pcBuf || !pcBuf[0] || !pcSeg || !pcSeg[0] || !pcName || !pcName[0]) {
        strcpy(pcStrOut, "None");
        return -1;
    }
    
    iRt = GETPRIVATEPROFILESTRING(pcSeg, pcName, cDefault, cOutStr, sizeof(cOutStr), pcBuf, 0);   //  iRt Ϊ��ȡ�����ַ����ȣ���û�и���򷵻�Ϊ���ַ�����Ϊ0��
    if (cOutStr[0]) {
        strcpy(pcStrOut, cOutStr);
    } else {
        strcpy(pcStrOut, cDefault);
    }
    return iRt > 0 ? 0 : -1;
}

#if 0
CvRect getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvRect rect_default) {
    CvRect rect      = cvRect(0, 0, 0, 0);
    char cPtBuf[128] = {0};
    char cPtDefaultBuf[128];

    sprintf(cPtDefaultBuf, "(%d,%d,%d,%d)", rect_default.x, rect_default.y, rect_default.width, rect_default.height);

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cPtDefaultBuf, cPtBuf);
    if (iRt < 0) {
        rect = rect_default;
    } else {
        sscanf(cPtBuf, "(%d,%d,%d,%d)", &rect.x, &rect.y, &rect.width, &rect.height);
    }
    return rect;
}

CvSize getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvSize sz_default) {
    CvSize sz      = cvSize(1280, 720);
    char cPtBuf[128] = {0};
    char cPtDefaultBuf[128];

    sprintf(cPtDefaultBuf, "(%d,%d)", sz_default.width, sz_default.height);

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cPtDefaultBuf, cPtBuf);
    if (iRt < 0) {
        sz = sz_default;
    } else {
        sscanf(cPtBuf, "(%d,%d)", &sz.width, &sz.height);
    }
    return sz;
}

CvPoint3D32f getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint3D32f pt_default) {
    CvPoint3D32f pt  = cvPoint3D32f(0, 0, 0);
    char cPtBuf[128] = {0};

    char cDefault[128];
    sprintf(cDefault,"(%f,%f,%f)", pt_default.x, pt_default.y, pt_default.z);

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cDefault, cPtBuf);
    if (iRt < 0) {
        pt = pt_default;
    } else {
        sscanf(cPtBuf, "(%f,%f,%f)", &pt.x, &pt.y, &pt.z);
    }
    return pt;
}

CvPoint2D32f getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint2D32f pt_default) {
    CvPoint2D32f pt  = cvPoint2D32f(0, 0);
    char cPtBuf[128] = {0};

    char cDefault[128];
    sprintf(cDefault,"(%f,%f)", pt_default.x, pt_default.y);

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cDefault, cPtBuf);
    if (iRt < 0) {
        pt = pt_default;
    } else {
        sscanf(cPtBuf, "(%f,%f)", &pt.x, &pt.y);
    }
    return pt;
}

CvPoint getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint pt_default) {
    CvPoint pt  = cvPoint(0, 0);
    char cPtBuf[128] = {0};

    char cDefault[128];
    sprintf(cDefault,"(%d,%d)", pt_default.x, pt_default.y);

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cDefault, cPtBuf);
    if (iRt < 0) {
        pt = pt_default;
    } else {
        sscanf(cPtBuf, "(%d,%d)", &pt.x, &pt.y);
    }
    return pt;
}

CvScalar getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvScalar pt_default) {
    CvScalar pt  = cvScalar(0, 0, 0, 255);
    float r,g,b,a;
    char cPtBuf[128] = {0};

    char cDefault[128];
    sprintf(cDefault, "(%d,%d,%d,%d)", (int)(pt_default.val[0]), (int)(pt_default.val[1]), (int)(pt_default.val[2]), (int)(pt_default.val[3]));

    int iRt = getNameValueStr(pcBuf, pcSeg, pcName, cDefault, cPtBuf);
    if (iRt < 0) {
        pt = pt_default;
    } else {
        int iCnt = 0;
        int i = 0;
        while (1) {
            if (cPtBuf[i] == 0) {
                break;
            }
            if (cPtBuf[i] == ',') {
                iCnt++;
            }
            i++;
        }

        r = g = b = a = 0;
        a = 255;
        if (i == 4) {           //  ���� alhpa
            sscanf(cPtBuf, "(%f,%f,%f,%f)", &r, &g, &b, &a);
        } else {
            sscanf(cPtBuf, "(%f,%f,%f)", &r, &g, &b);
        }
        pt.val[0] = r;
        pt.val[1] = g;
        pt.val[2] = b;
        pt.val[3] = a;
    }
    return pt;
}
#endif

void setNameValue (char *pcBuf, char *pcSeg, char *pcName, float fData, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "%g", fData);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}

void setNameValue (char *pcBuf, char *pcSeg, char *pcName, int iData, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "%d", iData);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}

#if 0
void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint pt, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "(%d,%d)", pt.x, pt.y);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}

void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint2D32f pt, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "(%f,%f)", pt.x, pt.y);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}

void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint3D32f pt, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "(%f,%f,%f)", pt.x, pt.y, pt.z);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}

void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvRect rect, int iMaxFileLen)
{
    char cTmp[128];
    sprintf(cTmp, "(%d,%d,%d,%d)", rect.x, rect.y, rect.width, rect.height);
    mySetPrivateProfileString(pcSeg, pcName, cTmp, pcBuf, iMaxFileLen);
}
#endif
