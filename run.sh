#!/bin/sh

gcc main.c
./a.out test/test1.c
cat myassembly.s
gcc -m32 myassembly.s -o program
./program
echo $?

rm myassembly.s
rm a.out
rm program
