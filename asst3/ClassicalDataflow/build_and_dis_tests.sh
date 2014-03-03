#/bin/bash

for f in ./tests/*.c
do
  echo "Building & disassembling file: $f"

  #Compile w/ optimizations
  clang -O0 -mno-sse -emit-llvm -c $f

  fileNoPath=${f##*/}
  bitcodeFile=${fileNoPath%.*}.bc

  #Apply just mem2reg pass to get SSA form (only needed if we compile to -O0 level using clang)
  opt -mem2reg $bitcodeFile -o $bitcodeFile

  #Optional: Disassemble for human-readability: f.bc --> f.ll
  llvm-dis $bitcodeFile
done
