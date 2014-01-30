15-745 Assigment 1
bhumbers,
psuresh

===== FunctionInfo: ===== 
Run tests with the following commands:

cd FunctionInfo
make

clang -O0 -emit-llvm -c loop.c
opt -mem2reg loop.bc -o loop-m2r.bc
opt -load ./FunctionInfo.so -function-info loop-m2r.bc -o loop-opt.bc

clang -O0 -emit-llvm -c ./tests/var_args_test.c
opt -mem2reg var_args_test.bc -o var_args_test-m2r.bc
opt -load ./FunctionInfo.so -function-info var_args_test-m2r.bc -o var_args_test-opt.bc

===== LocalOpts: ===== 

cd LocalOpts
make

clang -O0 -emit-llvm -c loop.c
opt -mem2reg loop.bc -o loop-m2r.bc
opt -load ./LocalOpts.so -some-local-opts loop-m2r.bc -o loop-opt.bc

clang -O0 -emit-llvm -c ./test-inputs/algebraic.c
opt -mem2reg algebraic.bc -o algebraic-m2r.bc
opt -load ./LocalOpts.so -some-local-opts algebraic-m2r.bc -o algebraic-opt.bc

clang -O0 -emit-llvm -c ./test-inputs/constfold.c
opt -mem2reg constfold.bc -o constfold-m2r.bc
opt -load ./LocalOpts.so -some-local-opts constfold-m2r.bc -o constfold-opt.bc

clang -O0 -emit-llvm -c ./test-inputs/strength.c
opt -mem2reg strength.bc -o strength-m2r.bc
opt -load ./LocalOpts.so -some-local-opts strength-m2r.bc -o strength-opt.bc