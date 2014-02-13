// 15-745 S14 Assignment 2: reaching-definitions.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////
//Dataflow analysis
class ReachingDefinitionsDataFlow : public DataFlow {

  protected:
    BitVector applyMeet(std::vector<BitVector> meetInputs) {
      BitVector meetResult;

      //TODO

      return meetResult;
    }

    TransferResult applyTransfer(const BitVector& value, DenseMap<Value*, int> domainEntryToValueIdx, BasicBlock* block) {
      TransferResult transfer;

      //TODO
      transfer.baseValue = value;

      return transfer;
    }
};
//////////////////////////////////////////////////////////////////////////////////////////////

class ReachingDefinitions : public FunctionPass {
 public:
  static char ID;

  ReachingDefinitions() : FunctionPass(ID) { }

  virtual bool runOnFunction(Function& F) {
    //TODO: Determine domain
    std::vector<Value*> domain;

    int numVars = domain.size();

    //Determine boundary & interior initial dataflow values
    BitVector boundaryCond(numVars);
    BitVector initInteriorCond(numVars);

    ReachingDefinitionsDataFlow flow;
    DataFlowResult dataflowResult = flow.run(F, domain, DataFlow::FORWARD, boundaryCond, initInteriorCond);

    // Did not modify the incoming Function.
    return false;
  }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesCFG();
  }

 private:
};

char ReachingDefinitions::ID = 0;
RegisterPass<ReachingDefinitions> X("cd-reaching-definitions",
    "15745 ReachingDefinitions");

}
