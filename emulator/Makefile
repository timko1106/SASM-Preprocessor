PREFIX := ..

BIN := $(PREFIX)/bin
OBJDIR := $(BIN)/obj/emulator

LDFLAGS := -lncursesw

OBJS := $(OBJDIR)/emulator.o $(OBJDIR)/load.o $(OBJDIR)/executor.o $(OBJDIR)/../common/common.o $(OBJDIR)/../common/linking.o

TARGET := $(BIN)/emulator

$(shell mkdir -p $(OBJDIR) > /dev/null)

.PHONY: all clean
all: common $(OBJS) $(TARGET)

.PHONY: common
common:
	make -C ../common "CC=${CC}" "CFLAGS=$(CFLAGS)" --no-print-directory

$(OBJDIR)/load.o: emulator.h load.c
	$(CC) $(CFLAGS) -c load.c -o $(OBJDIR)/load.o
$(OBJDIR)/emulator.o: emulator.h emulator.c
	$(CC) $(CFLAGS) -c emulator.c -o $(OBJDIR)/emulator.o
$(OBJDIR)/executor.o: emulator.h executor.c
	$(CC) $(CFLAGS) -c executor.c -o $(OBJDIR)/executor.o
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f $(OBJS)
	rm -r $(OBJDIR)
	make -C ../common clean
