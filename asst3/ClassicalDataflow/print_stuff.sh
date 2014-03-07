#/bin/bash

#Source code
enscript -DDuplex:true -1 -E -fCourier7 --tabsize=2 -p source.ps dataflow.h dataflow.cpp loop-invariant-code-motion.h loop-invariant-code-motion.cpp reaching-defs.h reaching-defs.cpp
ps2pdf source.ps source.pdf

#Benchmarks & corresponding IR
enscript -DDuplex:true -1 -E  -fCourier7 --tabsize=2 -p basic_main.ps  ./tests/basic_main.c basic_main.ll basic_main-opt.ll
ps2pdf basic_main.ps basic_main.pdf
enscript -DDuplex:true -1 -E -fCourier7 --tabsize=2 -p nested_main.ps ./tests/nested_main.c nested_main.ll nested_main-opt.ll
ps2pdf nested_main.ps nested_main.pdf
enscript -DDuplex:true -1 -E -fCourier7 --tabsize=2 -p double_nested_main.ps ./tests/double_nested_main.c double_nested_main.ll double_nested_main-opt.ll
ps2pdf double_nested_main.ps double_nested_main.pdf

