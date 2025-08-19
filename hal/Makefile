# Makefile for STM32MP157F-DK2 HAL
# ===================================

# Use Buildroot toolchain
BUILDROOT_PATH = ../../STM32/buildroot/output/host
SYSROOT = $(BUILDROOT_PATH)/arm-buildroot-linux-gnueabihf/sysroot

# Cross-compiler and flags
CC = $(BUILDROOT_PATH)/bin/arm-linux-gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 --sysroot=$(SYSROOT)
INCLUDES = -Iinclude
LDFLAGS = --sysroot=$(SYSROOT)

# Directories
SRC_DIR = src
INCLUDE_DIR = include
EXAMPLES_DIR = examples
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
HAL_SOURCES = $(SRC_DIR)/hal.c \
			  $(SRC_DIR)/hal/gpio.c \
			  $(SRC_DIR)/hal/lcd.c \
			  $(SRC_DIR)/hal/touch.c \
			  $(SRC_DIR)/hal/ui_lite.c
HAL_OBJECTS = $(HAL_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Example sources
EXAMPLE_SOURCES = $(EXAMPLES_DIR)/led_test.c \
				  $(EXAMPLES_DIR)/lcd_test.c \
				  $(EXAMPLES_DIR)/touch_test.c
EXAMPLE_OBJECTS = $(EXAMPLE_SOURCES:$(EXAMPLES_DIR)/%.c=$(OBJ_DIR)/examples/%.o)

# Library
HAL_LIB = $(BUILD_DIR)/libhal.a

# Executables
LED_TEST_BIN = $(BIN_DIR)/led_test
LCD_TEST_BIN = $(BIN_DIR)/lcd_test
TOUCH_TEST_BIN = $(BIN_DIR)/touch_test

# Default target
all: directories $(HAL_LIB) examples

# Add target for Buildroot integration
install: $(HAL_LIB)
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	cp $(HAL_LIB) $(DESTDIR)/usr/lib/
	cp -r include/* $(DESTDIR)/usr/include/

# Cross-compile target
cross: CC = $(BUILDROOT_PATH)/bin/arm-linux-gcc
cross: all

# Create directories
directories:
	@mkdir -p $(OBJ_DIR)/hal $(OBJ_DIR)/examples $(BIN_DIR)

# Build HAL library
$(HAL_LIB): $(HAL_OBJECTS)
	@echo "Creating HAL library..."
	ar rcs $@ $^

# Compile HAL source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile example source files
$(OBJ_DIR)/examples/%.o: $(EXAMPLES_DIR)/%.c
	@echo "Compiling example $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build examples
examples: $(LED_TEST_BIN) $(LCD_TEST_BIN) $(TOUCH_TEST_BIN)

# Build LED test executable
$(LED_TEST_BIN): $(OBJ_DIR)/examples/led_test.o $(HAL_LIB)
	@echo "Linking LED test executable..."
	$(CC) $< -L$(BUILD_DIR) -lhal -o $@

# Build LCD test executable
$(LCD_TEST_BIN): $(OBJ_DIR)/examples/lcd_test.o $(HAL_LIB)
	@echo "Linking LCD test executable..."
	$(CC) $< -L$(BUILD_DIR) -lhal -o $@

# Build Touch test executable
$(TOUCH_TEST_BIN): $(OBJ_DIR)/examples/touch_test.o $(HAL_LIB)
	@echo "Linking Touch test executable..."
	$(CC) $< -L$(BUILD_DIR) -lhal -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Debug info
debug:
	@echo "HAL Sources: $(HAL_SOURCES)"
	@echo "HAL Objects: $(HAL_OBJECTS)"
	@echo "Example Sources: $(EXAMPLE_SOURCES)"
	@echo "Example Objects: $(EXAMPLE_OBJECTS)"
	@echo "Executables: $(LED_TEST_BIN) $(LCD_TEST_BIN)"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "Cross-compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

# Help
help:
	@echo "STM32MP157F-DK2 HAL Build System"
	@echo "================================"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build HAL library and examples (default)"
	@echo "  examples   - Build example programs (led_test, lcd_test, touch_test)"
	@echo "  install    - Install library and headers"
	@echo "  cross      - Cross-compile for ARM target"
	@echo "  clean      - Remove all build files"
	@echo "  debug      - Show build variables"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make cross           - Cross-compile for STM32MP157F-DK2"
	@echo "  make clean && make   - Clean build"
	@echo "  ./build/bin/led_test   - Test LED functionality"
	@echo "  ./build/bin/lcd_test   - Test LCD functionality"
	@echo "  ./build/bin/touch_test - Test touch functionality"

.PHONY: all clean debug help directories examples cross install