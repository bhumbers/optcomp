#/bin/bash

for f in *.bc
do
  echo "Running LICM test: $f"
  opt -load ./loop-invariant-code-motion.so -loop-simplify -cd-licm $f -o /dev/null 
done
