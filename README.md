
# Simple C Compiler

This is a small project that toys around
with creating a simple C compiler in C.

It takes simple C Files and generates
x86 assembly code.
This generated assembly file can be assembled
with the GNU-Assembler.

To compile and run the compiler on the test file
in the test directory, just execute
```sh
./run.sh
```

Warning:
for now the generated assembly is not optimized.
Some structs and functions can be parsed (order independent),
but they cannot be used in the main function as
the generated assembly is still not correct.
