INC_DIR = ./includes
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

CC = clang
CFLAGS = -std=c11 -g -Wall -Wextra -I$(INC_DIR)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.d)
TARGET = $(BIN_DIR)/sclc

TARGET = $(BIN_DIR)/sclc

all: $(TARGET)

install: all
	sudo cp $(BIN_DIR)/sclc /usr/local/bin

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

compile_commands.json:
	bear -- $(MAKE) clean all

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

clean-all: clean
	rm compile_commands.json

-include $(DEPS)

.PHONY: all clean clean-all compile_commands.json
