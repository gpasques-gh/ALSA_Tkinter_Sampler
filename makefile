# Compiler
CC = gcc

# Binary
TARGET = sampler

# Directories
SRC_DIR = engine/src
INC_DIR = engine/include
BIN_DIR = engine/bin
OBJ_DIR = engine/obj

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Flags
CFLAGS = -Wall -Wextra -O2 -I$(INC_DIR) -I/usr/include/libxml2 -MMD -MP
LDFLAGS = -lasound -lm -lraylib -lxml2 -lX11

CONTROLLER = DEFAULT

ifeq ($(CONTROLLER), AKAI_MPK)
CFLAGS += -DAKAI_MPK_PADS
endif

INPUT=MIDI_INPUT
ifeq ($(INPUT), KB_INPUT)
CFLAGS += -DKB_INPUT
endif

# Default
all: $(BIN_DIR)/$(TARGET)

# Link
$(BIN_DIR)/$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if needed
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/$(TARGET)

# Rebuild
re: clean all

run:
	./$(BIN_DIR)/$(TARGET)

-include $(DEPS)
.PHONY: all clean re
