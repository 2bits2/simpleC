#!/bin/sh

gcc -ggdb  main.c lex.c parse.c gen.c print.c free.c var.c
./a.out
cat myassembly.s
gcc -m32 myassembly.s -o program
./program
echo $?

rm program
rm myassembly.s
rm a.out
