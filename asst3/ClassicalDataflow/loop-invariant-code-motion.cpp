////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/ValueMap.h"

#include "dataflow.h"

#include <iomanip>
#include <queue>

using namespace llvm;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////
//Dataflow analysis
class DominanceDataFlow : public DataFlow {

  protected:
    BitVector applyMeet(std::vector<BitVector> meetInputs) {
      BitVector meetResult;

      //Meet op = intersection of inputs
      if (!meetInputs.empty()) {
        for (int i = 0; i < meetInputs.size(); i++) {
          if (i == 0)
            meetResult = meetInputs[i];
          else
            meetResult &= meetInputs[i];
        }
      }

      return meetResult;
    }

    TransferResult applyTransfer(const BitVector& value, DenseMap<Value*, int> domainEntryToValueIdx, BasicBlock* block) {
      TransferResult transfer;
      transfer.baseValue = value;

      //Transfer of dominance is simple: Just add the current block to the dominance set
      unsigned blockIdx = domainEntryToValueIdx[block];
      transfer.baseValue.set(blockIdx);

      return transfer;
    }
};
//////////////////////////////////////////////////////////////////////////////////////////////

class LoopInvariantCodeMotion : public LoopPass {
 public:
  static char ID;

  LoopInvariantCodeMotion() : LoopPass(ID) { }

  bool doInitialization(Loop * L, LPPassManager &LPM) {
//    errs() << "Doing LoopPass Init";
    return false;
  }

  DataFlowResult computeDominance(Loop* L) {
    //Dataflow domain = Set of all basic blocks in the loop (as well as their parents)
    std::set<BasicBlock*> blocksSet;
    std::vector<BasicBlock*> loopBlocks = L->getBlocks();
    for (std::vector<BasicBlock*>::iterator blockIter = loopBlocks.begin(); blockIter != loopBlocks.end(); ++blockIter) {
      BasicBlock* block = *blockIter;
      //Add parents
      for (pred_iterator predBlock = pred_begin(block), E = pred_end(block); predBlock != E; ++predBlock)
        blocksSet.insert(*predBlock);
      blocksSet.insert(block); //Add block
    }
    std::vector<Value*> domain;
    std::vector<BasicBlock*> blocks;
    for (std::set<BasicBlock*>::iterator it = blocksSet.begin(); it != blocksSet.end(); ++it) {
      blocks.push_back(*it);
      domain.push_back(*it);
    }

    int numVars = domain.size();

    //Boundary value at entry is just the entry block (entry dominates itself)
    BitVector boundaryCond(numVars, false);

    //Initial interior set is full set of blocks
    BitVector initInteriorCond(numVars, true);

    //Get dataflow values at IN and OUT points of each block
    DominanceDataFlow flow;
    return flow.run(blocks, domain, DataFlow::FORWARD, boundaryCond, initInteriorCond);
  }

  virtual bool runOnLoop(Loop* L, LPPassManager& LPM) {

    //Don't bother with loops without a preheader
    if (L->getLoopPreheader() == NULL)
      return false;

    LoopInfo* loopInfo = &getAnalysis<LoopInfo>();

    //First, compute dominance info for blocks in the loop
    DataFlowResult dominanceResults = computeDominance(L);
    //Then, find the immediate dominators in a somewhat less-than-optimally-efficient way: basically,
    //for each block B, walk up the graph toward the root of the CFG in a BFS ordering until we see a node in dom(B)
    //There appear to be better idom algorithms, but I wasn't sure how to make them work nicely with our dataflow framework.
    ValueMap<BasicBlock*, BasicBlock*> immDoms;
    for (DenseMap<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
           resultsIter != dominanceResults.resultsByBlock.end();
           ++resultsIter) {
      DataFlowResultForBlock& blockResult = resultsIter->second;
      BitVector visited(dominanceResults.resultsByBlock.size(), false);
      std::queue<BasicBlock*> work;
      work.push(resultsIter->first);
      while (!work.empty()) {
        BasicBlock* currAncestor = work.front();
        work.pop();
        int currIdx = dominanceResults.domainEntryToValueIdx[currAncestor];
        visited.set(currIdx);

        //If ancesstor is contained in dom set for the results block, mark as idom and quit
        if (blockResult.in[currIdx]) {
          immDoms[resultsIter->first] = currAncestor;
          break;
        }

        for (pred_iterator predBlock = pred_begin(currAncestor), E = pred_end(currAncestor); predBlock != E; ++predBlock) {
          int predIdx = dominanceResults.domainEntryToValueIdx[*predBlock];
          if (!visited[predIdx]) {
            work.push(*predBlock);
          }
        }
      }
    }

    errs() << "{";
    for (DenseMap<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
           resultsIter != dominanceResults.resultsByBlock.end();
           ++resultsIter) {
      errs() << resultsIter->first->getName() << "  ";
    }
    errs() << "}\n";

    //Output: Print all
    for (DenseMap<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
           resultsIter != dominanceResults.resultsByBlock.end();
           ++resultsIter) {
      char str[100];
      BasicBlock* idom = immDoms[resultsIter->first];
      if (idom) {
        sprintf(str, "%-30s is idom of %-30s", ((std::string)idom->getName()).c_str(), ((std::string)resultsIter->first->getName()).c_str());
        errs() << str << "\n";
      }
      else {
        sprintf(str, "%-30s has no idom", ((std::string)resultsIter->first->getName()).c_str());
        errs() << str << "\n";
      }
//      sprintf(str, "Dominators for %-20s:", );
//      errs() << str << bitVectorToStr(resultsIter->second.out) << "\n";
    }

    //TODO: Compute reaching definitions
    //TODO: Compute loop invariant computations
    //TODO: Find loop exits (nodes w/ exits outside loop)
    //TODO: Mark candidate statements for motion. Necessary conditions:
    //  Loop invariant
    //  In blocks that dominate all loop exits
    //  Assigned to variable not assigned to elsewhere in loop
    //  In blocks that dominate all blocks in the loop that use the variable assigned
    //TODO: Move candidates for LICM to preheader
    //  Do a DFS ordering of blocks;
    //  Move candidate to preheader if all invariant ops that it depends on have been moved

    //TODO: Ensure that we either iterate over loops until all LICM expressions bubble out,
    // or that we process loops from innermost to outermost ordering

    return false;
  }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<LoopInfo>();
  }

};

char LoopInvariantCodeMotion::ID = 0;
RegisterPass<LoopInvariantCodeMotion> X("cd-licm", "15-745 Loop Invariant Code Motion");

}
