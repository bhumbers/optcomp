#/bin/bash

#Compile with minimial optimizations
clang -O0 -mno-sse -emit-llvm -c ./tests/sum.c

#Apply just mem2reg pass to get SSA form
opt -mem2reg sum.bc -o sum-m2r.bc

#Make human-readable
llvm-dis sum-m2r.bc
