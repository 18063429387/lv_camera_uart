#ifndef BV_GLOBAL_H
#define BV_GLOBAL_H
#include <stdint.h>
#include <stddef.h>
#include <iostream>

#if defined(WIN32) && !defined(linux)
typedef __int64 qint64;            /* 64 bit signed */
typedef unsigned __int64 quint64;  /* 64 bit unsigned */
#else
typedef long long qint64;           /* 64 bit signed */
typedef unsigned long long quint64; /* 64 bit unsigned */
#endif

typedef unsigned int quint32;      /* 32 bit unsigned */
typedef qint64 qlonglong;
typedef quint64 qulonglong;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef double qreal;

typedef unsigned int QRgb;                        // RGB triplet

#define COUNTOF(arr) (sizeof(arr) / sizeof(0[arr]))
#define COUNTINDEX(arr) (sizeof(arr) / sizeof(0[arr])) - 1

#define DEBUG_PRINTF(...) \
    do{ \
        fprintf(stdout, "[DEBUG]%s %s(Line:%d): ",__FILE__,__FUNCTION__,__LINE__); \
        fprintf(stdout, __VA_ARGS__); \
        fprintf(stdout, "\r\n");    \
    }while(0)

#define BV_WARNING(...) \
    do{ \
        fprintf(stdout, "<WARNING> %s(Line:%d): ",__FUNCTION__,__LINE__); \
        fprintf(stdout, __VA_ARGS__); \
        fprintf(stdout, "\r\n");    \
    }while(0)

#define PRINTF_LINE		printf("------12345678++++++line = %d, file = %s, func = %s\n", __LINE__, __FILE__, __FUNCTION__)

#define Q_MAX_3(a, b, c) (( a > b && a > c) ? a : (b > c ? b : c))
#define Q_MIN_3(a, b, c) (( a < b && a < c) ? a : (b < c ? b : c))

template <typename T>
inline T qAbs(const T& t) { return t >= 0 ? t : -t; }

inline int qRound(double d)
{
	return d >= 0.0 ? int(d + 0.5) : int(d - double(int(d - 1)) + 0.5) + int(d - 1);
}
inline int qRound(float d)
{
	return d >= 0.0f ? int(d + 0.5f) : int(d - float(int(d - 1)) + 0.5f) + int(d - 1);
}

template <typename T>
constexpr inline const T& qMin(const T& a, const T& b) { return (a < b) ? a : b; }
template <typename T>
constexpr inline const T& qMax(const T& a, const T& b) { return (a < b) ? b : a; }
template <typename T>
constexpr inline const T& qBound(const T& min, const T& val, const T& max)
{
	return qMax(min, qMin(max, val));
}

constexpr inline const int& qRingRange(const int& min, const int& val, const int& max)
{
	if (val > max)
		return min;
	if (val < min)
		return max;
	return val;
}

static inline  bool qFuzzyIsNull(double d)
{
	return qAbs(d) <= 0.000000000001;
}

static inline  bool qFuzzyIsNull(float f)
{
	return qAbs(f) <= 0.00001f;
}

inline float AngleToRad(float dgree) {
	return dgree * 3.1415926f / 180.0f;
}

inline float RadToAngle(float rad) {
	return rad * 180.0f / 3.1415926f;
}

inline QRgb qRgb(int r, int g, int b)// set RGB value
{
	return (0xffu << 24) | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
}

inline QRgb qRgba(int r, int g, int b, int a)// set RGBA value
{
	return ((a & 0xffu) << 24) | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
}

inline int qRed(QRgb rgb)                // get red part of RGB
{
	return ((rgb >> 16) & 0xff);
}

inline int qGreen(QRgb rgb)                // get green part of RGB
{
	return ((rgb >> 8) & 0xff);
}

inline int qBlue(QRgb rgb)                // get blue part of RGB
{
	return (rgb & 0xff);
}

inline int qAlpha(QRgb rgb)                // get alpha part of RGBA
{
	return rgb >> 24;
}

inline int qGray(int r, int g, int b)// convert R,G,B to gray 0..255
{
	return (r * 11 + g * 16 + b * 5) / 32;
}

inline int qGray(QRgb rgb)                // convert RGB to gray 0..255
{
	return qGray(qRed(rgb), qGreen(rgb), qBlue(rgb));
}

inline bool qIsGray(QRgb rgb)
{
	return qRed(rgb) == qGreen(rgb) && qRed(rgb) == qBlue(rgb);
}


//字体样式
enum FontType {
	FontType_ExtraLight = 0,		//字重200
	FontType_Light,					//字重300
	FontType_Normal,				//字重400
	FontType_Regular,				//字重400
	FontType_Medium,				//字重500
	FontType_Bold,					//字重700
	FontType_Heavy,					//字重900
};

//字体对齐方式
enum AlignmentFlag {
	AlignAbsolute,		//绝对位置
	AlignLeft,
	AlignRight,
	AlignCenter
};

#endif // BV_GLOBAL_H
