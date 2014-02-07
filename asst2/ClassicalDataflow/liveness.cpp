// 15-745 S14 Assignment 2: liveness.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////
//Dataflow analysis functions
BitVector livenessMeetFunc(std::vector<BitVector> meetInputs) {
  BitVector meetResult;

  //Meet op = union of inputs
  if (!meetInputs.empty()) {
    for (int i = 0; i < meetInputs.size(); i++) {
      if (i == 0)
        meetResult = meetInputs[i];
      else
        meetResult |= meetInputs[i];
    }
  }

  return meetResult;
}

BitVector livenessTransferFunc(BitVector value, BasicBlock* block) {
  //TODO
  return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////

class Liveness : public FunctionPass {
 public:
  static char ID;

  Liveness() : FunctionPass(ID) { }

  virtual bool runOnFunction(Function& F) {
//    int numVars =

    //Determine boundary & interior initial dataflow values
    BitVector boundaryCond;
    BitVector initInteriorCond;

    DataFlow flow(BitVector(), DataFlow::BACKWARD, livenessMeetFunc, livenessTransferFunc, boundaryCond, initInteriorCond);
    DenseMap<BasicBlock*, DataFlowResultForBlock> dataflowResults = flow.run(F);

    //Now, use dataflow results to determine liveness at each program point, inside each block
    for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      DataFlowResultForBlock blockLivenessVals = dataflowResults[basicBlock];

      //Initialize liveness at end of block
      BitVector livenessVals = blockLivenessVals.out;

      //Iterate backward through instructions of the block, updating and outputting liveness of vars as we go
      for (BasicBlock::reverse_iterator instruction = basicBlock->rbegin(); instruction != basicBlock->rend(); ++instruction) {
        //Special treatment for phi functions: Kill RHS and all operands; don't output liveness here (not a "real" instruction)
        if (PHINode* phiInst = dyn_cast<PHINode>(&*instruction)) {
          //TODO
        }
        else {
          //TODO: Remove vars from live set when if this is their defining inst

          //Add vars to live set when used as an operand
          for (Instruction::const_op_iterator operand = instruction->op_begin(), opEnd = instruction->op_end();
               operand != opEnd; ++operand) {
            //TODO
          }
        }

        //TODO: output set of live variables at program point just past this inst (if not a phi inst)
      }
    }

    //flow.ExampleFunctionPrinter(errs(), F);

    // Did not modify the incoming Function.
    return false;
  }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesCFG();
  }

 private:
};



char Liveness::ID = 0;
RegisterPass<Liveness> X("cd-liveness", "15745 Liveness");

}
