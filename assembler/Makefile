PREFIX := ..

BIN := $(PREFIX)/bin
OBJREAL := $(BIN)/obj
OBJDIR := $(OBJREAL)/assembler

OBJS := $(OBJDIR)/assembler.o

TARGET := $(BIN)/assembler

$(shell mkdir -p $(OBJDIR) > /dev/null)

.PHONY: all
all: common $(OBJDIR)/assembler.o $(TARGET)

.PHONY: common
common:
	make -C ../common "CC=${CC}" "CFLAGS=$(CFLAGS)" --no-print-directory

$(OBJDIR)/assembler.o: assembler.c
	$(CC) $(CFLAGS) -c assembler.c -o $(OBJDIR)/assembler.o
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(OBJREAL)/common/common.o $(OBJREAL)/common/linking.o $(OBJREAL)/common/parsing.o -o $(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	rm -r $(OBJDIR)
	make -C ../common clean --no-print-directory
