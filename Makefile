CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Iinclude
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TARGET = $(BUILD_DIR)/graw_service
CLI_TARGET = $(BUILD_DIR)/graw_cli

COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.cpp)
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(COMMON_SRCS))

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

all: $(BUILD_DIR) $(TARGET) $(CLI_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/common

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/common/%.o: $(SRC_DIR)/common/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(BUILD_DIR)/main.o $(BUILD_DIR)/discovery.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLI_TARGET): $(BUILD_DIR)/cli.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(CLI_TARGET)

.PHONY: all clean