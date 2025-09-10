INC_DIR = ./includes
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

CC = clang
CFLAGS = -std=c23 -g -Wall -Wextra -I$(INC_DIR)

SRCS = $(shell find $(SRC_DIR) -name "*.c" -type f)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.d)
TARGET = $(BIN_DIR)/sclc

TARGET = $(BIN_DIR)/sclc

all: $(TARGET)
	@echo "[INFO] Build Successful"

install: all
	@echo "[INSTALL] $(TARGET) -> /usr/local/bin"
	@sudo cp $(BIN_DIR)/sclc /usr/local/bin

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "[LD] $@"
	@$(CC) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "[CC] $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

compile_commands.json:
	@echo "[BEAR] Generating compile_commands.json"
	@bear -- $(MAKE) clean all

clean:
	@echo "[CLEAN] Removing $(OBJ_DIR) $(BIN_DIR)"
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

clean-all: clean
	@echo "[CLEAN] Removing compile_commands.json"
	@rm compile_commands.json

-include $(DEPS)

.PHONY: all clean clean-all compile_commands.json install
