all: loop-invariant-code-motion.so

#We'll want to force clang usage rather than g++
CXX=clang

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

dataflow.o: dataflow.cpp dataflow.h

%.so: %.o dataflow.o
	$(CXX) -dylib -flat_namespace -shared -fcolor-diagnostics $^ -o $@

clean:
	rm -f *.o *~ *.so

.PHONY: clean all
