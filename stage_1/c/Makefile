CC      = gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -Wno-format-truncation -Isrc
LDLIBS  = -lsqlite3 -lcurl

SRC     = $(wildcard src/*.c)
OBJ     = $(patsubst src/%.c,build/%.o,$(SRC))
TARGET  = build/search_engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

.PHONY: all run clean