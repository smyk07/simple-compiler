INC_DIR = ./includes
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

CFLAGS = -std=c11 -g -Wall -Wextra -I$(INC_DIR)
CC = clang

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/sclc

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

clean-all: clean 
	rm compile_commands.json

compile_commands.json:
	bear -- $(MAKE) clean all

.PHONY: all clean clean-all compile_commands.json
