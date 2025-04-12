# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -I./lib -fPIC
LDFLAGS := -L./lib
AR := ar
ARFLAGS := rcs

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
LIB_DIR := lib

# Targets
TARGET_BIN := $(BIN_DIR)/my_program
TARGET_LIB := $(LIB_DIR)/libqueue.a  # Renamed to reflect queue.c

# Sources
MAIN_SOURCE := $(SRC_DIR)/main.c
QUEUE_SOURCE := $(SRC_DIR)/mylib/queue.c

# Objects
MAIN_OBJ := $(OBJ_DIR)/main.o
QUEUE_OBJ := $(OBJ_DIR)/mylib/queue.o

# Default target
all: $(BIN_DIR) $(OBJ_DIR) $(TARGET_BIN)

# Create directories
$(BIN_DIR) $(OBJ_DIR) $(OBJ_DIR)/mylib:
	mkdir -p $@

# Build binary (links main.o + queue.o)
$(TARGET_BIN): $(MAIN_OBJ) $(QUEUE_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile main.c
$(MAIN_OBJ): $(MAIN_SOURCE) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile queue.c
$(QUEUE_OBJ): $(QUEUE_SOURCE) | $(OBJ_DIR)/mylib
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library (optional)
$(TARGET_LIB): $(QUEUE_OBJ) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

# Clean
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* $(LIB_DIR)/*.a

.PHONY: all clean
