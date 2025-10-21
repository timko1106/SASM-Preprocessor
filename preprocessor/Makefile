CC = gcc

PREFIX := ..

BIN := $(PREFIX)/bin
OBJREAL := $(BIN)/obj
OBJDIR := $(OBJREAL)/preprocessor

CFLAGS = -O2 -Wall -Wextra -Werror -pedantic

TARGET := $(BIN)/preprocessor
OBJS := $(OBJDIR)/preprocessor_stage1.o $(OBJDIR)/preprocessor_stage2.o $(OBJDIR)/preprocessor_stage3.o $(OBJDIR)/preprocessor.o

$(shell mkdir -p $(OBJDIR) > /dev/null)

.PHONY: all clean
all: common $(OBJS) $(TARGET)

.PHONY: common
common:
	make -C ../common "CC=${CC}" "CFLAGS=$(CFLAGS)" --no-print-directory

$(OBJDIR)/preprocessor_stage1.o: preprocessor.h preprocessor_stage_1.c
	$(CC) $(CFLAGS) -c preprocessor_stage_1.c -o $(OBJDIR)/preprocessor_stage1.o
$(OBJDIR)/preprocessor_stage2.o: preprocessor.h preprocessor_stage_2.c
	$(CC) $(CFLAGS) -c preprocessor_stage_2.c -o $(OBJDIR)/preprocessor_stage2.o
$(OBJDIR)/preprocessor_stage3.o: preprocessor.h preprocessor_stage_3.c
	$(CC) $(CFLAGS) -c preprocessor_stage_3.c -o $(OBJDIR)/preprocessor_stage3.o
$(OBJDIR)/preprocessor.o: preprocessor.h preprocessor.c $(OBJDIR)/preprocessor_stage1.o $(OBJDIR)/preprocessor_stage2.o $(OBJDIR)/preprocessor_stage3.o
	$(CC) $(CFLAGS) -c preprocessor.c -o $(OBJDIR)/preprocessor.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(OBJREAL)/common/common.o $(OBJREAL)/common/parsing.o $(OBJREAL)/common/linking.o -o $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	rm -r $(OBJDIR)
	make -C ../common clean --no-print-directory
