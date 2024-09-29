# Makefile

CC = gcc
OPTIONS = -std=c11
CFLAGS = -Wall -Iinclude
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/erosDB

all: $(TARGET)

$(TARGET):
	$(CC) $(OPTIONS) $(CFLAGS) $(SRC_DIR)/srv.c -o $(TARGET)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(OPTIONS) $(CFLAGS) -c $< -o $%.o

clean:
	rm -f $(TARGET)

.PHONY: all clean
