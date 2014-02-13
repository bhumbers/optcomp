#/bin/bash

for f in *.bc
do
  echo "Running liveness test: $f"
  opt -load ./liveness.so -cd-liveness $f -o /dev/null 
done
