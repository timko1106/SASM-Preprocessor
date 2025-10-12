#!/bin/bash
for filename in *.asm; do
	echo $filename
	NAME=$(echo "$filename" | grep -oE "^[^.]+")
	./preprocessor "./$filename" "./$NAME.sasm" "./$NAME.Sasm" > "./$NAME_output.txt"
	cat "./$NAME.Sasm" | grep -oE "^[^;]+" > "./$NAME.Sasm.short"
done
