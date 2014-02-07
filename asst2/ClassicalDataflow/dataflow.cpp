// 15-745 S14 Assignment 2: dataflow.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

#include "llvm/Support/CFG.h"

namespace llvm {

DenseMap<BasicBlock*, DataFlow::DataFlowResultForBlock> DataFlow::run(Function& F) {
  DenseMap<BasicBlock*, DataFlow::DataFlowResultForBlock> results;

  //TODO: Would it make sense to use a linear vector of blocks and use "basic block indices" rather
  //than a map of BasicBlock pointers in order to do block references for dataflow?
  //Might be cleaner and/or faster... depends on how efficient DenseMap is.

  bool analysisConverged = false;

  //Set initial val for boundary block (depends on direction of analysis)
  BasicBlock* boundaryBlock = (direction == FORWARD) ? &F.front() : &F.back();
  results[boundaryBlock] = DataFlowResultForBlock(boundaryCond, boundaryCond);

  //Set initial vals for interior blocks
  for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
    if ((BasicBlock*)basicBlock != boundaryBlock)
      results[basicBlock] = DataFlowResultForBlock(initInteriorCond, initInteriorCond);
  }

  //Generate meet input list for each block (depending on direction of analysis)
  DenseMap<BasicBlock*, std::list<BitVector> > meetInputsByBlock;
  for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      std::list<BitVector> meetInputs;
      switch (direction) {
        case FORWARD:
          for (pred_iterator predBlock = pred_begin(basicBlock), E = pred_end(basicBlock); predBlock != E; ++predBlock)
            meetInputs.push_back(results[*predBlock].out);
          break;
        case BACKWARD:
          for (succ_iterator succBlock = succ_begin(basicBlock), E = succ_end(basicBlock); succBlock != E; ++succBlock)
            meetInputs.push_back(results[*succBlock].in);
            break;
      }

      meetInputsByBlock[basicBlock] = meetInputs;
  }

  //Iterate over blocks in function until convergence of output sets for all blocks
  while (!analysisConverged) {
    analysisConverged = true; //assume converged until proven otherwise during this iteration

    for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      DataFlowResultForBlock& blockVals = results[basicBlock];

      //Store old output before applying this analysis pass to the block (depends on analysis dir)
      BitVector& oldPassOut = (direction == FORWARD) ? blockVals.out : blockVals.in;

      //Apply meet operator to generate updated input set for this block
      BitVector& passIn = (direction == FORWARD) ? blockVals.out : blockVals.in;
      passIn = meetFunc(meetInputsByBlock[basicBlock]);

      //Apply transfer function to input set in order to get output set for this iteration
      BitVector& passOut = (direction == FORWARD) ? blockVals.out : blockVals.in;
      passOut = transferFunc(passIn, basicBlock);

      //Update convergence: if the output set for this block has changed, then we've not converged for this iteration
      if (analysisConverged && passOut != oldPassOut)
        analysisConverged = false;
    }
  }

  return results;
}

void DataFlow::PrintInstructionOps(raw_ostream& O, const Instruction* I) {
  O << "\nOps: {";
  if (I != NULL) {
    for (Instruction::const_op_iterator OI = I->op_begin(), OE = I->op_end();
        OI != OE; ++OI) {
      const Value* v = OI->get();
      v->print(O);
      O << ";";
    }
  }
  O << "}\n";
}

void DataFlow::ExampleFunctionPrinter(raw_ostream& O, const Function& F) {
  for (Function::const_iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    const BasicBlock* block = FI;
    O << block->getName() << ":\n";
    const Value* blockValue = block;
    PrintInstructionOps(O, NULL);
    for (BasicBlock::const_iterator BI = block->begin(), BE = block->end();
        BI != BE; ++BI) {
      BI->print(O);
      PrintInstructionOps(O, &(*BI));
    }
  }
}

}
