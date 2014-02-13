#/bin/bash

for f in *.bc
do
  echo "Running reaching definitions test: $f"
  opt -load ./reaching-definitions.so -cd-reaching-definitions $f -o /dev/null 
done
