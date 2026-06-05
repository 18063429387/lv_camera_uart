#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <map>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <math.h>
#include <array>
#include <cmath>

// #include "core/bvglobal.h"
// #include "core/bv_types.h"
// #include "core/bvpoint.h"
// #include "core/bvrect.h"
#if defined(WIN32) || defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include <fstream>
#include <io.h>
#else
#include <dirent.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <cstdarg>
#endif

namespace util {

    //???path?¦Ì?????§µ?addPath???????????¡¤??
    static std::vector<std::string> GetListFolders(const std::string& path, bool addPath = true)
    {
        std::vector<std::string> list;
        list.clear();
        std::string path_f = path + "/";
// #if WIN32
#if 0
		// ??????
		long long hFile = 0;
		// ??????
		struct _finddata_t fileinfo;

		std::string p;

		if ((hFile = _findfirst(p.assign(path).append("/*").c_str(), &fileinfo)) != -1)
		{
			do {
				// ??????????????
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
				{
					// ??????????¡¤??
					if (addPath)
						list.push_back(p.assign(path).append("/").append(fileinfo.name));
					else
						list.push_back(fileinfo.name);
				}
			} while (_findnext(hFile, &fileinfo) == 0);  //?????????????????0??????-1

			_findclose(hFile);
		}
#else
        DIR* dp;
        struct dirent* dirp;
        if ((dp = opendir(path_f.c_str())) == NULL)
        {
            return list;
        }
        while ((dirp = readdir(dp)) != NULL)
        {
            if (dirp->d_type == DT_DIR &&
                strcmp(dirp->d_name, ".") != 0 &&
                strcmp(dirp->d_name, "..") != 0) {

                if (addPath)
                    list.push_back(path + "/" + static_cast<std::string>(dirp->d_name));
                else
                    list.push_back(static_cast<std::string>(dirp->d_name));
            }
        }
        closedir(dp);
#endif
        return list;
    }

    //path????????
    //exten: ???????
    //addPath?????????????¡¤??
    static std::vector<std::string> GetListFiles(const std::string& path, std::string exten = "", bool addPath = true)
    {
        std::vector<std::string> list;
        list.clear();
        std::string path_f = path + "/";

// #if WIN32
#if 0
        path_f += exten;
        WIN32_FIND_DATAA FindFileData;
        HANDLE hFind;

        hFind = FindFirstFileA((LPCSTR)path_f.c_str(), &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            return list;
        }
        else
        {
            do
            {
                if (FindFileData.dwFileAttributes == FILE_ATTRIBUTE_NORMAL ||
                    FindFileData.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE ||
                    FindFileData.dwFileAttributes == FILE_ATTRIBUTE_HIDDEN ||
                    FindFileData.dwFileAttributes == FILE_ATTRIBUTE_SYSTEM ||
                    FindFileData.dwFileAttributes == FILE_ATTRIBUTE_READONLY)
                {
                    char* fname;
                    fname = FindFileData.cFileName;
                    if (addPath)
                        list.push_back(path + "/" + std::string(fname));
                    else
                        list.push_back(std::string(fname));
                }
            } while (FindNextFileA(hFind, &FindFileData));
            FindClose(hFind);
        }
#else
        DIR* dp;
        struct dirent* dirp;
        if ((dp = opendir(path_f.c_str())) == NULL)
            return list;

        while ((dirp = readdir(dp)) != NULL)
        {
            if (dirp->d_type == DT_REG)
            {
                if (exten.compare("*") == 0)
                {
                    if (addPath)
                        list.push_back(path_f + static_cast<std::string>(dirp->d_name));
                    else
                        list.push_back(static_cast<std::string>(dirp->d_name));
                }
                else
                {
                    if (std::string(dirp->d_name).find(exten) != std::string::npos)
                    {
                        if (addPath)
                            list.push_back(path_f + static_cast<std::string>(dirp->d_name));
                        else
                            list.push_back(static_cast<std::string>(dirp->d_name));
                    }
                }
            }
        }
        closedir(dp);
#endif
        return list;
    }

