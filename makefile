SRC=src
BUILD=bin

prereq:
	mkdir -p bin

build: prereq $(SRC)/main.c
	cc $(SRC)/main.c -o $(BUILD)/main -lnotcurses

run: build
	./main
