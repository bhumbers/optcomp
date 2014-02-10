// 15-745 S14 Assignment 2: liveness.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "dataflow.h"

using namespace llvm;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////
//Dataflow analysis
class LivenessDataFlow : public DataFlow {

  protected:
    BitVector applyMeet(std::vector<BitVector> meetInputs) {
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

    TransferResult applyTransfer(const BitVector& value, DenseMap<Value*, int> domainEntryToValueIdx, BasicBlock* block) {
      TransferResult transfer;

      //First, calculate set of locally exposed uses and set of defined variables in this block
      int domainSize = domainEntryToValueIdx.size();
      BitVector defSet(domainSize);
      BitVector useSet(domainSize);
      for (BasicBlock::iterator instruction = block->begin(); instruction != block->end(); ++instruction) {
        //Locally exposed uses
        //Phi node handling: Add operands to predecessor-specific value set
        if (PHINode* phiNode = dyn_cast<PHINode>(&*instruction)) {
          for (int incomingIdx = 0; incomingIdx < phiNode->getNumIncomingValues(); incomingIdx++) {
            Value* val = phiNode->getIncomingValue(incomingIdx);
            if (isa<Instruction>(val) || isa<Argument>(val)) {
              int valIdx = domainEntryToValueIdx[val];

              BasicBlock* incomingBlock = phiNode->getIncomingBlock(incomingIdx);
              if (transfer.predSpecificValues.find(incomingBlock) == transfer.predSpecificValues.end())
                transfer.predSpecificValues[incomingBlock] = BitVector(domainSize);
              transfer.predSpecificValues[incomingBlock].set(valIdx);
            }
          }
        }
        //Non-phi node handling: Add operands to general use set
        else {
          User::op_iterator operand, opEnd;
          for (operand = instruction->op_begin(), opEnd = instruction->op_end(); operand != opEnd; ++operand) {
            Value* val = *operand;
            if (isa<Instruction>(val) || isa<Argument>(val)) {
              int valIdx = domainEntryToValueIdx[val];

              //Only locally exposed use if not defined earlier in this block
              if (!defSet[valIdx])
                useSet.set(valIdx);
            }
          }
        }

        //Definitions
        DenseMap<Value*, int>::const_iterator iter = domainEntryToValueIdx.find(instruction);
        if (iter != domainEntryToValueIdx.end())
          defSet.set((*iter).second);
      }

      //Then, apply liveness transfer function: Y = UseSet \union (X - DefSet)
      transfer.baseValue = defSet;
      errs() << bitVectorToString(transfer.baseValue) << "\n";
      transfer.baseValue.flip();
      errs() << bitVectorToString(transfer.baseValue) << "\n";
      transfer.baseValue &= value;
      errs() << bitVectorToString(transfer.baseValue) << "\n";
      transfer.baseValue |= useSet;
      errs() << bitVectorToString(transfer.baseValue) << "\n";

      return transfer;
    }
};
//////////////////////////////////////////////////////////////////////////////////////////////

class Liveness : public FunctionPass {
 public:
  static char ID;

  Liveness() : FunctionPass(ID) { }

  virtual bool runOnFunction(Function& F) {
    //Set domain = variables in the function
    std::vector<Value*> domain;
    for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg)
      domain.push_back(arg);
    for (inst_iterator instruction = inst_begin(F), e = inst_end(F); instruction != e; ++instruction) {
      //If instruction has a nonempty name, then it defines a variable for our domain
      //(TODO: I'm not really sure how correct this is at all... is there a better way to identify "definining" instructions?)
      if (!instruction->getName().empty())
        domain.push_back(&*instruction);
    }

    //DEBUG: Print out domain entry names
    errs() << "Domain: ";
    for (int i = 0; i < domain.size(); i++) {
      errs() << domain[i]->getName();
      errs() << ", ";
    }
    errs() << "\n";

    int numVars = domain.size();

    //Determine boundary & interior initial dataflow values
    BitVector boundaryCond(numVars);
    BitVector initInteriorCond(numVars);

    //Get dataflow values at IN and OUT points of each block
    LivenessDataFlow flow;
    DenseMap<BasicBlock*, DataFlowResultForBlock> dataFlowResults = flow.run(F, domain, DataFlow::BACKWARD, boundaryCond, initInteriorCond);

    //DEBUG: Output in/out point values by block
    for (DenseMap<BasicBlock*, DataFlowResultForBlock>::iterator dataFlowResult = dataFlowResults.begin();
         dataFlowResult != dataFlowResults.end();
         ++dataFlowResult) {
      errs() << "Dataflow results for block " << (*dataFlowResult).first->getName() << ":\n";
      errs() << "  In:  " << bitVectorToString((*dataFlowResult).second.in) << "\n";
      errs() << "  Out: " << bitVectorToString((*dataFlowResult).second.out) << "\n";
    }

    //Now, use dataflow results to determine liveness at program points within each block
    for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      DataFlowResultForBlock blockLivenessVals = dataFlowResults[basicBlock];

      //Initialize liveness at end of block
      BitVector livenessVals = blockLivenessVals.out;

      //Iterate backward through instructions of the block, updating and outputting liveness of vars as we go
      for (BasicBlock::reverse_iterator instruction = basicBlock->rbegin(); instruction != basicBlock->rend(); ++instruction) {
        //Special treatment for phi functions: Kill RHS and all operands; don't output liveness here (not a "real" instruction)
        if (PHINode* phiInst = dyn_cast<PHINode>(&*instruction)) {
          //TODO
        }
        else {
          //TODO: Remove vars from live set if this is their defining inst

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
