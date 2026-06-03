#ifndef __IMPORT_PARAM_H
#define __IMPORT_PARAM_H
//#include "cv.h"

void createSeg(char *pcFile, char *pcSeg);

void appendName(char *pcFile, char *pcName, int data);

void appendName(char *pcFile, char *pcName, float data);

int getNameValue(char *pcBuf, char *pcSeg, char *pcName, int iDefaultValue);

float getNameValue(char *pcBuf, char *pcSeg, char *pcName, float fDefaultValue);

int getNameValueStr(char * pcBuf, char * pcSeg, char * pcName, char *pcDefaultStr, char *pcStrOut);

//vRect getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvRect rect_default);
//vSize getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvSize sz_default);
//vPoint3D32f getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint3D32f pt_default);
//vPoint2D32f getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint2D32f pt_default);
//vPoint getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint pt_default);
//vScalar getNameValue (char *pcBuf, char *pcSeg, char *pcName, CvScalar pt_default);

template <typename T>
T getNameValueT (char *pcBuf, char *pcSeg, char *pcName, T vDefault);

void setNameValue(char *pcBuf, char *pcSeg, char *pcName, float fData, int iMaxFileLen);

void setNameValue(char *pcBuf, char *pcSeg, char *pcName, int iData, int iMaxFileLen);

//void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint pt, int iMaxFileLen);
//void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint2D32f pt, int iMaxFileLen);
//void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvPoint3D32f pt, int iMaxFileLen);
//void setNameValue (char *pcBuf, char *pcSeg, char *pcName, CvRect rect, int iMaxFileLen);

template <typename T> 
void setNameValueT (char *pcBuf, char *pcSeg, char *pcName, T v, int iMaxFileLen);


#endif

