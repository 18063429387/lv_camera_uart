# cpp-ipc 说明与移植方案

## 一、cpp-ipc 是什么？

**cpp-ipc** (libipc) 是一个基于**共享内存**的高性能跨平台 IPC（进程间通信）库，版本 1.4.1。

核心特性：
- 使用 **lock-free / 轻量级 spin-lock**，底层数据结构为循环数组
- `ipc::route` 支持单写多读，`ipc::channel` 支持多写多读（最多 32 个 receiver）
- 默认广播模式，支持超时，不会长时间忙等
- 除 STL 外无其他依赖，支持 Linux/Windows/FreeBSD，C++17
- Linux 下通过 POSIX 共享内存 (`shm_open`) + futex 实现

**在车载 UI 项目中的用途**：让 LVGL UI 进程与其他进程（传感器采集、控制逻辑等）通过共享内存通道高效通信。

---

## 二、用户方案验证结论

用户提出的思路**基本正确**，仅需两处小修正：

| 项目 | 结论 |
|------|------|
| 用同一 toolchain 单独编 libipc.a | **可行** — cpp-ipc CMake 支持 `LIBIPC_BUILD_SHARED_LIBS=OFF` |
| 库输出路径 `build/lib/libipc.a` | **正确** — target PROPERTIES 覆盖全局设置 |
| riscv64 兼容性（原子操作/futex） | **兼容** — GCC 内置原子操作和 `syscall(SYS_futex)` 在 riscv64 完全支持 |
| `-lrt` 链接 | **需添加** — 当前 Makefile 缺少，`shm_open`/`clock_gettime` 需要 |
| `-std=c++17` | **需添加** — 当前 CXXFLAGS 未指定，cpp-ipc 要求 C++17 |

---

## 三、实施步骤

### Task 1：交叉编译 cpp-ipc 静态库

在 Linux 构建机上执行：

```bash
cd lvgl-sdk-demo-orig-7-5/cpp-ipc

cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=../lv_port_linux/user_cross_compile_setup.cmake \
  -DLIBIPC_BUILD_SHARED_LIBS=OFF \
  -DLIBIPC_BUILD_TESTS=OFF \
  -DLIBIPC_BUILD_DEMOS=OFF

cmake --build build -j$(nproc)
```

产出：`cpp-ipc/build/lib/libipc.a`

### Task 2：修改 `lv_g2d_test/src/Makefile`

**关键文件**：`lv_g2d_test/src/Makefile`

#### 修改 1 — 新增 cpp-ipc 路径变量（第 26 行后插入）

```makefile
CPP_IPC_DIR   ?= ../../cpp-ipc
CPP_IPC_INC   ?= -I$(CPP_IPC_DIR)/include
CPP_IPC_LIB   ?= $(CPP_IPC_DIR)/build/lib/libipc.a
```

#### 修改 2 — CXXFLAGS 加 `-std=c++17` 和头文件路径

```makefile
# 原来第 20 行：CXXFLAGS ?= $(CFLAGS)
CXXFLAGS ?= $(CFLAGS) -std=c++17 $(CPP_IPC_INC)
```

#### 修改 3 — LDFLAGS 加 `-lrt`，链接行加 `$(CPP_IPC_LIB)`

```makefile
# 原来第 36 行：LDFLAGS ?= -lm -lpthread -ldl $(FREETYPE_LIBS)
LDFLAGS ?= -lm -lpthread -lrt -ldl $(FREETYPE_LIBS)

# 原来第 66 行链接行：
default: $(MAINOBJ) $(CPPOBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$(BIN) $(MAINOBJ) $(CPPOBJS) $(LVGL_LINK_LIBS) $(CPP_IPC_LIB) $(LDFLAGS)
```

### Task 3：验证编译

```bash
cd lvgl-sdk-demo-orig-7-5/lv_g2d_test/src
make clean && make
```

---

## 四、使用方式

```cpp
#include "libipc/ipc.h"

// 发送端
ipc::channel sender("my_channel", ipc::sender);
sender.send("hello", 6);

// 接收端
ipc::channel receiver("my_channel", ipc::receiver);
auto buff = receiver.recv();
```

---

## 五、注意事项

- 运行时需确认目标板内核 `CONFIG_FUTEX_PI=y`（OpenWrt 一般默认开启）
- 静态库手动链接不会自动传递 CMake PUBLIC 依赖，必须显式 `-lpthread -lrt`
- `-std=c++17` 对现有 `.cpp` 文件向后兼容，无风险