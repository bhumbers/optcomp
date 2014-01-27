// 15-745 S14 Assignment 1: LocalOpts.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {

class FunctionInfo : public ModulePass {

  //
  void printFunctionInfo(Module& M) {
    std::cout << "Module " << M.getModuleIdentifier().c_str() << std::endl;

    int numAlgIdentityOpts = 0;
    int numConstFoldOpts = 0;
    int numStrengthRedOpts = 0;
  
    //Iterate over all functions and collect info for each
    for (Module::iterator modIter = M.begin(); modIter != M.end(); ++modIter) {
      Function* targetFunc = modIter;
      
      std::string func_name = targetFunc->getName();
      
      //Iterate through complete module and optimize
      //NOTE: Most of these are modeled on the built-in LLVM functionality in ConstantFold.cpp
      int count = 0;
      for (Module::iterator modIter = M.begin(); modIter != M.end(); ++modIter) {
        for (Function::iterator funIter = modIter->begin(); funIter != modIter->end(); ++funIter) {
          for (BasicBlock::iterator bbIter = funIter->begin(); bbIter != funIter->end(); ++bbIter) {
            //If looking at a binary operator, apply one of the opts for this question
            if (BinaryOperator* binInst = dyn_cast<BinaryOperator>(&*bbIter)) {   
                //Extract constant integer operands if able
                Constant* constA = dyn_cast<Constant>(binInst->getOperand(0));
                Constant* constB = dyn_cast<Constant>(binInst->getOperand(1));

                ConstantInt* constIntA = dyn_cast<ConstantInt>(binInst->getOperand(0));
                ConstantInt* constIntB = dyn_cast<ConstantInt>(binInst->getOperand(1));

                //If both operands are consts, we can do constant folding and replace with the expression w/ eval result
                if (constA && constB) {
                  //TODO
                }
                //Otherwise, if at least once is a const, see what else we can do...
                else if (constA || constB) {
                  if (constIntA || constIntB) {
                    ConstantInt* constIntTerm = (constIntA) ? constIntA : constIntB; //grab the const term for this binary op
                    Value* otherTerm = (constIntA) ? binInst->getOperand(1) : binInst->getOperand(0); //grab the other "value" term
                    
                    //Apply any algebraic identity opts
                    if (binInst->getOpcode() == Instruction::Add && constIntTerm->isZero()) { // x + 0 = x
                      std::cout << "Doing an additive identity opt..." << std::endl;
                      ReplaceInstWithValue(binInst->getParent()->getInstList(), bbIter, otherTerm);
                      numAlgIdentityOpts++;
                    }
                  }
                }

                //TODO: Handle strength reductions
                //TODO: Handle floating point identities & const folding?
            }
          }
        }
      }
    
    }

    std::cout << "Transformations Applied:" << std::endl;
    std::cout << "    Algebraic Identities: " << numAlgIdentityOpts << std::endl;
    std::cout << "    Constant Folding:     " << numConstFoldOpts << std::endl;
    std::cout << "    Strength Reduction:   " << numStrengthRedOpts << std::endl;
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
    std::cerr << "15745 Local Optimizations Pass\n"; // TODO: remove this.
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
RegisterPass<FunctionInfo> X("some-local-opts", "15745: Local Optimizations");

}
