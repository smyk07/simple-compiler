CC = gcc
CFLAGS = -g -Wall -Wextra

SRC_DIR = ./includes
OBJ_DIR = ./obj

SRCS = main.c $(SRC_DIR)/data_structures.c $(SRC_DIR)/utils.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

TARGET = compiler

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
