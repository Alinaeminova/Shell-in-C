CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include -D_POSIX_C_SOURCE=200809L
LDFLAGS = 
TARGET = myshell

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=build/%.o)

$(shell mkdir -p build)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

build/lexer.o: src/lexer.c include/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

build/parser.o: src/parser.c include/parser.h include/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

build/executor.o: src/executor.c include/executor.h include/parser.h include/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run