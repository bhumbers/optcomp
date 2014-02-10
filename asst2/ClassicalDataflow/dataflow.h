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

#include <vector>

namespace llvm {

//Util to create string representation of given BitVector
std::string bitVectorToString(const BitVector& bv);

/** An intermediate transfer function output entry from a block. In addition to the main value,
 * may include a list of predecessor block-specific transfer values which are appended (unioned)
 * onto the main value for the meet operator input of each predecessor (used to handle SSA phi nodes) */
struct TransferResult {
  BitVector baseValue;
  DenseMap<BasicBlock*, BitVector> predSpecificValues;
};

struct DataFlowResultForBlock {
  //Final output
  BitVector in;
  BitVector out;

  //Intermediate results
  TransferResult currTransferResult;

  DataFlowResultForBlock() {}
  DataFlowResultForBlock(BitVector in, BitVector out) {
    this->in = in;
    this->out = out;
    this->currTransferResult.baseValue = out; //tra
  }
};

/** Base interface for running dataflow analysis passes.
 * Must be subclassed with pass-specific logic in order to be used.
*/
class DataFlow {
  public:
    enum Direction {
      FORWARD,
      BACKWARD
    };

    /** Run this dataflow analysis on function using given parameters.
     * Returns a mapping from basic blocks to the IN and OUT sets for each after analysis converges */
    DenseMap<BasicBlock*, DataFlowResultForBlock> run(Function& F,
                                                      std::vector<Value*> domain,
                                                      Direction direction,
                                                      BitVector boundaryCond,
                                                      BitVector initInteriorCond);

    /** Prints a representation of F to raw_ostream O. */
    void ExampleFunctionPrinter(raw_ostream& O, const Function& F);

    void PrintInstructionOps(raw_ostream& O, const Instruction* I);

  protected:
    /** Meet operator behavior; specific to the subclassing data flow */
    virtual BitVector applyMeet(std::vector<BitVector> meetInputs) = 0;

    /** Transfer function behavior; specific to a subclassing data flow
     * domainEntryToValueIdx provides mapping from domain elements to the linear bitvector index for that element. */
    virtual TransferResult applyTransfer(const BitVector& value, DenseMap<Value*, int> domainEntryToValueIdx, BasicBlock* block) = 0;
};

}

#endif
