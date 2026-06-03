#ifndef __CPARAMFILE_H
#define __CPARAMFILE_H
#include "CMyFile.h"
#include <time.h>
#include <sys/stat.h>
#ifdef WIN32
#include <sys/types.h>  

#else
#include <unistd.h>
#endif

enum filetype {
	filetype_ini = 0,
	filetype_bin,
	filetype_unassign,
	filetype_unknow,
};

class CParamFile : public CMyFile
{
public:
	CParamFile() {
		m_pcBuf = NULL;
	}

	CParamFile(char* pcFileName) {
		m_pcBuf = NULL;
		OpenParamFile(pcFileName);
	}

	~CParamFile() {
		delete[] m_pcBuf;
	}

	int OpenParamFile(char* pcFileName) {
		if (!pcFileName || !pcFileName[0]) {
			printf("<WARNING> %s: invalid filename.\r\n", __FUNCTION__);
			return -1;
		}

		char alterfile[128];
		char suffix[16] = { 0 };

		if (strlen(pcFileName) > 4) {
			sprintf(suffix, "%s", &pcFileName[strlen(pcFileName) - 4]); //  .ini or .bin
		}
		//printf("%s: open file %s...\r\n", __FUNCTION__, pcFileName);

		//  若指定为 .ini 或 .bin 文件，则直接打开该文件，若该文件不存在，则尝试打开另外一种文件
		if (strcmp(suffix, ".ini") == 0 || strcmp(suffix, ".bin") == 0) {
			if (__OpenFile(pcFileName) <= 0) {
				//printf("<WARNING> Open %s failed, change to .ini or .bin file!", pcFileName);
				strcpy(alterfile, pcFileName);
				if (strcmp(suffix, ".ini") == 0) {
					strcpy(&alterfile[strlen(pcFileName) - 4], ".bin");
				}
				else {
					strcpy(&alterfile[strlen(pcFileName) - 4], ".ini");
				}
				if (__OpenFile(alterfile) <= 0) {
					//printf("<WARNING> Open the alter file %s failed!", alterfile);
					return -1;
				}
			}
			return 0;
		}
		else {                    //  未指定 .ini 或 .bin，则采用修改时间为最新的文件
			time_t t_ini = 0;
			time_t t_bin = 0;
			char file_ini[128];
			char file_bin[128];
			sprintf(file_ini, "%s.ini", pcFileName);
			sprintf(file_bin, "%s.bin", pcFileName);
			int iIsIniExist = IsFileExist(file_ini, &t_ini);
			int iIsBinExist = IsFileExist(file_bin, &t_bin);
			if (iIsIniExist == 0 && iIsBinExist == 0) {
				printf("<WARNING> no %s .ini or .bin file!\r\n", pcFileName);
				return -1;
			}
			//  选择最新的文件
			if (t_bin > t_ini) {
				//printf("try using %s\r\n", file_bin);
				if (__OpenFile(file_bin) <= 0) {
					printf("<WARNING> %s, line %d: %s.bin not exist!\r\n", __FILE__, __LINE__, file_bin);
					return -1;
				}
			}
			else {
				//printf("try using %s\r\n", file_ini);
				if (__OpenFile(file_ini) <= 0) {
					printf("<WARNING> %s, line %d: %s.ini not exist!\r\n", __FILE__, __LINE__, file_bin);
					return -1;
				}
			}
			return 0;
		}
		return 0;
	}

	void CloseParamFile(void) {
		if (m_pcBuf) delete[] m_pcBuf;
		m_pcBuf = NULL;
	}

	char* GetFileBuf(void) {
		return m_pcBuf;
	}

	int IsVaild(void) {
		return m_pcBuf != NULL;
	}

