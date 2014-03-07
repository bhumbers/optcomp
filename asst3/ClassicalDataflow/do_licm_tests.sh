#/bin/bash

for f in *_main.bc
do
  echo ""
  echo "-----------------------------------------------------------------------------------------------"
  echo "Running LICM benchmark: $f"
  echo ""

  fileNoPath=${f##*/}
  optimizedFile=${fileNoPath%.*}-opt.bc

  echo "Pre-optimization results: "
  lli -stats -force-interpreter $f

  opt -load ./loop-invariant-code-motion.so -loop-simplify -cd-licm $f -o $optimizedFile

  echo "Post-optimization results: "
  lli -stats -force-interpreter $optimizedFile

  #Disassemble optimized bitcode for inspection
  llvm-dis $optimizedFile

  echo ""
  echo "End of LICM benchmark: $f"
  echo "-----------------------------------------------------------------------------------------------"
  echo ""
done
