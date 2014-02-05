all: liveness.so reaching-definitions.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

dataflow.o: dataflow.cpp dataflow.h

%.so: %.o dataflow.o
	clang -dylib -flat_namespace -shared $^ -o $@

clean:
	rm -f *.o *~ *.so

.PHONY: clean all
