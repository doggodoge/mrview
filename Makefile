CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic $$(pkg-config --cflags libadwaita-1)

TARGET := adwaita-test
SOURCE := \
	main.c \
	alignment.c \
	arena.c \
	static_arena.c \
	files.c \
	string_view.c \
	slices.c \
	arrays.c \
	prs.c \
	pull_request_card.c \
	config.c \
	vendor/yyjson/yyjson.c
LIBS   := $$(pkg-config --libs libadwaita-1) -lcurl

# All .ui files in the project root.
UI_FILES := $(wildcard *.ui)

# e.g. pull_request_card.ui -> ui_data/pull_request_card_ui.h
UI_HEADERS := $(patsubst %.ui,ui_data/%_ui.h,$(UI_FILES))

.PHONY: build run release debug install clean

build: $(UI_HEADERS)
	$(CC) $(CFLAGS) $(SOURCE) $(LIBS) -o $(TARGET)

run: build
	./$(TARGET)

release: $(UI_HEADERS)
	$(CC) $(CFLAGS) -O2 $(SOURCE) $(LIBS) -o $(TARGET)

debug: $(UI_HEADERS)
	$(CC) $(CFLAGS) -O0 -g $(SOURCE) $(LIBS) -o $(TARGET)

install: release
	mkdir -p ~/.local/bin
	cp $(TARGET) ~/.local/bin/$(TARGET)
	chmod +x ~/.local/bin/$(TARGET)

# Create the generated-header directory only when needed.
ui_data:
	mkdir -p ui_data

# e.g. pull_request_card.ui -> ui_data/pull_request_card_ui.h
ui_data/%_ui.h: %.ui | ui_data
	xxd -i -n $*_ui $< > $@

clean:
	rm -rf $(TARGET) $(TARGET).dSYM/ ui_data
