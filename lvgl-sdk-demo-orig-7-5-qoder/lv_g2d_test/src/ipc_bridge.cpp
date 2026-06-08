/**
 * @file ipc_bridge.cpp
 * @brief cpp-ipc 双通道 C 封装实现
 *
 * 通道 1: "ui_to_biz"  — 本进程作为 sender，发消息给业务进程
 * 通道 2: "biz_to_ui"  — 本进程作为 receiver，收业务进程的消息
 */
#include "ipc_bridge.h"
#include "libipc/ipc.h"

#include <cstdio>
#include <cstring>

/* ---- 两个通道对象 ---- */
static ipc::channel *g_ch_ui_to_biz = nullptr;   /* UI 发送 */
static ipc::channel *g_ch_biz_to_ui = nullptr;   /* UI 接收 */

extern "C" int ipc_bridge_init(void)
{
    if (g_ch_ui_to_biz || g_ch_biz_to_ui) {
        return 0;  /* 已初始化 */
    }

    g_ch_ui_to_biz = new ipc::channel("ui_to_biz", ipc::sender);
    if (!g_ch_ui_to_biz->valid()) {
        printf("[ipc_bridge] 打开发送通道 ui_to_biz 失败\n");
        delete g_ch_ui_to_biz;
        g_ch_ui_to_biz = nullptr;
        return -1;
    }

    g_ch_biz_to_ui = new ipc::channel("biz_to_ui", ipc::receiver);
    if (!g_ch_biz_to_ui->valid()) {
        printf("[ipc_bridge] 打开接收通道 biz_to_ui 失败\n");
        delete g_ch_biz_to_ui;
        g_ch_biz_to_ui = nullptr;
        delete g_ch_ui_to_biz;
        g_ch_ui_to_biz = nullptr;
        return -1;
    }

    printf("[ipc_bridge] 双通道初始化完成\n");
    return 0;
}

extern "C" int ipc_bridge_send_to_biz(const void *data, size_t size)
{
    if (!g_ch_ui_to_biz || !data || size == 0) {
        return -1;
    }
    return g_ch_ui_to_biz->send(data, size) ? 0 : -1;
}

extern "C" int ipc_bridge_send_to_biz_str(const char *str)
{
    if (!str) return -1;
    return ipc_bridge_send_to_biz(str, strlen(str) + 1);
}

extern "C" int ipc_bridge_try_recv_from_biz(void *out_buf, size_t buf_size)
{
    if (!g_ch_biz_to_ui || !out_buf || buf_size == 0) {
        return -1;
    }

    auto buff = g_ch_biz_to_ui->try_recv();
    if (buff.size() == 0) {
        return 0;  /* 没有消息 */
    }

    size_t copy_len = (buff.size() < buf_size) ? buff.size() : buf_size;
    memcpy(out_buf, buff.data(), copy_len);
    return static_cast<int>(copy_len);
}

extern "C" void ipc_bridge_deinit(void)
{
    if (g_ch_ui_to_biz) {
        g_ch_ui_to_biz->disconnect();
        delete g_ch_ui_to_biz;
        g_ch_ui_to_biz = nullptr;
    }
    if (g_ch_biz_to_ui) {
        g_ch_biz_to_ui->disconnect();
        delete g_ch_biz_to_ui;
        g_ch_biz_to_ui = nullptr;
    }
    printf("[ipc_bridge] 双通道已清理\n");
}
