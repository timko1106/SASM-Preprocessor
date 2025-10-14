CC=gcc
#CFLAGS=-Wall -Wextra -Werror -pedantic -g -fsanitize=address -DDEBUG
CFLAGS=-O2 -Wall -Werror -Wextra -pedantic # -DDEBUG
TARGET=preprocessor
TARGET2=assembler

.PHONY: all clean
all: $(TARGET) $(TARGET2)

clean:
	rm *.o $(TARGET) $(TARGET2)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o
preprocessor_stage1.o: common.o preprocessor.h preprocessor_stage_1.c
	$(CC) $(CFLAGS) -c preprocessor_stage_1.c -o preprocessor_stage1.o
parsing.o: common.o parsing.c common.h
	$(CC) $(CFLAGS) -c parsing.c -o parsing.o
preprocessor_stage2.o: common.o parsing.o preprocessor.h preprocessor_stage_2.c
	$(CC) $(CFLAGS) -c preprocessor_stage_2.c -o preprocessor_stage2.o
linking.o: common.o linking.c
	$(CC) $(CFLAGS) -c linking.c -o linking.o
preprocessor_stage3.o: common.o linking.o parsing.o preprocessor.h preprocessor_stage_3.c
	$(CC) $(CFLAGS) -c preprocessor_stage_3.c -o preprocessor_stage3.o
preprocessor.o: preprocessor_stage1.o preprocessor.c preprocessor.h preprocessor_stage2.o preprocessor_stage3.o
	$(CC) $(CFLAGS) -c preprocessor.c -o preprocessor.o
assembler.o: common.o parsing.o linking.o common.h assembler.c
	$(CC) $(CFLAGS) -c assembler.c -o assembler.o
$(TARGET): common.o preprocessor_stage1.o preprocessor.o parsing.o preprocessor_stage2.o linking.o preprocessor_stage3.o
	$(CC) $(CFLAGS) preprocessor.o common.o preprocessor_stage1.o parsing.o preprocessor_stage2.o linking.o preprocessor_stage3.o -o $(TARGET)
$(TARGET2): common.o parsing.o linking.o assembler.o
	$(CC) $(CFLAGS) common.o parsing.o linking.o assembler.o -o $(TARGET2)
