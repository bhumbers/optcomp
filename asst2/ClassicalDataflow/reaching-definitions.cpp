// 15-745 S14 Assignment 2: reaching-definitions.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

class ReachingDefinitions : public FunctionPass {
 public:
  static char ID;

  ReachingDefinitions() : FunctionPass(ID) { }

  virtual bool runOnFunction(Function& F) {
    ExampleFunctionPrinter(errs(), F);

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
