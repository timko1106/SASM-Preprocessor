PREFIX := ..

BIN := $(PREFIX)/bin
OBJDIR := $(BIN)/obj/common

OBJS := $(OBJDIR)/common.o $(OBJDIR)/linking.o $(OBJDIR)/parsing.o

$(shell mkdir -p $(OBJDIR) > /dev/null)

.PHONY: all clean
all: $(OBJS)

$(OBJDIR)/common.o: common.h common.c
	$(CC) $(CFLAGS) -c common.c -o $(OBJDIR)/common.o
$(OBJDIR)/linking.o: common.h $(OBJDIR)/common.o linking.c
	$(CC) $(CFLAGS) -c linking.c -o $(OBJDIR)/linking.o
$(OBJDIR)/parsing.o: common.h $(OBJDIR)/common.o parsing.c
	$(CC) $(CFLAGS) -c parsing.c -o $(OBJDIR)/parsing.o

clean:
	rm -f $(OBJS)
	rm -r $(OBJDIR)
