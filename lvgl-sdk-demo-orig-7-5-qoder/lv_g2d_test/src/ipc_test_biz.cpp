/**
 * @file ipc_test_biz.cpp
 * @brief IPC 双向通信测试 —— 业务进程端
 *
 * 用法：在目标板上运行此程序，同时运行 camera_uart_test（UI端）
 *   - 自动接收 UI 发来的消息并打印
 *   - 每 2 秒向 UI 发送一条测试消息
 *   - 输入 'q' + 回车可退出
 *
 * 编译（在 Linux 构建机上）：
 *   riscv64-unknown-linux-gnu-g++ -std=c++17 -O2 \
 *     -I../../cpp-ipc/include \
 *     ipc_test_biz.cpp \
 *     ../../cpp-ipc/build/lib/libipc.a \
 *     -lpthread -lrt \
 *     -o ../bin/ipc_test_biz
 */
#include "libipc/ipc.h"

#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

static std::atomic<bool> g_running{true};

/* 接收线程：监听 UI 发来的消息 */
static void recv_thread_func()
{
    ipc::channel ch_recv("ui_to_biz", ipc::receiver);
    if (!ch_recv.valid()) {
        printf("[biz] 打开接收通道 ui_to_biz 失败!\n");
        return;
    }
    printf("[biz] 正在监听 ui_to_biz 通道...\n");

    while (g_running) {
        /* 带 100ms 超时的接收 */
        auto buff = ch_recv.recv(100);
        if (buff.size() > 0) {
            const char *msg = static_cast<const char *>(buff.data());
            printf("[biz] 收到 UI 消息 (%zu 字节): %s\n",
                   buff.size(), msg);
        }
    }
}

int main()
{
    printf("===== IPC 双向通信测试（业务进程端） =====\n");
    printf("通道 ui_to_biz : 接收 UI 消息\n");
    printf("通道 biz_to_ui : 发送消息给 UI\n");
    printf("输入 q + 回车退出\n\n");

    /* 启动接收线程 */
    std::thread recv_th(recv_thread_func);

    /* 发送通道 */
    ipc::channel ch_send("biz_to_ui", ipc::sender);
    if (!ch_send.valid()) {
        printf("[biz] 打开发送通道 biz_to_ui 失败!\n");
        g_running = false;
        recv_th.join();
        return 1;
    }

    int seq = 0;
    while (g_running) {
        /* 每 2 秒发一条消息给 UI */
        char msg[128];
        snprintf(msg, sizeof(msg), "biz_msg:seq=%d,temp=%.1f", seq++, 25.0 + seq * 0.1);
        ch_send.send(msg);
        printf("[biz] 发送给 UI: %s\n", msg);

        /* 等待 2 秒，期间检查是否输入了 'q' */
        for (int i = 0; i < 20 && g_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    recv_th.join();
    printf("[biz] 退出\n");
    return 0;
}