    inline bool existFold(const std::string& path)
    {
// #if WIN32
#if 0
        WIN32_FIND_DATAA FindFileData;
        HANDLE hFind;

        hFind = FindFirstFileA((LPCSTR)path.c_str(), &FindFileData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            return true;
        }
#else
        DIR* dp = opendir(path.c_str());
        if (dp)
        {
            closedir(dp);
            return true;
        }
#endif
		return false;
	}

	inline bool existFile(const std::string& path)
	{
		FILE* fp = fopen(path.c_str(), "r");
		if (fp)
		{
			fclose(fp);
			return true;
		}

		return false;
	}

	template<class Key, class Value>
	Value FindMapValue(std::map<Key, Value> m, Key key)
	{
		typename std::map<Key, Value>::iterator it = m.begin();
		while (it != m.end())
		{
			if (it->first == key)
				return it->second;
			it++;
		}
		return Value();
	}

    inline void msleep(int msec)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }

    inline void sleep(int s)
    {
        std::this_thread::sleep_for(std::chrono::seconds(s));
    }

    inline long long getCurrentTimeMicro()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline long long getCurrentTimeMsec()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline long long getCurrentTimeSec()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

	inline std::chrono::system_clock::time_point getCurrentTineNow()
	{
		return std::chrono::system_clock::now();
	}
	
	inline int _1_25x(int point)
	{
		return std::round(point * 1.25f);
	}
	
	template<class T>
	inline T _1_25x(T clazz)
	{
		return 1.25f * clazz;
	}
	
	// inline BvRect _1_25x(BvRect rc)
	// {
	// 	return BvRect(_1_25x(BvPoint(rc.x(), rc.y())), _1_25x(BvSize(rc.width(), rc.height())));
	// }

	inline int scale_x(int point,float x=1.167f)
	{
		return std::round(point * x);
	}
	template<class T>
	inline T scale_x(T clazz, float x = 1.167f)
	{
		return x * clazz;
	}

	// inline BvRect scale_x(BvRect rc, float x = 1.167f)
	// {
	// 	return BvRect(scale_x(BvPoint(rc.x(), rc.y()), x), scale_x(BvSize(rc.width(), rc.height()),x));
	// }

    //empty:?????????????? ,, ?????
    inline std::vector<std::string> splitString(std::string src, std::string separator, bool empty = true)
    {
        std::vector<std::string> vecString;
        if (empty)
        {
            std::string str = src;
            std::string subString;
            std::string::size_type start = 0, index;

            index = str.find_first_of(separator, start);
            do
            {
                if (index != std::string::npos)
                {
                    subString = str.substr(start, index - start);
                    vecString.push_back(subString);
                    start = index + separator.size();
                    index = str.find(separator, start);
                    if (start == std::string::npos)
                        break;
                }
            } while (index != std::string::npos);

            //the last part
            subString = str.substr(start);
            vecString.push_back(subString);
        }
        else {
            const char* delim = separator.c_str();
            char* p = strtok((char*)src.c_str(), delim);
            while (p)
            {
                std::string buffer = p;
                vecString.push_back(buffer);
                p = strtok(nullptr, delim);
            }
        }

        return vecString;
    }

	template<class T>
	T string2number(std::string str, bool* ok = nullptr)
	{
		T value = 0;
		bool bOk = true;
		if (str.empty())
		{
			bOk = false;
		}
		else {
			try
			{
				size_t pos = 0;
				if (std::is_same<int, T>::value)
					value = stoi(str, &pos);
				else if (std::is_same<float, T>::value)
					value = stof(str, &pos);
				else if (std::is_same<double, T>::value)
					value = stod(str, &pos);
				else if (std::is_same<long, T>::value)
					value = stol(str, &pos);
				else if (std::is_same<unsigned long, T>::value)
					value = stoul(str, &pos);
				else if (std::is_same<long long, T>::value)
					value = stoll(str, &pos);
				else if (std::is_same<unsigned long long, T>::value)
					value = stoull(str, &pos);
				if (pos != str.size())
				{
					value = 0;
					bOk = false;
				}
			}
			catch (std::invalid_argument)
			{
				bOk = false;
			}
			catch (std::out_of_range)
			{
				bOk = false;
			}
		}

		if (ok)
			*ok = bOk;

		return value;
	}

    class ElapsedTimer {
    public:
        ElapsedTimer() : t1(int64_t(0x8000000000000000))
        {
        }

        void start()
        {
            t1 = getCurrentTimeMicro();
        }

        int64_t elapsedus()const 
        {
            int64_t elap = getCurrentTimeMicro() - t1;
            return elap ;
        }

        int64_t elapsedms()const
        {
            int64_t elap = getCurrentTimeMicro() - t1;
            return elap / 1000;
        }

        void printfElapsed(int line, const char* func)
        {
            int elap = int(getCurrentTimeMicro() - t1);
            printf("------- line = %d, func = %s, microseconds = %d us, milliseconds = %d ms\n", line, func, elap, elap / 1000);
        }

	private:
        int64_t t1;
    };

	template<class T>
	bool str2Enum(const char** arrNames, int count, const char* str, T& value)
	{
		if (!arrNames || !str || count == 0)
			return false;

		std::string strValue = str;
		transform(strValue.begin(), strValue.end(), strValue.begin(), ::tolower);

		for (int i = 0; i < count; i++)
		{
            std::string strTmp = arrNames[i];
            transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

            if (strTmp == strValue)
            {
                value = static_cast<T>(i);
                return true;
            }
		}
		return false;
	}

	inline int frequency(const std::string& str, const std::string& sub)
	{
		int idx = 0;        //?????????¡À? & ??????????????¡À?
		int cnt = 0;        //??????????
							//s.find(sub, idx)??s??idx?????????sub
		while ((idx = str.find(sub, idx)) != str.npos) {
			idx++;
			cnt++;
		}

		return cnt;
	}

    inline int frequencyChinese(const std::string& str)
    {
        //????ascii??
        int length = str.size();
        const char* buf = str.c_str();

        int count = 0;
        for (int i = 0; i < length; )
        {
            if (!isascii(buf[i]))
            {
                //utf->unicode
                if ((buf[i] & 0xe0) == 0xc0)   ///< 110x-xxxx 10xx-xxxx
                    i += 2;
                else if ((buf[i] & 0xf0) == 0xe0) ///< 1110-xxxx 10xx-xxxx 10xx-xxxx
                    i += 3;
                else if ((buf[i] & 0xf8) == 0xf0)   ///< 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
                    i += 4;
                else	///< 1111-10xx 10xx-xxxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
                    i += 5;
                count++;
            }
            else
                i++;
        }

        return count;
    }

	inline std::string StringFormat(const char* fmt, ...)
	{
        int bsize = 1024;
        char buf[1024] = { 0 };

		va_list args;
		va_start(args, fmt);
		
#ifdef WIN32
        int len = _vsnprintf_s(buf, bsize, _TRUNCATE, fmt, args);
		// ensure null terminating on VS
		if (len >= 0 && len < bsize)
		{
			buf[len] = 0;
		}
#else
        int len = vsnprintf(buf, bsize, fmt, args);
#endif
	    va_end(args);

		//CV_Assert(len >= 0 && "Check format string for errors");
		if (len >= bsize)
		{
            printf("It is too long, length = %d\n", len);
            len = bsize - 1;
		}
		buf[bsize - 1] = 0;
		return std::string(buf, len);
	}

	inline std::u32string UTF8ToUnicode(std::string const& utf8)
	{
		if (utf8.empty())
			return {};

		std::u32string res;

		for (size_t i = 0; i < utf8.size(); )
		{
			auto c = (unsigned char)utf8[i];
			char32_t wideChar = 0;
			if ((c & 0x80) == 0)
			{
				wideChar = c;
				++i;
			}
			else if ((c & 0xE0) == 0xC0)  ///< 110x-xxxx 10xx-xxxx
			{
				if (i + 2 > utf8.size()) break;
				wideChar = (char32_t(c) & 0x3F) << 6;
				wideChar |= (char32_t(utf8[i + 1]) & 0x3F);
				i += 2;
			}
			else if ((c & 0xF0) == 0xE0)  ///< 1110-xxxx 10xx-xxxx 10xx-xxxx
			{
				if (i + 3 > utf8.size()) break;
				wideChar = (char32_t(c) & 0x1F) << 12;
				wideChar |= (char32_t(utf8[i + 1]) & 0x3F) << 6;
				wideChar |= (char32_t(utf8[i + 2]) & 0x3F);
				i += 3;
			}
			else if ((c & 0xF8) == 0xF0)  ///< 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
			{
				if (i + 4 > utf8.size()) break;
				wideChar = (char32_t(c) & 0x0F) << 18;
				wideChar |= (char32_t(utf8[i + 1]) & 0x3F) << 12;
				wideChar |= (char32_t(utf8[i + 2]) & 0x3F) << 6;
				wideChar |= (char32_t(utf8[i + 3]) & 0x3F);
				i += 4;
			}
			else///< 1111-10xx 10xx-xxxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
			{
				if (i + 4 > utf8.size()) break;
				wideChar = (char32_t(c) & 0x07) << 24;
				wideChar |= (char32_t(utf8[i + 1]) & 0x3F) << 18;
				wideChar |= (char32_t(utf8[i + 2]) & 0x3F) << 12;
				wideChar |= (char32_t(utf8[i + 3]) & 0x3F) << 6;
				wideChar |= (char32_t(utf8[i + 4]) & 0x3F);
				i += 4;
			}
			res.push_back(wideChar);
		}

		return res;
	}

	inline char32_t SingleUtf2Unicode(const std::string& str)
	{
		int len = str.size();
		if (len == 0)
			return 0;

		char32_t wideChar = 0;
		auto c = (unsigned char)str[0];
		if ((c & 0x80) == 0)
		{
			wideChar = c;
		}
		else if ((c & 0xE0) == 0xC0)
		{
			if (len < 2) return 0;
			wideChar = (char32_t(c) & 0x3F) << 6;
			wideChar |= (char32_t(str[1]) & 0x3F);
		}
		else if ((c & 0xF0) == 0xE0)
		{
			if (len < 3) return 0;
			wideChar = (char32_t(c) & 0x1F) << 12;
			wideChar |= (char32_t(str[1]) & 0x3F) << 6;
			wideChar |= (char32_t(str[2]) & 0x3F);
		}
		else if ((c & 0xF8) == 0xF0)
		{
			if (len < 4) return 0;
			wideChar = (char32_t(c) & 0x0F) << 18;
			wideChar |= (char32_t(str[1]) & 0x3F) << 12;
			wideChar |= (char32_t(str[2]) & 0x3F) << 6;
			wideChar |= (char32_t(str[3]) & 0x3F);
		}
		else
		{
			if (len < 5) return 0;
			wideChar = (char32_t(c) & 0x07) << 24;
			wideChar |= (char32_t(str[1]) & 0x3F) << 18;
			wideChar |= (char32_t(str[2]) & 0x3F) << 12;
			wideChar |= (char32_t(str[3]) & 0x3F) << 6;
			wideChar |= (char32_t(str[4]) & 0x3F);
		}
		return wideChar;
	}

	inline std::string UnicodeToUTF8(std::u32string const& strRes)
	{
		std::string utf8;
		for (char32_t c : strRes)
		{
			auto i = (uint32_t)c;
			if (i < 0x80)
			{
				utf8.push_back((char)i);
			}
			else if (i < 0x800)
			{
				utf8.push_back((char)(0xc0 | (i >> 6)));
				utf8.push_back((char)(0x80 | (i & 0x3f)));
			}
			else if (i < 0x10000)
			{
				utf8.push_back((char)(0xe0 | (i >> 12)));
				utf8.push_back((char)(0x80 | ((i >> 6) & 0x3f)));
				utf8.push_back((char)(0x80 | (i & 0x3f)));
			}
			else if (i < 0x200000)
			{
				utf8.push_back((char)(0xf0 | (i >> 18)));
				utf8.push_back((char)(0x80 | ((i >> 12) & 0x3f)));
				utf8.push_back((char)(0x80 | ((i >> 6) & 0x3f)));
				utf8.push_back((char)(0x80 | (i & 0x3f)));
			}
			else
			{
				utf8.push_back((char)(0xf8 | (i >> 24)));
				utf8.push_back((char)(0x80 | ((i >> 18) & 0x3f)));
				utf8.push_back((char)(0x80 | ((i >> 12) & 0x3f)));
				utf8.push_back((char)(0x80 | ((i >> 6) & 0x3f)));
				utf8.push_back((char)(0x80 | (i & 0x3f)));
			}
		}
		return utf8;
	}
}
