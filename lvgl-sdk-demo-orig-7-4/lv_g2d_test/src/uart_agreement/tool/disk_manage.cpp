#include <stdio.h>
#include "disk_manage.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#endif

char DiskManage::m_Dev[64] = {0};
char DiskManage::m_Dir[64] = USB_MNT_DIR;
DiskManage::FS_TYPE DiskManage::m_FsType = DiskManage::FS_TYPE_FAT;

int DiskManage::Format(FS_TYPE fsType)
{
#ifndef WIN32
	char cmd[256] = { 0 };
	char dev[64] = { 0 };
	int ret = 0;
	int tryTimes = 3;

    while(tryTimes--) {
        DoUmount();
        usleep(5000);
    }

    if (access(m_Dev, F_OK) == 0) {
        strncpy(dev, m_Dev, sizeof(DEV_PREFIX));
        dev[sizeof(DEV_PREFIX)] = '\0';
    } else {
        for (char c = 'a'; c < 'z'; c++) {
            sprintf(dev, "%s%c", DEV_PREFIX, c);
            if (access(dev, F_OK) == 0) {
                break;
            }
        }
    }

    if (strlen(dev) < sizeof(DEV_PREFIX)) {
        printf("%s Not found disk!\n", __func__);
        return FMT_NO_DEV;
    }

    tryTimes = 3;
    while(tryTimes--) {
        DoUmount();
        usleep(5000);
    }

    sprintf(cmd, "/bin/chmod +x /usr/bv/partition.sh; /usr/bv/partition.sh %s", dev);
    PRINT_LOG("exec %s\n", cmd);
    system(cmd);
    usleep(200000);

    dev[sizeof(DEV_PREFIX)] = '1';
    dev[sizeof(DEV_PREFIX)+1] = '\0';

    if (fsType == FS_TYPE_EXT4) {
        snprintf(cmd, sizeof(cmd), "mkfs.ext4 -F %s", dev);
    } else {
        snprintf(cmd, sizeof(cmd), "mkfs.vfat -F 32 %s", dev);
    }
    PRINT_LOG("exec %s\n", cmd);
    DoUmount();
    ret = system(cmd);
    if (ret) {
        return FMT_ERR;
    }

    ret = DoMount(dev);
    if (ret < 0) {
        return FMT_ERR;
    }

    m_FsType = fsType;
    strcpy(m_Dev, dev);     // save the device

    return FMT_OK;
#else
    return time(NULL)/2;
#endif
}

int DiskManage::GetSpace(long *freeMB, long *totalMB)
{
    *freeMB = 0;
    *totalMB = 0;

    if (freeMB == NULL || totalMB == NULL) {
        return -2;
    }
#ifndef WIN32
    struct statfs diskInfo;

    if ((!IsMount()) || (STATE_OK != DiskState())) {
        printf("disk is not mount or invalid!\n");
        return -1;
    }

    if (statfs(m_Dir, &diskInfo) == 0) {
        *totalMB = diskInfo.f_bsize * diskInfo.f_blocks / SIZE_MB;
        *freeMB  = diskInfo.f_bsize * diskInfo.f_bavail / SIZE_MB;
        PRINT_LOG("free/total= %ldMB/%ldMB\n", *freeMB, *totalMB);
    } else {
        perror("statfs");
    }
#else
    *freeMB = 1024 * 32;
    *totalMB = 1024 * 64;
#endif
    return 0;
}

bool DiskManage::IsPlugin()
{
#ifndef WIN32
    char dev[64] = { 0 };
    for (char c = 'a'; c < 'z'; c++) {
        sprintf(dev, "%s%c", DEV_PREFIX, c);
        if (access(dev, F_OK) == 0) {
            char dev1[64] = { 0 };
            sprintf(dev1, "%s1", dev);
            if (access(dev1, F_OK) == 0) {
                strcpy(m_Dev, dev1);     // save the device
                PRINT_LOG("m_Dev:%s\n", m_Dev);
            }
            else {
                strcpy(m_Dev, dev);     // save the device
                PRINT_LOG("m_Dev:%s\n", m_Dev);
            }
            return true;
        }
    }
    return false;
#else
    return true;
#endif
}

bool DiskManage::IsMount()
{
#ifndef WIN32
	char mountInfos[BUF_LEN];
	char* dev = NULL;
	char* buf = NULL;
	FILE* fp = NULL;
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        printf("%s fopen(/proc/mounts) failed!\n", __FUNCTION__);
        goto out;
    }

    do {
        memset(mountInfos, 0, sizeof(mountInfos));
        buf = fgets(mountInfos, BUF_LEN, fp);
        if (buf == NULL) {
            fclose(fp);
            goto out;
        }
        if (strlen(mountInfos) > (strlen(m_Dir) + sizeof(DEV_PREFIX))) {
            char *posDev = strstr(mountInfos, DEV_PREFIX);
            char *posDir = strstr(mountInfos, m_Dir);
            if (posDev && posDir) {
                char *posLeft = NULL;
                dev = strtok_r(mountInfos, " \t", &posLeft);
                if (dev) {
                    PRINT_LOG("dev:%s\n", dev);
                    if (access(dev, F_OK) == 0) {
                        fclose(fp);
                        strcpy(m_Dev, dev);     // save the device
                        PRINT_LOG("m_Dev:%s is found\n", m_Dev);
                        return true;
                    } else {
                        PRINT_LOG("dev:%s is not found!\n", dev);
                    }
                }
            }
        }
    } while(buf != NULL);
