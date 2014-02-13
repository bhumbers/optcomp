#/bin/bash

#Compile w/ optimizations
clang -O -mno-sse -emit-llvm -c ./tests/sum.c

#Apply just mem2reg pass to get SSA form (only needed if we compile to -O0 level using clang)
# opt -mem2reg sum.bc -o sum.bc

#Make human-readable
llvm-dis sum.bc
