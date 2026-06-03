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
	-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare
CXXFLAGS ?= $(CFLAGS)

LV_PORT_LINUX_BUILD ?= ../../lv_port_linux/build
LVGL_LIB_DIR ?= $(LV_PORT_LINUX_BUILD)/lvgl/lib
LV_PORT_LIB ?= $(LV_PORT_LINUX_BUILD)/liblvgl_linux.a
LVGL_LIB ?= $(LVGL_LIB_DIR)/liblvgl.a

LDFLAGS ?= -lm -lpthread -ldl
LVGL_LINK_LIBS ?= $(LV_PORT_LIB) $(LVGL_LIB)

MAINSRC := $(wildcard ./*.c)
CPPSRC := $(wildcard ./uart_agreement/*.cpp) \
          $(wildcard ./uart_agreement/control/*.cpp) \
          $(wildcard ./uart_agreement/dqbuffer/*.cpp) \
          $(wildcard ./uart_agreement/tool/*.cpp)

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
