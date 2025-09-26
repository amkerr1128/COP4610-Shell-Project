SHELL    := /bin/bash
CC       := gcc
CFLAGS   := -Wall -Wextra -std=gnu99 -O2 -Iinclude
LDFLAGS  :=

SRC_DIR  := src
OBJ_DIR  := obj
BIN_DIR  := bin
TARGET   := $(BIN_DIR)/shell

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all run clean veryclean

all: $(TARGET)

$(TARGET): $(BIN_DIR) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

run: $(TARGET)
	@echo "Launching shell (Ctrl-D to exit)..."
	@exec $(TARGET) </dev/tty >/dev/tty 2>&1

clean:
	@rm -f $(OBJS) $(TARGET)

veryclean: clean
	@rmdir --ignore-fail-on-non-empty $(OBJ_DIR) 2>/dev/null || true
	@rmdir --ignore-fail-on-non-empty $(BIN_DIR) 2>/dev/null || true
