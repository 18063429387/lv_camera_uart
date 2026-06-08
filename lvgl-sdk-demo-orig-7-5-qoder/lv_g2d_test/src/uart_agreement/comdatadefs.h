#ifndef __COMDATADEFS_H
#define __COMDATADEFS_H
#include <stdio.h>
#include <string.h>
#include "../ref_inc/osport.h"

template <typename T>
T myMin(T a, T b) {
	if (a < b) return a;
	else return b;
}

template <typename T>
T myMax(T a, T b) {
	if (a < b) return b;
	else return a;
}

template <typename T>
T cast(T x, T min, T max) {
	if (x < min) x = min;
	if (x > max) x = max;
	return x;
}

template<class T>
class MyCritialData
{
public:
	MyCritialData() {
		m_Num = (T)0;
	}

	MyCritialData(T tData) {
		m_Num = tData;
	}

	~MyCritialData() {

	}

	void SetNum(T num) {
		m_Mutex.Enter();
		m_Num = num;
		m_Mutex.Exit();
	}

	T GetNum(void) {
		m_Mutex.Enter();
		T num = m_Num;
		m_Mutex.Exit();
		return num;
	}

private:
	CMutex m_Mutex;
	T      m_Num;
};


#endif

