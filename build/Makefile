# ==================== 编译器配置 ====================
CXX := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -g -O0 -fPIC
LDFLAGS := -pthread -ludev

# ==================== 路径配置 ====================
SMWLIB_DIR := ./lib
BUILD_DIR  := .
OBJ_DIR    := ./obj

# ==================== Include 路径 ====================
INCLUDE := -I. -I../smw
INCLUDE += -I../smw/classfactory
INCLUDE += -I../smw/common
INCLUDE += -I../smw/eventloop
INCLUDE += -I../smw/ini
INCLUDE += -I../smw/logger
INCLUDE += -I../smw/thread
INCLUDE += -I../drivers/IMU -I../drivers/IMU/WT61 -I../drivers/IMU/H30
INCLUDE += -I../drivers/Speed -I../drivers/Speed/BRT38 -I../drivers/Speed/OID-R5008D

# ==================== 源文件收集 ====================
SMW_SRCS   := $(wildcard ../smw/**/*.cpp)
IMU_SRCS   := $(wildcard ../drivers/IMU/WT61/*.cpp) $(wildcard ../drivers/IMU/H30/*.cpp)
SPEED_SRCS := $(wildcard ../drivers/Speed/BRT38/*.cpp) $(wildcard ../drivers/Speed/OID-R5008D/*.cpp)

LIB_SRCS   := $(SMW_SRCS) $(IMU_SRCS) $(SPEED_SRCS)
MAIN_SRCS  := main.cpp
ALL_SRCS   := $(LIB_SRCS) $(MAIN_SRCS)

# 扁平化：所有 .o 统一放到 obj 目录，仅保留文件名
ALL_OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(ALL_SRCS:.cpp=.o)))
LIB_OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(LIB_SRCS:.cpp=.o)))

# ==================== 批量生成编译规则（核心：消除规则混用警告） ====================
# 定义单条编译规则模板
define build_rule
$(OBJ_DIR)/$(notdir $(1:.cpp=.o)): $(1)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $$< -o $$@
endef

# 遍历所有源码，逐条生成独立规则
$(foreach src,$(ALL_SRCS),$(eval $(call build_rule,$(src))))

# ==================== 编译目标 ====================
.PHONY: all lib main clean

all: lib main

# 编译动态库
lib: $(SMWLIB_DIR)/libmysmw.so

$(SMWLIB_DIR)/libmysmw.so: $(LIB_OBJS)
	@mkdir -p $(SMWLIB_DIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(LIB_OBJS) $(LDFLAGS)

# 编译主程序（纯静态链接，不依赖动态库）
main: $(BUILD_DIR)/main.out

$(BUILD_DIR)/main.out: $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(ALL_OBJS) $(LDFLAGS)

# 清理：只删库、可执行文件，保留所有 .o
clean:
	rm -f $(SMWLIB_DIR)/libmysmw.so $(BUILD_DIR)/main.out  $(BUILD_DIR)/obj/*.o