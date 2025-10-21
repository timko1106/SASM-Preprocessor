#!/bin/bash
rm -f *.sasm *.Sasm *\_output.txt *\_output2.txt *.Sasm.short
for filename in *.asm; do
	NAME=$(echo "$filename" | grep -oE "^[^.]+")
	echo $filename $NAME
	./bin/preprocessor "./$filename" "./$NAME.sasm" "./$NAME.Sasm" > ./$NAME\_output.txt
	./bin/assembler "./$NAME.Sasm" "./$NAME.out" > ./$NAME\_output2.txt
	cat "./$NAME.Sasm" | grep -oE "^[^;]+" > "./$NAME.Sasm.short"
done
