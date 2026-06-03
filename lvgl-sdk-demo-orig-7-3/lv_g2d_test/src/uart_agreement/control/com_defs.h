#ifndef __COM_DEFS_H
#define __COM_DEFS_H

//  按键相关定义（请勿随意改动）
#define BV_KEY_HOME             0x01    //0x1001              /*  值定义会不会跟其它的冲突?       */
#define BV_KEY_MENU             0x02    //0x1002              /*  值定义会不会跟其它的冲突?       */
#define BV_KEY_ENTER            0x03    //13
#define BV_KEY_ENTER2           0x83    //  为避免原车确认键导致开屏，故新增此确认键，用于串口通用协议2的确认键
#define BV_KEY_ESC              0x04    //0x1b
#define BV_KEY_RETURN           0x04    //0x1000
#define BV_KEY_UP               0x05    //0x260000
#define BV_KEY_DOWN             0x06    //0x280000
#define BV_KEY_LEFT             0x07    //0x250000
#define BV_KEY_RIGHT            0x08    //0x270000
#define BV_KEY_POWER            0x09
#define BV_KEY_PREVIEW          0x0a
#define BV_KEY_NEXTVIEW         0x0b
#define BV_KEY_VOLDOWN          0x0E
#define BV_KEY_VOLUP            0x0F
#define BV_KEY_EXIT             0x20    // 退出
#define BV_KEY_Physical_Open    0x39    //物理按键开屏

#define BV_KEY_NUM_0            0x30
#define BV_KEY_NUM_1            0x31
#define BV_KEY_NUM_2            0x32
#define BV_KEY_NUM_3            0x33
#define BV_KEY_NUM_4            0x34
#define BV_KEY_NUM_5            0x35
#define BV_KEY_NUM_6            0x36
#define BV_KEY_NUM_7            0x37
#define BV_KEY_NUM_8            0x38
#define BV_KEY_NUM_9            0x39

#endif

