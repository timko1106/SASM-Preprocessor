CC := gcc
#CFLAGS=-Wall -Wextra -Werror -pedantic -g -fsanitize=address -DDEBUG
CFLAGS := -O2 -Wall -Werror -Wextra -pedantic -flto# -DDEBUG

BIN := bin
OBJDIR := $(BIN)/obj

all: common preprocessor assembler emulator

.PHONY: common preprocessor assembler emulator
common:
	make -C common "CC=$(CC)" "CFLAGS=$(CFLAGS)" --no-print-directory
preprocessor:
	make -C preprocessor "CC=$(CC)" "CFLAGS=$(CFLAGS)" --no-print-directory
assembler:
	make -C assembler "CC=$(CC)" "CFLAGS=$(CFLAGS)" --no-print-directory
emulator:
	make -C emulator "CC=$(CC)" "CFLAGS=$(CFLAGS)" --no-print-directory

test: all
	./testall.sh

clean:
	make -C common clean --no-print-directory
	make -C preprocessor clean --no-print-directory
	make -C assembler clean --no-print-directory
