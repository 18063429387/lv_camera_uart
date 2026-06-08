/**
 * @file ipc_bridge.h
 * @brief cpp-ipc 双通道 C 封装（供 main.c 等 C 代码调用）
 *
 * 通道 1: "ui_to_biz"  — UI 进程发送 → 业务进程接收
 * 通道 2: "biz_to_ui"  — 业务进程发送 → UI 进程接收
 */
#ifndef IPC_BRIDGE_H
#define IPC_BRIDGE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化 IPC 双通道。
 * 在 main() 中调用一次即可。
 * @return 0 成功, -1 失败
 */
int ipc_bridge_init(void);

/**
 * UI → 业务进程：发送数据。
 * @param data  要发送的数据指针
 * @param size  数据字节数
 * @return 0 成功, -1 失败
 */
int ipc_bridge_send_to_biz(const void *data, size_t size);

/**
 * UI → 业务进程：发送字符串（自动包含结尾 '\0'）。
 */
int ipc_bridge_send_to_biz_str(const char *str);

/**
 * 业务进程 → UI：非阻塞尝试接收。
 * @param out_buf   输出缓冲区
 * @param buf_size  缓冲区大小
 * @return >0 实际收到的字节数, 0 没有消息, -1 错误
 */
int ipc_bridge_try_recv_from_biz(void *out_buf, size_t buf_size);

/**
 * 清理 IPC 资源（退出前调用）。
 */
void ipc_bridge_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* IPC_BRIDGE_H */
