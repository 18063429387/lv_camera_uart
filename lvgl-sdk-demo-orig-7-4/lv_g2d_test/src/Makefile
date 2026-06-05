#
# Camera preview + UART agreement test
#
ifeq ($(origin CC),default)
CC := riscv64-unknown-linux-gnu-gcc
endif
ifeq ($(origin CXX),default)
CXX := riscv64-unknown-linux-gnu-g++
endif

BIN = camera_uart_test
OBJ_DIR := obj
BIN_DIR := bin

CFLAGS ?= -O3 -g0 -I. -I../../vin_test -Iref_inc -Iuart_agreement \
	-I../../lv_port_linux -I../../lv_port_linux/lvgl \
	-I../../lv_port_linux/src/lib \
	-I../../tinyxml2/include -I../../tinyxml2/examples \
	-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare
CXXFLAGS ?= $(CFLAGS)

LV_PORT_LINUX_BUILD ?= ../../lv_port_linux/build
LVGL_LIB_DIR ?= $(LV_PORT_LINUX_BUILD)/lvgl/lib
LV_PORT_LIB ?= $(LV_PORT_LINUX_BUILD)/liblvgl_linux.a
LVGL_LIB ?= $(LVGL_LIB_DIR)/liblvgl.a
LVGL_DEMOS_LIB ?= $(LVGL_LIB_DIR)/liblvgl_demos.a
LVGL_EXAMPLES_LIB ?= $(LVGL_LIB_DIR)/liblvgl_examples.a
LVGL_THORVG_LIB ?= $(LVGL_LIB_DIR)/liblvgl_thorvg.a

TINA_TARGET_SYSROOT ?= /home/xiaozhi/Downloads/f133/mx-hxx003/tina5.0-f133/out/f133-mx-hxx/prototype/openwrt/staging_dir/target
export PKG_CONFIG_SYSROOT_DIR := $(TINA_TARGET_SYSROOT)
export PKG_CONFIG_LIBDIR := $(TINA_TARGET_SYSROOT)/usr/lib/pkgconfig:$(TINA_TARGET_SYSROOT)/usr/share/pkgconfig
FREETYPE_LIBS := $(shell pkg-config --libs freetype2 2>/dev/null)
ifeq ($(FREETYPE_LIBS),)
FREETYPE_LIBS := -L$(TINA_TARGET_SYSROOT)/usr/lib -Wl,-rpath-link,$(TINA_TARGET_SYSROOT)/usr/lib -lfreetype
endif

LDFLAGS ?= -lm -lpthread -ldl $(FREETYPE_LIBS)
# --start-group / --end-group 解决 liblvgl ↔ liblvgl_thorvg 循环依赖
LVGL_LINK_LIBS ?= $(LV_PORT_LIB) -Wl,--start-group $(LVGL_DEMOS_LIB) $(LVGL_EXAMPLES_LIB) $(LVGL_LIB) $(LVGL_THORVG_LIB) -Wl,--end-group

MAINSRC := $(wildcard ./*.c)
CPPSRC := $(wildcard ./uart_agreement/*.cpp) \
          $(wildcard ./uart_agreement/control/*.cpp) \
          $(wildcard ./uart_agreement/dqbuffer/*.cpp) \
          $(wildcard ./uart_agreement/tool/*.cpp) \
          ../../tinyxml2/tinyxml2.cpp \
          ../../tinyxml2/examples/bvtext.cpp \
          ../../tinyxml2/examples/bvtext_c.cpp

OBJEXT ?= .o
MAINOBJ := $(addprefix $(OBJ_DIR)/, $(patsubst ./%,%,$(MAINSRC:.c=$(OBJEXT))))
CPPOBJS := $(addprefix $(OBJ_DIR)/, $(patsubst ./%,%,$(CPPSRC:.cpp=$(OBJEXT))))

all: default

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "CXX $<"

default: $(MAINOBJ) $(CPPOBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$(BIN) $(MAINOBJ) $(CPPOBJS) $(LVGL_LINK_LIBS) $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
