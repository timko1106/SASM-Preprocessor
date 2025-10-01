CC=gcc
CFLAGS=-Wall -Wextra -g -fsanitize=address
#CFLAGS=-O2 -Wall # -Werror -Wextra -pedantic
TARGET=preprocessor

.PHONY: all clean
all: $(TARGET)

clean:
	rm *.o $(TARGET)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o
preprocessor_stage1.o: common.o preprocessor.h preprocessor_stage_1.c
	$(CC) $(CFLAGS) -c preprocessor_stage_1.c -o preprocessor_stage1.o
preprocessor.o: preprocessor_stage1.o preprocessor.c preprocessor.h
	$(CC) $(CFLAGS) -c preprocessor.c -o preprocessor.o
$(TARGET): common.o preprocessor_stage1.o preprocessor.o
	$(CC) $(CFLAGS) preprocessor.o common.o preprocessor_stage1.o -o $(TARGET)


