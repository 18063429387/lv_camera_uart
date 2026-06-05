#longan system
#CC:=/home/username/workspace/lichee/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
#CC:=/home/username/workspace/lichee/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
#tina system
#CC:=/home/username/workspace/tina/prebuilt/gcc/linux-x86/arm/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-muslgnueabi-gcc-6.4.1

#longan system
# 1. 定义工具链路径
TOOLCHAIN_DIR := ~/Downloads/f133/mx-hxx003/tina5.0-f133/out/toolchain/riscv64-glibc-gcc-thead_20200702/bin
CC := $(TOOLCHAIN_DIR)/riscv64-unknown-linux-gnu-gcc

# 2. 定义内核架构头文件路径
KERNEL_DIR := ../../../../../..
ARCH_DIR := $(KERNEL_DIR)/arch/riscv/include

# 3. 编译选项
# 新增了 -D__user="" 来解决报错
# -Wno-unused-variable 等选项用来屏蔽那一大堆无关紧要的警告，让输出更干净
CFLAGS := -I $(ARCH_DIR) -static -Wall -D__user="" \
          -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET := csi_test_mplane

.PHONY: all clean

all: $(TARGET)

$(TARGET): csi_test_mplane.c
	@echo "Compiling for RISC-V..."
	$(CC) $(CFLAGS) $^ -o $@ -lc -lgcc_eh

clean:
	rm -rf $(TARGET)