	//  写配置文件, 不需要额外打开文件，传入文件名和要写入的数据缓冲区即可, 若文件名为.bin，则会加密，若不带后缀 .ini/.bin ，则自动以加密的 .bin 保存
	int WriteParamToFile(char* pcFileName, char* pcBuf) {
		if (getfiletype(pcFileName) == filetype_ini || getfiletype(pcFileName) == filetype_bin) {
			CMyFile file(pcFileName, "wb+");
			if (file.IsOpen()) {
				if (getfiletype(pcFileName) == filetype_ini) {
					file.write(pcBuf, strlen(pcBuf));
				}
				else {
					int len1 = strlen(pcBuf);
					char* pcBufBin = new char[len1 + 64]();
					for (int tmp = 0; tmp < (len1 + 3) / 4; tmp++) {
						*(unsigned int*)(pcBufBin + tmp * 4) = *(unsigned int*)(pcBuf + tmp * 4) ^ 0xa39d793c;       //  0xa39d793c 是约定的简易密钥
					}
					file.write(pcBufBin, len1);
					delete[] pcBufBin;
				}
				file.close();
			}
			else {
				printf("<WARNING> %s: file open %s failed!\r\n", __FUNCTION__, pcFileName);
				return -1;
			}
		}
		else {        //  按 .bin 文件处理
			printf("%s: %s: not specify .ini or .bin file, now write it as .bin file.\r\n", __FUNCTION__, pcFileName);
			char* pcBinFile = new char[strlen(pcFileName) + 16]();
			sprintf(pcBinFile, "%s.bin", pcFileName);
			CMyFile file_bin(pcBinFile, "wb+");
			if (file_bin.IsOpen()) {
				int len1 = strlen(pcBuf);
				char* pcBufBin = new char[len1 + 64]();
				for (int tmp = 0; tmp < (len1 + 3) / 4; tmp++) {
					*(unsigned int*)(pcBufBin + tmp * 4) = *(unsigned int*)(pcBuf + tmp * 4) ^ 0xa39d793c;       //  0xa39d793c 是约定的简易密钥
				}
				file_bin.write(pcBufBin, len1);
				delete[] pcBufBin;
				file_bin.close();
			}
			else {
				printf("<WARNING> %s: file open %s failed!\r\n", __FUNCTION__, pcFileName);
				return -1;
			}
			delete[] pcBinFile;
		}
#ifndef WIN32
		system("sync");
#endif
		return 0;
	}

	//  删除文件
	void DeleteParamFile(char* pcFileName) {
		if (getfiletype(pcFileName) == filetype_ini || getfiletype(pcFileName) == filetype_bin) {
			if (IsFileExist(pcFileName)) {
				if (remove(pcFileName) < 0) {
					printf("<WARNING> %s: remove %s failed!\r\n", __FUNCTION__, pcFileName);
				}
			}
		}
		else {
			//  将 .ini 和 .bin 均删除
			char* pcIniBinFile = new char[strlen(pcFileName) + 16]();
			sprintf(pcIniBinFile, "%s.ini", pcFileName);
			if (IsFileExist(pcIniBinFile)) {
				remove(pcIniBinFile);
			}
			sprintf(pcIniBinFile, "%s.bin", pcFileName);
			if (IsFileExist(pcIniBinFile)) {
				remove(pcIniBinFile);
			}
		}
#ifndef WIN32
		system("sync");
#endif
	}

private:
	int __OpenFile(char* pcFileName) {
		char suffix[16] = { 0 };
		getsuffix(pcFileName, suffix);
		open(pcFileName, "rb");
		if (IsOpen()) {
			int len = getFileSize();
			if (m_pcBuf) {
				delete[] m_pcBuf;
				m_pcBuf = NULL;
			}
			m_pcBuf = new char[len + 64]();
			read(m_pcBuf, len);
			close();
			//  bin 文件，则认为数据已加密，解密之
			if (strcmp(suffix, ".bin") == 0) {
				for (int tmp = 0; tmp < (len + 3) / 4; tmp++) {
					*(unsigned int*)(m_pcBuf + tmp * 4) = *(unsigned int*)(m_pcBuf + tmp * 4) ^ 0xa39d793c;       //  0xa39d793c 是约定的简易密钥
				}
			}
			m_pcBuf[len] = 0;
			return len;
		}
		else {
			return 0;
		}
		return 0;
	}

	int IsFileExist(char* pcFileName, time_t* pt = NULL) {
		struct stat buf;
		memset(&buf, 0, sizeof(buf));
		if (stat(pcFileName, &buf) < 0) {
			if (pt) *pt = 0;
			return 0;
		}
#ifdef WIN32
		if (S_IFREG & buf.st_mode) {
#else
		if (S_ISREG(buf.st_mode)) {
#endif
			if (pt) *pt = buf.st_mtime;         //  最后修改时间
			return 1;
		}
		else {
			if (pt) *pt = 0;
			return 0;
		}
	}

	void getsuffix(char* pcFileName, char* suffix) {
		suffix[0] = 0;
		if (strlen(pcFileName) > 4) {
			sprintf(suffix, "%s", &pcFileName[strlen(pcFileName) - 4]); //  .ini or .bin
		}
	}

	filetype getfiletype(char* pcFileName) {
		char suffix[16] = { 0 };
		int filetype = 0;
		if (strlen(pcFileName) > 4) {
			sprintf(suffix, "%s", &pcFileName[strlen(pcFileName) - 4]); //  .ini or .bin
			if (strcmp(suffix, ".ini") == 0) {
				return filetype_ini;
			}
			else if (strcmp(suffix, ".bin") == 0) {
				return filetype_bin;
			}
			else {
				return filetype_unassign;
			}
		}
		return filetype_unassign;
	}

private:
	char* m_pcBuf;
};


#endif
