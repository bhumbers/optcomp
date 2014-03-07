////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include <set>
#include <sstream>

#include "dataflow.h"

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/CFG.h"

namespace llvm {

/* Var definition util */
Value* getDefinitionVar(Value* v) {
  // Definitions are assumed to be one of:
  // 1) Function arguments
  // 2) Store instructions (2nd argument is the variable being (re)defined)
  // 3) Instructions that start with "  %" (note the 2x spaces)
  //      Note that this is a pretty brittle and hacky way to catch what seems the most common definition type in LLVM.
  //      Unfortunately, we couldn't figure a better way to catch all definitions otherwise, as cases like
  //      "%0" and "%1" don't show up  when using "getName()" to identify definition instructions.
  //      There's got to be a better way, though...

  if (isa<Argument>(v)) {
    return v;
  }
  else if (isa<StoreInst>(v)) {
    return ((StoreInst*)v)->getPointerOperand();
  }
  else if (isa<Instruction>(v)){
    std::string str = valueToStr(v);
    const int VAR_NAME_START_IDX = 2;
    if (str.length() > VAR_NAME_START_IDX && str.substr(0,VAR_NAME_START_IDX+1) == "  %")
      return v;
  }
  return 0;
}

/******************************************************************************************
 * String output utilities */
std::string bitVectorToStr(const BitVector& bv) {
  std::string str(bv.size(), '0');
  for (int i = 0; i < bv.size(); i++)
    str[i] = bv[i] ? '1' : '0';
  return str;
}

std::string valueToStr(const Value* value) {
  std::string instStr; llvm::raw_string_ostream rso(instStr);
  value->print(rso);
  return instStr;
}

const int VAR_NAME_START_IDX = 2;

std::string valueToDefinitionStr(Value* v) {
  //Verify it's a definition first
  Value* def = getDefinitionVar(v);
  if (def == 0)
    return "";

  std::string str = valueToStr(v);
  if (isa<Argument>(v)) {
    return str;
  }
  else {
      str = str.substr(VAR_NAME_START_IDX);
      return str;
  }

  return "";
}

std::string valueToDefinitionVarStr(Value* v) {
  //Similar to valueToDefinitionStr, but we return just the defined var rather than the whole definition

  Value* def = getDefinitionVar(v);
  if (def == 0)
    return "";

  if (isa<Argument>(def) || isa<StoreInst>(def)) {
    return "%" + def->getName().str();
  }
  else {
    std::string str = valueToStr(def);
    int varNameEndIdx = str.find(' ',VAR_NAME_START_IDX);
    str = str.substr(VAR_NAME_START_IDX,varNameEndIdx-VAR_NAME_START_IDX);
    return str;
  }
}

std::string setToStr(std::vector<Value*> domain, const BitVector& includedInSet, std::string (*valFormatFunc)(Value*)) {
  std::stringstream ss;
  ss << "{\n";
  int numInSet = 0;
  for (int i = 0; i < domain.size(); i++) {
    if (includedInSet[i]) {
      if (numInSet > 0) ss << " \n";
      numInSet++;
      ss << "    " << valFormatFunc(domain[i]);
    }
  }
  ss << "}";
  return ss.str();
}

/* End string output utilities *
******************************************************************************************/


DataFlowResult DataFlow::run(std::vector<llvm::BasicBlock*> blocks,
                              std::vector<Value*> domain,
                              Direction direction,
                              BitVector boundaryCond,
                              BitVector initInteriorCond) {
  DenseMap<BasicBlock*, DataFlowResultForBlock> resultsByBlock;
  bool analysisConverged = false;

  //Create mapping from domain entries to linear indices
  //(simplifies updating bitvector entries given a particular domain element)
  DenseMap<Value*, int> domainEntryToValueIdx;
  for (int i = 0; i < domain.size(); i++)
    domainEntryToValueIdx[domain[i]] = i;

  //Set initial val for boundary blocks, which depend on direction of analysis
  std::set<BasicBlock*> boundaryBlocks;
  switch (direction) {
    case FORWARD:
      //Post-"entry" block assumed to be the first one without a predecessor
      for(std::vector<BasicBlock*>::iterator blockIter = blocks.begin(), E = blocks.end(); blockIter != E; ++blockIter) {
        if (pred_begin(*blockIter) == pred_end(*blockIter)) {
//          errs() << "Inserting fwd boundary block: " << (*blockIter)->getName().str() << "\n";
          boundaryBlocks.insert(*blockIter);
        }
      }
      break;
    case BACKWARD:
      //Pre-"exit" blocks = those that have a return statement
      for(std::vector<BasicBlock*>::iterator blockIter = blocks.begin(), E = blocks.end(); blockIter != E; ++blockIter)
        if (isa<ReturnInst>((*blockIter)->getTerminator()))
          boundaryBlocks.insert((*blockIter));
      break;
  }
  for (std::set<BasicBlock*>::iterator boundaryBlock = boundaryBlocks.begin(); boundaryBlock != boundaryBlocks.end(); boundaryBlock++) {
    DataFlowResultForBlock boundaryResult = DataFlowResultForBlock();
    //Set either the "IN" of post-entry blocks or the "OUT" of pre-exit blocks (since entry/exit blocks don't actually exist...)
    BitVector* boundaryVal = (direction == FORWARD) ? &boundaryResult.in : &boundaryResult.out;
    *boundaryVal = boundaryCond;
    boundaryResult.currTransferResult.baseValue = boundaryCond;
    resultsByBlock[*boundaryBlock] = boundaryResult;
  }

  //Set initial vals for interior blocks (either OUTs for fwd analysis or INs for bwd analysis)
  for (std::vector<BasicBlock*>::iterator blockIter = blocks.begin(); blockIter != blocks.end(); ++blockIter) {
    if (boundaryBlocks.find((*blockIter)) == boundaryBlocks.end()) {
      DataFlowResultForBlock interiorInitResult = DataFlowResultForBlock();
      BitVector* interiorInitVal = (direction == FORWARD) ? &interiorInitResult.out : &interiorInitResult.in;
      *interiorInitVal = initInteriorCond;
      interiorInitResult.currTransferResult.baseValue = initInteriorCond;
      resultsByBlock[*blockIter] = interiorInitResult;
    }
  }

  //Generate analysis "predecessor" list for each block (depending on direction of analysis)
  //Will be used to drive the meet inputs.
  DenseMap<BasicBlock*, std::vector<BasicBlock*> > analysisPredsByBlock;
  for (std::vector<BasicBlock*>::iterator blockIter = blocks.begin(); blockIter != blocks.end(); ++blockIter) {
      std::vector<BasicBlock*> analysisPreds;

//      errs() << "Build predecessor list for : " << (*blockIter)->getName().str() << "\n";

      switch (direction) {
        case FORWARD:
        for (pred_iterator predBlock = pred_begin((*blockIter)), E = pred_end((*blockIter)); predBlock != E; ++predBlock)
            analysisPreds.push_back(*predBlock);
          break;
        case BACKWARD:
          for (succ_iterator succBlock = succ_begin((*blockIter)), E = succ_end((*blockIter)); succBlock != E; ++succBlock)
            analysisPreds.push_back(*succBlock);
          break;
      }

      analysisPredsByBlock[(*blockIter)] = analysisPreds;
  }

  //Iterate over blocks in function until convergence of output sets for all blocks
  while (!analysisConverged) {
    analysisConverged = true; //assume converged until proven otherwise during this iteration

    //TODO: if analysis is backwards, may want instead to iterate from back-to-front of blocks list

    for (std::vector<BasicBlock*>::iterator blockIter = blocks.begin(); blockIter != blocks.end(); ++blockIter) {
      DataFlowResultForBlock& blockVals = resultsByBlock[*blockIter];

      //Store old output before applying this analysis pass to the block (depends on analysis dir)
      DataFlowResultForBlock oldBlockVals = blockVals;
      BitVector oldPassOut = (direction == FORWARD) ? blockVals.out : blockVals.in;

      //If any analysis predecessors have outputs ready, apply meet operator to generate updated input set for this block
      BitVector* passInPtr = (direction == FORWARD) ? &blockVals.in : &blockVals.out;
      std::vector<BasicBlock*> analysisPreds = analysisPredsByBlock[*blockIter];
      std::vector<BitVector> meetInputs;
      //Iterate over analysis predecessors in order to generate meet inputs for this block
      for (std::vector<BasicBlock*>::iterator analysisPred = analysisPreds.begin(); analysisPred < analysisPreds.end(); ++analysisPred) {
        DataFlowResultForBlock& predVals = resultsByBlock[*analysisPred];

        BitVector meetInput = predVals.currTransferResult.baseValue;

        //If this pred matches a predecessor-specific value for the current block, union that value into value set
        DenseMap<BasicBlock*, BitVector>::iterator predSpecificValueEntry = predVals.currTransferResult.predSpecificValues.find(*blockIter);
        if (predSpecificValueEntry != predVals.currTransferResult.predSpecificValues.end()) {
//            errs() << "Pred-specific meet input from " << (*analysisPred)->getName() << ": " <<bitVectorToStr(predSpecificValueEntry->second) << "\n";
            meetInput |= predSpecificValueEntry->second;
        }

        meetInputs.push_back(meetInput);
      }
      if (!meetInputs.empty())
        *passInPtr = applyMeet(meetInputs);

      //Apply transfer function to input set in order to get output set for this iteration
      blockVals.currTransferResult = applyTransfer(*passInPtr, domainEntryToValueIdx, *blockIter);
      BitVector* passOutPtr = (direction == FORWARD) ? &blockVals.out : &blockVals.in;
      *passOutPtr = blockVals.currTransferResult.baseValue;

      //Update convergence: if the output set for this block has changed, then we've not converged for this iteration
      if (analysisConverged) {
        if (*passOutPtr != oldPassOut)
          analysisConverged = false;
        else if (blockVals.currTransferResult.predSpecificValues.size() != oldBlockVals.currTransferResult.predSpecificValues.size())
          analysisConverged = false;
        //(should really check whether contents of pred-specific values changed as well, but
        // that doesn't happen when the pred-specific values are just a result of phi-nodes)
      }
    }
  }

  DataFlowResult result;
  result.domainEntryToValueIdx = domainEntryToValueIdx;
  result.resultsByBlock = resultsByBlock;
  return result;
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
