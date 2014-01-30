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

  void applyLocalOptimizations(Module& M) {
    std::cout << "Module " << M.getModuleIdentifier().c_str() << std::endl;

    int numAlgIdentityOpts = 0;
    int numConstFoldOpts = 0;
    int numStrengthRedOpts = 0;

    bool anyOptsApplied = true; //used to track when all possible optimizations are applied
  
    //Keep applying optimizations until no longer possible to do so
    while (anyOptsApplied) {
      int prevNumAlgIdentityOpts = numAlgIdentityOpts;
      int prevNumConstFoldOpts = numConstFoldOpts;
      int prevNumStrengthRedOpts = numStrengthRedOpts;
      
      //Iterate through complete module and optimize
      //NOTE: Most of these optimizations are modeled on the built-in LLVM functionality in ConstantFold.cpp
      int count = 0;
      for (Module::iterator modIter = M.begin(); modIter != M.end(); ++modIter) {
        for (Function::iterator funIter = modIter->begin(); funIter != modIter->end(); ++funIter) {
          for (BasicBlock::iterator bbIter = funIter->begin(); bbIter != funIter->end(); ++bbIter) {
            //If looking at a binary operator, apply one of the opts for this question
            if (BinaryOperator* binInst = dyn_cast<BinaryOperator>(&*bbIter)) {   
                //Extract constant integer operands if able
                ConstantInt* constIntA = dyn_cast<ConstantInt>(binInst->getOperand(0));
                ConstantInt* constIntB = dyn_cast<ConstantInt>(binInst->getOperand(1));

                //If both operands are const ints, we can do constant folding and replace with the expression w/ eval result
                if (constIntA && constIntB) {
                  const APInt& constIntValA = constIntA->getValue();
                  const APInt& constIntValB = constIntB->getValue();

                  ConstantInt* evalConst = 0;
                  switch (binInst->getOpcode()) {
                    case Instruction::Add: evalConst = ConstantInt::get(constIntA->getContext(), constIntValA + constIntValB); break;
                    case Instruction::Sub: evalConst = ConstantInt::get(constIntA->getContext(), constIntValA - constIntValB); break;
                    case Instruction::Mul: evalConst = ConstantInt::get(constIntA->getContext(), constIntValA * constIntValB); break;
                  }
                  if (evalConst) {
                    std::cout << "Const-folded an expression: " << binInst->getName().str() << " " << constIntValA.toString(10, true) << ", " + constIntValB.toString(10, true) << std::endl;
                    ReplaceInstWithValue(binInst->getParent()->getInstList(), bbIter, evalConst);
                    numAlgIdentityOpts++;
                  }
                }
                //Otherwise, if at least one is a constant integer, see what else we can optimize...
                else if (constIntA || constIntB) {
                  ConstantInt* constIntTerm = (constIntA) ? constIntA : constIntB; //grab the const term for this binary op
                  Value* otherTerm = (constIntA) ? binInst->getOperand(1) : binInst->getOperand(0); //grab the other "value" term
                  
                  //Apply any algebraic identity opts
                  switch (binInst->getOpcode()) {
                    case Instruction::Add:
                      // Additive identity: x + 0 = x
                      if (constIntTerm->isZero()) {
                        ReplaceInstWithValue(binInst->getParent()->getInstList(), bbIter, otherTerm);
                        std::cout << "Optimized an instance of additive identity." << std::endl;
                        numAlgIdentityOpts++;
                      }
                      break;
                    case Instruction::Mul:
                      // Multiplicative identity: x * 1 = 1
                      if (constIntTerm->isOne()) {
                        ReplaceInstWithValue(binInst->getParent()->getInstList(), bbIter, otherTerm);
                        std::cout << "Optimized an instance of multiplicative identity." << std::endl;
                        numAlgIdentityOpts++;
                      }
                      break;
                    case Instruction::UDiv:
                    case Instruction::SDiv:
                      //Division identity: x / 1 = x
                      //(Note: not commutative... left term must be variable & right term to be 1)
                      if (constIntB && constIntB->isOne()) {
                        ReplaceInstWithValue(binInst->getParent()->getInstList(), bbIter, binInst->getOperand(0));
                        std::cout << "Optimized an instance of division identity." << std::endl;
                        numAlgIdentityOpts++;
                      }
                      break;
                    }
                  }

                //Handle strength reductions where possible
                if (constIntA || constIntB)
                {
                  ConstantInt* constIntTerm = (constIntA) ? constIntA : constIntB; //grab the const term for this binary op
                  Value* otherTerm = (constIntA) ? binInst->getOperand(1) : binInst->getOperand(0); //grab the other "value" term
                  //Strength Reduction for identifying multiplication by 2 and converting to left shift
                  if (binInst->getOpcode() == Instruction::Mul && constIntTerm->getValue().isPowerOf2())
                  { 
                    //TODO: Handle correct power of two shift
                    const int64_t constIntVal = constIntTerm->getSExtValue();
                    Value* shiftVal = ConstantInt::get(constIntTerm->getType(), log2(constIntVal));

                    std::cout << "About to apply a bitshift: " << constIntVal << ", " << log2(constIntVal) << std::endl;

                    ReplaceInstWithInst(binInst->getParent()->getInstList(),bbIter, BinaryOperator::Create(Instruction::Shl, otherTerm, shiftVal, "shl", (Instruction*)(0)));
                    numStrengthRedOpts++;
                  }
                }
                //TODO: Handle strength reductions
                //TODO: Handle floating point identities & const folding?
            }
          }
        }
      }

      //(Crudely) check whether any optimizations occurred so that we know whether to keep iterating
      anyOptsApplied =  (prevNumAlgIdentityOpts != numAlgIdentityOpts) || 
                        (prevNumConstFoldOpts != numConstFoldOpts) ||
                        (prevNumStrengthRedOpts != numStrengthRedOpts);
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
    std::cerr << "15-745 Local Optimizations Pass\n"; // TODO: remove this.
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
      runOnFunction(*MI);
    }
    applyLocalOptimizations(M);
    return false;
  }

private:


};

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char FunctionInfo::ID = 0;
RegisterPass<FunctionInfo> X("some-local-opts", "15745: Local Optimizations");

}
