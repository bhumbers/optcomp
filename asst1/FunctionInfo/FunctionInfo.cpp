// 15-745 S14 Assignment 1: FunctionInfo.cpp
// Group: bovik, bovik2
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"

#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {

class FunctionInfo : public ModulePass {

  // Output the function information to standard out.
  void printFunctionInfo(Module& M) {
    std::cout << "Module " << M.getModuleIdentifier().c_str() << std::endl;
    std::cout << "Name,\tArgs,\tCalls,\tBlocks,\tInsns\n";
  
    //Iterate over all functions and collect info for each
    for (Module::iterator modIter = M.begin(); modIter != M.end(); ++modIter) {
      Function* targetFunc = modIter;
      
      std::string func_name = targetFunc->getName();
      bool is_var_arg = targetFunc->isVarArg();
      size_t arg_count = targetFunc->arg_size();
      size_t callsite_count = 0;
      size_t block_count = targetFunc->getBasicBlockList().size();
      size_t instruction_count = 0;
      
      //Count all intructions in basic blocks for this func
      for (Function::iterator funIter = modIter->begin(); funIter != modIter->end(); ++funIter) 
        instruction_count += funIter->getInstList().size();
      
      //Iterate through complete module and count number of calls to this function
      int count = 0;
      for (Module::iterator modIter = M.begin(); modIter != M.end(); ++modIter) {
        for (Function::iterator funIter = modIter->begin(); funIter != modIter->end(); ++funIter) {
          for (BasicBlock::iterator bbIter = funIter->begin(); bbIter != funIter->end(); ++bbIter) {
            if (CallInst* callInst = dyn_cast<CallInst>(&*bbIter)) {              
              if (callInst->getCalledFunction() == targetFunc) {
                ++callsite_count;
              }
            }
          }
        }
      }
      
      std::cout << func_name << ",\t";
      if (is_var_arg) {
        std::cout << "*,\t";
      } else {
        std::cout << arg_count << ",\t";
      }
      std::cout << callsite_count << ",\t" << block_count << ",\t" << instruction_count << std::endl;
    }
  }

public:

  static char ID;

  FunctionInfo() : ModulePass(ID) { }

  ~FunctionInfo() { }

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }

  virtual bool runOnFunction(Function &F) {
    // TODO: implement this.
    return false;
  }
  
  virtual bool runOnModule(Module& M) {
    std::cerr << "15745 Function Information Pass\n"; // TODO: remove this.
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
      runOnFunction(*MI);
    }
    printFunctionInfo(M);
    return false;
  }

};

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char FunctionInfo::ID = 0;
RegisterPass<FunctionInfo> X("function-info", "15745: Function Information");

}
