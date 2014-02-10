// 15-745 S14 Assignment 2: dataflow.cpp
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/CFG.h"

namespace llvm {

std::string bitVectorToString(BitVector bv) {
  std::string str(bv.size(), '0');
  for (int i = 0; i < bv.size(); i++)
    str[i] = bv[i] ? '1' : '0';
  return str;
}

DenseMap<BasicBlock*, DataFlowResultForBlock> DataFlow::run(Function& F,
                                                            std::vector<Value*> domain,
                                                            Direction direction,
                                                            BitVector boundaryCond,
                                                            BitVector initInteriorCond) {
  DenseMap<BasicBlock*, DataFlowResultForBlock> results;

  //TODO: Would it make sense to use a linear vector of blocks and use "basic block indices" rather
  //than a map of BasicBlock pointers in order to do block references for dataflow?
  //Might be cleaner and/or faster... depends on how efficient DenseMap is.

  bool analysisConverged = false;

  //Create mapping from domain entries to linear indices
  //(simplifies updating bitvector entries given a particular domain element)
  DenseMap<Value*, int> domainEntryToValueIdx;
  for (int i = 0; i < domain.size(); i++)
    domainEntryToValueIdx[domain[i]] = i;

  //Set initial val for boundary block (depends on direction of analysis)
  BasicBlock* boundaryBlock = (direction == FORWARD) ? &F.front() : &F.back();
  results[boundaryBlock] = DataFlowResultForBlock(boundaryCond, boundaryCond);

  //Set initial vals for interior blocks
  for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
    if ((BasicBlock*)basicBlock != boundaryBlock)
      results[basicBlock] = DataFlowResultForBlock(initInteriorCond, initInteriorCond);
  }

  //Generate meet input list for each block (depending on direction of analysis)
  DenseMap<BasicBlock*, std::vector<BitVector> > meetInputsByBlock;
  for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      std::vector<BitVector> meetInputs;
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

    //TODO: if analysis is backwards, may want to iterate from back-to-front of blocks list
    for (Function::iterator basicBlock = F.begin(); basicBlock != F.end(); ++basicBlock) {
      DataFlowResultForBlock& blockVals = results[basicBlock];

      //Store old output before applying this analysis pass to the block (depends on analysis dir)
      BitVector oldPassOut = (direction == FORWARD) ? blockVals.out : blockVals.in;

      //Apply meet operator to generate updated input set for this block
      BitVector& passIn = (direction == FORWARD) ? blockVals.in : blockVals.out;
      std::vector<BitVector> meetInputs = meetInputsByBlock[basicBlock];
      if (!meetInputs.empty())
        passIn = applyMeet(meetInputs);

      //Apply transfer function to input set in order to get output set for this iteration
      BitVector& passOut = (direction == FORWARD) ? blockVals.out : blockVals.in;
      passOut = applyTransfer(passIn, domainEntryToValueIdx, basicBlock);

      //DEBUGGING
      errs() << "Old passOut: " << bitVectorToString(oldPassOut) << "\n";
      errs() << "New passOut: " << bitVectorToString(passOut) << "\n";

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
