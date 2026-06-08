#ifndef __CMYFILE_H
#define __CMYFILE_H
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <stdlib.h>
#include <unistd.h>
#endif

class CMyFile
{
public:
	CMyFile()
	{
		m_pcFile = NULL;
		m_pf = NULL;
	}

	~CMyFile()
	{
		close();
	}

	CMyFile(const char* pcFile, const char* mode = "rb")
		: CMyFile()
	{
		open(pcFile, mode);
	}

	int open(const char* pcFile, const char* mode = "rb")
	{
		close();

		if (pcFile)
		{
			m_pcFile = new char[strlen(pcFile) + 1]();
			strcpy(m_pcFile, pcFile);
		}

		m_pf = fopen(m_pcFile, mode);

		if (!m_pf)
			return -1;

		return 0;
	}

	int read(void* pvData, int iLen) {
		if (!m_pf) {
			printf("<WARNING> %s: %s - file not open.\r\n", __FUNCTION__, m_pcFile);
			return 0;
		}
		if (iLen <= 0) {
			printf("<WARNING> %s: %s - iLen = %d is invalid!\r\n", __FUNCTION__, m_pcFile, iLen);
			return 0;
		}
		return fread(pvData, 1, iLen, m_pf);
	}

	int write(void* pvData, int iLen) {
		if (!m_pf) {
			printf("<WARNING> %s: file not open.\r\n", __FUNCTION__);
			return 0;
		}
		return fwrite(pvData, 1, iLen, m_pf);
	}

	int seek(int base, int iPos) {   //  base: SEEK_SET, SEEK_CUR or SEEK_END
		if (!m_pf) {
			printf("<WARNING> %s: file not open.\r\n", __FUNCTION__);
			return 0;
		}
		return fseek(m_pf, iPos, base);
	}

	int seekBegin(void) {
		if (!m_pf) {
			printf("<WARNING> %s: file not open.\r\n", __FUNCTION__);
			return 0;
		}
		return fseek(m_pf, 0, SEEK_SET);
	}

	int seekEnd(void) {
		if (!m_pf) {
			printf("<WARNING> %s: file not open.\r\n", __FUNCTION__);
			return 0;
		}
		return fseek(m_pf, 0, SEEK_END);
	}

	int getFileSize(void) {
		int iLen = 0;
		int iPos = 0;

		if (!m_pf) {
			printf("<WARNING> %s: %s - file not open.\r\n", __FUNCTION__, m_pcFile);
			return 0;
		}
		iPos = ftell(m_pf);             //  备份位置
		fseek(m_pf, 0, SEEK_END);
		iLen = ftell(m_pf);
		fseek(m_pf, iPos, SEEK_SET);    //  恢复位置
		return iLen;
	}

	void close() {
		if (m_pf)
		{
			fflush(m_pf);
#ifndef WIN32
			if (fsync(fileno(m_pf))) {
				system("sync");
			}
#endif
			fclose(m_pf);
		}
		if (m_pcFile)
			delete[] m_pcFile;

		m_pcFile = nullptr;
		m_pf = nullptr;
	}

	int IsOpen(void) {
		return (m_pf != NULL);
	}

private:
	char* m_pcFile;
	FILE* m_pf;
};

#endif // !__CMYFILE_H
