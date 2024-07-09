#!/bin/sh

# Note: typically you would want to use
# a Makefile for the build process
# but currently there are not files so the
# compilation speed is quite good

# compile the compiler A
gcc -ggdb  main.c lex.c var.c parser.c file.c str.c dep.c table.c gen.c

# run the generated compiler A
# with a test file
./a.out ./test/test1.c

# compile the generated x68-64
# assembly code
gcc -m64 myassembly.s -o program

# run the machine code
# and show the result
./program
echo $?

# cleanup
rm program
