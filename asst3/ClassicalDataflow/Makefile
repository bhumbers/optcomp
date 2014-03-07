all: loop-invariant-code-motion.so

#We'll want to force clang usage rather than g++
CXX=clang

CXXFLAGS = $(shell llvm-config --cxxflags) -g -O0

dataflow.o: dataflow.cpp dataflow.h
reaching-defs.o: reaching-defs.cpp reaching-defs.h

%.so: %.o dataflow.o reaching-defs.o
	$(CXX) -shared -fcolor-diagnostics $^ -o $@

clean:
	rm -f *.o *~ *.so

.PHONY: clean all
