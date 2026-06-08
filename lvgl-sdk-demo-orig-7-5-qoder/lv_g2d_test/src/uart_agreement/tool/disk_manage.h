#ifndef DISK_MANAGE_H
#define DISK_MANAGE_H
#include <stdio.h>

class DiskManage
{
#define BUF_LEN             (1024 * 4)
#define DEV_PREFIX          "/dev/sd"
#define USB_MNT_DIR         "/media/usb"
#define DIR_DVR             "DVR-HD"
#define DIR_EVT             "EVT-HD"
#define SIZE_MB             (1024 * 1024)

#define DISK_MANAGE_DEBUG   0
#if DISK_MANAGE_DEBUG

#ifndef WIN32
#define PRINT_LOG(fmt, arg...)   printf(fmt, ##arg)
#else
#define PRINT_LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#endif

#else

#ifndef WIN32
#define PRINT_LOG(arg...)
#else
#define PRINT_LOG(fmt, ...)
#endif

#endif

public:
    enum FS_TYPE{
        FS_TYPE_FAT,
        FS_TYPE_EXT4,
    };
    enum FORMAT_RESULT{
        FMT_OK,
        FMT_NO_DEV,
        FMT_UMOUNT_ERR,
        FMT_ERR,
    };
    enum DISK_STATE{
        // 正常
        STATE_OK = 0,
        // 未挂载
        STATE_UNMOUNT,
        // 容量太小
        STATE_SMALL,
        // 多个分区
        STATE_MULTI_PART,
        // 格式不可用
        STATE_INVALID,
        // 其它错误
        STATE_ERR,
    };

    static int Format(FS_TYPE fsType = FS_TYPE_FAT);
    static int GetSpace(long *freeMB, long *totalMB);
    static bool IsPlugin();
    static bool IsMount();
    static int DoUmount();
    static int DiskState();
    static int DoMount(const char* dev = NULL);

private:
    static char m_Dev[64];
    static char m_Dir[64];
    static FS_TYPE m_FsType;

};

#endif // DISK_MANAGE_H
