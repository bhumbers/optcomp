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
BitVector livenessMeetFunc(std::list<BitVector> meetInputs) {
  //TODO
  return BitVector();
};

BitVector livenessTransferFunc(BitVector value, BasicBlock* block) {
  //TODO
  return BitVector();
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Liveness : public FunctionPass {
 public:
  static char ID;

  Liveness() : FunctionPass(ID) { }

  virtual bool runOnFunction(Function& F) {
    //TODO: Properly determine boundary & interior initial dataflow values
    BitVector boundaryCond;
    BitVector initInteriorCond;

    DataFlow flow(BitVector(), DataFlow::BACKWARD, livenessMeetFunc, livenessTransferFunc, boundaryCond, initInteriorCond);
    DenseMap<BasicBlock*, DataFlow::DataFlowResultForBlock> dataflowResults = flow.run(F);

    //TODO: Use dataflow results to determine liveness at each program point, inside each block


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
