SRC=src
BUILD=bin

CFLAGS  += $(shell pkg-config --cflags notcurses)
LDFLAGS += $(shell pkg-config --libs notcurses)

CFLAGS += -lm -I$(SRC)

prereq:
	mkdir -p bin

build: prereq $(SRC)/main.c
	cc $(SRC)/main.c -o $(BUILD)/main $(CFLAGS) $(LDFLAGS)

run: build
	$(BUILD)/main
