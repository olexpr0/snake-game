CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = snake.exe
SRC = snake.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	del /Q $(TARGET) 2>nul || true

.PHONY: all run clean
