#/bin/bash

clang -O3 -mno-sse -emit-llvm -c ./tests/sum.c
llvm-dis sum.bc
