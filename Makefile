CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic $$(pkg-config --cflags libadwaita-1)
TARGET := adwaita-test
SOURCE := alignment.c arena.c static_arena.c files.c string_view.c slices.c arrays.c prs.c pull_request_card.c vendor/yyjson/yyjson.c main.c
LIBS   := $$(pkg-config --libs libadwaita-1) -lcurl

.PHONY: build run release debug install clean

build:
	$(CC) $(CFLAGS) $(SOURCE) $(LIBS) -o $(TARGET)

run: build
	./$(TARGET)

release:
	$(CC) $(CFLAGS) -O2 $(SOURCE) $(LIBS) -o $(TARGET)

debug:
	$(CC) $(CFLAGS) -O0 -g $(SOURCE) $(LIBS) -o $(TARGET)

install: release
	mkdir -p ~/.local/bin
	cp $(TARGET) ~/.local/bin/$(TARGET)
	chmod +x ~/.local/bin/$(TARGET)

clean:
	rm -rf $(TARGET) $(TARGET).dSYM/