out:
    memset(m_Dev, 0, sizeof(m_Dev));
    return false;
#else
    return true;
#endif
}

int DiskManage::DoUmount()
{
	int ret1 = 0, ret2 = 0;

    if (/*IsMount()*/ 1) {
#ifndef WIN32
        char cmd[128];
        ret1 = umount2(m_Dir, MNT_FORCE);
        if (ret1) {
            perror("umount");
        }

        sprintf(cmd, "umount -l %s", m_Dir);
        PRINT_LOG("exec %s\n", cmd);
        ret2 = system(cmd);
//        if (!ret2) {
//            printf("exec %s failed!\n", cmd);
//        }
#endif
    }
    return ((ret1 && ret2)? -STATE_ERR : STATE_OK);
}

int DiskManage::DoMount(const char *dev)
{
#ifndef WIN32
	int ret = 0;
	char cmd[256] = { 0 };
    if (dev && strlen(dev) >= sizeof(DEV_PREFIX)) {
        if (access(dev, F_OK) != 0) {
            perror(dev);
            return -STATE_ERR;
        }
        snprintf(cmd, sizeof(cmd), "mount %s %s", dev, m_Dir);
    } else if (IsPlugin() && strlen(m_Dev) >= sizeof(DEV_PREFIX)){
        if (access(m_Dev, F_OK) != 0) {
            perror(m_Dev);
            return -STATE_ERR;
        }
        snprintf(cmd, sizeof(cmd), "mount %s %s", m_Dev, m_Dir);
    }

    // DoUmount();

    PRINT_LOG("exec cmd: %s\n", cmd);
    ret = system(cmd);
    if (ret < 0) {
        perror("system");
        return -STATE_ERR;
    }
#endif
    return STATE_OK;
}

int DiskManage::DiskState()
{
#ifndef WIN32
	long totalMB = 0;
	char partInfo[BUF_LEN];
	int partFd, ret;
	char dev[64];
	int partCnt = 0;
	char testFile[64], testBuf[16];
    struct statfs diskInfo;

    if (!IsMount()) {
        PRINT_LOG("Disk is not mount\n");
        return STATE_UNMOUNT;
    }
    if (strlen(m_Dev) < sizeof(DEV_PREFIX)) {
        PRINT_LOG("Disk device is invalid\n");
        return -STATE_ERR;
    }

    if (statfs(m_Dir, &diskInfo) == 0) {
        totalMB = diskInfo.f_bsize * diskInfo.f_blocks / SIZE_MB;
        PRINT_LOG("total = %ldMB\n", totalMB);
        if (totalMB < 1024) {
            return -STATE_SMALL;
        }
    } else {
        perror("statfs");
        return -STATE_ERR;
    }

    partFd = open("/proc/partitions", O_RDONLY);
    if (partFd <= 0) {
        return -STATE_ERR;
    }

    memset(partInfo, 0, BUF_LEN);
    ret = read(partFd, partInfo, BUF_LEN-1);
    if (ret >= 0) {
        partInfo[ret] = '\0';
    }
    close(partFd);

    strncpy(dev, (char *)m_Dev + 5, 3);  // get "sdx" from "/dev/sdx"

    for (int i=0; i<9; i++) {
        dev[3] = '0' + i;
        dev[4] = '\0';
        if (strstr(partInfo, dev)) {
            partCnt++;
        }
    }

    PRINT_LOG("Disk partCnt = %d\n", partCnt);
    if (partCnt > 1) {
        printf("more than one partition!\n");
        return -STATE_MULTI_PART;
    }

    sprintf(testFile, "%s/.disk.test.temp.txt", m_Dir, time(NULL));
    if (access(testFile, F_OK) == 0) {
        return STATE_OK;
    }

    // write test.
    FILE *fp = fopen(testFile, "wb+");
    if (fp) {
        for (int i = 0; i < sizeof(testBuf); i++) {
            testBuf[i] = 'a' + i;
        }
        ret = fwrite(testBuf, 1, sizeof(testBuf), fp);
        if (ret != sizeof(testBuf)) {
            perror("fwrite");
            return -STATE_INVALID;
        }
        fflush(fp);
        fclose(fp);
    } else {
        perror("fopen");
        return -STATE_INVALID;
    }

    // read and check test.
    fp = fopen(testFile, "rb");
    if (fp) {
        ret = fread(testBuf, 1, sizeof(testBuf), fp);
        if (ret != sizeof(testBuf)) {
            for (int i = 0; i < sizeof(testBuf); i++) {
                if (testBuf[i] != 'a' + i) {
                    printf("%s, read check error!\n", __func__);
                    fclose(fp);
                    return -STATE_INVALID;
                }
            }
        }
        fclose(fp);
    } else {
        perror("fopen");
        return -STATE_INVALID;
    }

#endif
    return STATE_OK;
}
