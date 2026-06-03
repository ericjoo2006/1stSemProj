CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
TARGET  = suneung_rpg
SRC_DIR = src
SRCS    = $(SRC_DIR)/main.c \
          $(SRC_DIR)/player.c \
          $(SRC_DIR)/study.c \
          $(SRC_DIR)/battle.c \
          $(SRC_DIR)/file_io.c \
          $(SRC_DIR)/ui.c \
          $(SRC_DIR)/map.c \
          $(SRC_DIR)/game_loop.c

OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/game.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
