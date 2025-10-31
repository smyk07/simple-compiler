##########
# Colors #
##########

GREEN = \033[1;32m
NC = \033[0m

####################
# The SCL Compiler #
####################

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

.DEFAULT_GOAL := sclc

sclc: $(TARGET)
	@echo -e "$(GREEN)[INFO]$(NC) Build Successful"

install: sclc
	@echo -e "$(GREEN)[INSTALL]$(NC) $(TARGET) -> /usr/local/bin"
	@sudo cp $(BIN_DIR)/sclc /usr/local/bin

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo -e "$(GREEN)[LD]$(NC) $@"
	@$(CC) $(OBJS) -o $@ -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo -e "$(GREEN)[CC]$(NC) $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

compile_commands.json:
	@echo -e "$(GREEN)[BEAR]$(NC) Generating compile_commands.json"
	@bear -- $(MAKE) clean sclc

clean-sclc:
	@echo -e "$(GREEN)[CLEAN]$(NC) Removing $(OBJ_DIR) $(BIN_DIR)"
	@rm -frf $(OBJ_DIR) $(BIN_DIR)

####################
# Examples for scl #
####################

SCLC = ./bin/sclc
SCLC_FLAGS = -i ./lib
EXAMPLES_DIR = ./examples
EXAMPLE_SRCS = $(shell find $(EXAMPLES_DIR) -name "*.scl" -type f)
EXAMPLE_BINARIES = $(EXAMPLE_SRCS:.scl=)

examples: sclc | $(EXAMPLE_BINARIES)
	@echo -e "$(GREEN)[INFO]$(NC) Examples Compiled Successfully"

$(EXAMPLES_DIR)/%: $(EXAMPLES_DIR)/%.scl $(TARGET)
	@$(SCLC) $(SCLC_FLAGS) $<

clean-examples:
	@echo -e "$(GREEN)[CLEAN]$(NC) Removing $(EXAMPLE_BINARIES)"
	@find ./examples -type f ! -name "*.scl" -delete

clean-all: clean-sclc clean-examples
	@echo -e "$(GREEN)[CLEAN]$(NC) Removing compile_commands.json"
	@rm compile_commands.json

-include $(DEPS)

.PHONY: all sclc clean-sclc clean-all compile_commands.json install examples clean-examples
