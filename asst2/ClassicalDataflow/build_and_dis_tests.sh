#/bin/bash

for f in ./tests/*.c
do
  echo "Building & disassembling file: $f"

  #Compile w/ optimizations
  clang -O -mno-sse -emit-llvm -c $f

  #Apply just mem2reg pass to get SSA form (only needed if we compile to -O0 level using clang)
  # opt -mem2reg sum.bc -o sum.bc

  #Optional: Disassemble for human-readability: f.bc --> f.ll
  fileNoPath=${f##*/}
  llvm-dis ${fileNoPath%.*}.bc
done
