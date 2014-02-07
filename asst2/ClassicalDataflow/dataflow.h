// 15-745 S14 Assignment 2: dataflow.h
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_DATAFLOW_H__

#include <stdio.h>

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/Support/CFG.h"

#include <list>

namespace llvm {

/** Used to run generic dataflow analysis passes
Inputs are the necessary pass-specific parts of a dataflow pass. 
Output is semi-lattice results at each in & out point per basic block in the module.
*/
class DataFlow {
  public:
	enum Direction {
		FORWARD,
		BACKWARD
	};

	struct DataFlowResultForBlock {
		BitVector in;
		BitVector out;

    DataFlowResultForBlock();
    DataFlowResultForBlock(BitVector in, BitVector out) : in(in), out(out) {}
	};


    DataFlow( BitVector domain, 
              Direction direction,
              BitVector (*meetFunc)(std::list<BitVector>),
              BitVector (*transferFunc)(BitVector, BasicBlock*),
              BitVector boundaryCond,
              BitVector initInteriorCond
              )
    {
      this->domain = domain;
      this->direction = direction;
      this->meetFunc = meetFunc;
      this->transferFunc = transferFunc;
      this->boundaryCond = boundaryCond;
      this->initInteriorCond = initInteriorCond;
    }

    /** Run anlysis on a given Module or Function & make results available (in/out bitvectors per block)
     * Returns a mapping from basic blocks to the IN and OUT sets for each after analysis converges */
    DenseMap<BasicBlock*, DataFlowResultForBlock> run(Function& F);

    // Prints a representation of F to raw_ostream O.
    void ExampleFunctionPrinter(raw_ostream& O, const Function& F);

  protected:
    BitVector domain;
    Direction direction;
    BitVector (*meetFunc)(std::list<BitVector>);
    BitVector (*transferFunc)(BitVector, BasicBlock*);
    BitVector boundaryCond;
    BitVector initInteriorCond;

    void PrintInstructionOps(raw_ostream& O, const Instruction* I);
};

}

#endif
