////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#include "loop-invariant-code-motion.h"

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/ADT/SmallVector.h"

#include "llvm/Analysis/ValueTracking.h"

#include "dataflow.h"

#include <iomanip>
#include <queue>
#include <map>

using namespace llvm;
using namespace std;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////
//Dataflow analyses

//DOMINANCE
class DominanceDataFlow : public DataFlow {
  protected:
    BitVector applyMeet(std::vector<BitVector> meetInputs) {
      BitVector meetResult;

      //Meet op = intersection of inputs
      if (!meetInputs.empty()) {
        for (int i = 0; i < meetInputs.size(); i++) {
//          errs() << "  " << bitVectorToStr(meetInputs[i]) << ", ";
          if (i == 0)
            meetResult = meetInputs[i];
          else
            meetResult &= meetInputs[i];
        }
      }
//      errs() << "\n";

      return meetResult;
    }

    TransferResult applyTransfer(const BitVector& value, DenseMap<Value*, int> domainEntryToValueIdx, BasicBlock* block) {
      TransferResult transfer;
      transfer.baseValue = value;

//      errs() << "Applying transfer for block: " << block->getName() << "\n";
//      errs() << "Pre: " << bitVectorToStr(value) << "\n";

      //Transfer of dominance is simple: Just add the current block to the dominance set
      unsigned blockIdx = domainEntryToValueIdx[block];
      transfer.baseValue.set(blockIdx);

//      errs() << "Post: " << bitVectorToStr(transfer.baseValue) << "\n";

      return transfer;
    }
};
//////////////////////////////////////////////////////////////////////////////////////////////

LoopInvariantCodeMotion::LoopInvariantCodeMotion() : FunctionPass(ID) { }

bool LoopInvariantCodeMotion::doInitialization(Module& M) {
  return false;
}

bool LoopInvariantCodeMotion::runOnFunction(Function& F) {
  bool modified = false;

  //Get reaching definitions at each program point over whole function
  map<Value*, ReachingDefinitionInfo> reachingDefs = ReachingDefinitions().computeReachingDefinitions(F);

  //Add all loops into the processing queue. Note that addLoopIntoQueue will recursively add subloops of each
  // top-level loop in front of the parent loop, so that processing will be from most-to-least nested order.
  // This helps guarantee that any loop invariant code motion will "bubble out" to the outer most loop.
  LoopInfo& LI = getAnalysis<LoopInfo>();
  for (LoopInfo::reverse_iterator I = LI.rbegin(), E = LI.rend(); I != E; ++I)
    addLoopIntoQueue(*I);

  //Apply LICM to each loop in the work queue
  while (!LQ.empty()) {
    Loop* L  = LQ.back();

    //Don't bother with loops without a preheader
    if (L->getLoopPreheader() == NULL)
      return false;

    DataFlowResult dominanceResults = computeDominance(L);
    map<BasicBlock*, BasicBlock*> immDoms = computeImmediateDominance(dominanceResults);
    printDominanceInfo(dominanceResults, immDoms);

    set<Value*> loopInvariantStatements = computeLoopInvariantStatements(L, reachingDefs);

    set<Value*> codeMotionCandidateStatements = computeCodeMotionCandidateStatements(L, loopInvariantStatements);

    bool loopModified = applyMotionToCandidates(L, codeMotionCandidateStatements);

    modified |= loopModified;

    LQ.pop_back();
  }

  //TODO: Move candidates for LICM to preheader
  //  If any moves, mark modified = true
  //  Do a DFS ordering of blocks;
  //  Move candidate to preheader if all invariant ops that it depends on have been moved

  return modified;
}

void LoopInvariantCodeMotion::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.addRequired<LoopInfo>();
}

SmallPtrSet<BasicBlock*, 32> LoopInvariantCodeMotion::getLoopExits(Loop* L) {
  SmallVector<BasicBlock*, 32> loopSuccessors;
  L->getUniqueExitBlocks(loopSuccessors);

  SmallPtrSet<BasicBlock*, 32> loopExits;
  for (SmallVector<BasicBlock*, 32>::iterator i = loopSuccessors.begin(); i < loopSuccessors.end(); ++i) {
    //Note: As a result of the loop-simplify pass, each out-of-loop successor's sole predecessor should be part of this loop
    loopExits.insert(*pred_begin(*i));
  }

  return loopExits;
}

DataFlowResult LoopInvariantCodeMotion::computeDominance(Loop* L) {
  //Dataflow domain = Set of all basic blocks in the loop (as well as their parents)
  std::set<BasicBlock*> blocksSet;
  std::vector<BasicBlock*> loopBlocks = L->getBlocks();
  for (std::vector<BasicBlock*>::iterator blockIter = loopBlocks.begin(); blockIter != loopBlocks.end(); ++blockIter) {
    BasicBlock* block = *blockIter;
    //Add parents
    for (pred_iterator predBlock = pred_begin(block), E = pred_end(block); predBlock != E; ++predBlock) {
      blocksSet.insert(*predBlock);
    }
    blocksSet.insert(block); //Add block
  }
  std::vector<Value*> domain;
  std::vector<BasicBlock*> blocks;
  for (std::set<BasicBlock*>::iterator it = blocksSet.begin(); it != blocksSet.end(); ++it) {
//    errs() << "Adding to domain for dominance: " << (*it)->getName() << "\n";
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

map<BasicBlock*, BasicBlock*> LoopInvariantCodeMotion::computeImmediateDominance(DataFlowResult dominanceResults) {
  map<BasicBlock*, BasicBlock*> immDoms;

  //We find the immediate dominators in a somewhat less-than-optimally-efficient way: basically,
  //for each block B, walk up the graph toward the root of the CFG in a BFS ordering until we see a node in dom(B)
  //There appear to be better idom algorithms, but I wasn't sure how to make them work nicely with our dataflow framework.

  for (map<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
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

//      errs() << "Checking if idom of block " << resultsIter->first->getName() << " is " << currAncestor->getName() << "\n";

      //If ancestor is contained in dom set for the results block, mark as idom and quit
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

  return immDoms;
}

void LoopInvariantCodeMotion::printDominanceInfo(DataFlowResult dominanceResults, map<BasicBlock*, BasicBlock*> immDoms) {
  //Output: Print immediate dominance information
  errs() << "Dominance domain: {";
  for (map<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
         resultsIter != dominanceResults.resultsByBlock.end();
         ++resultsIter) {
    errs() << resultsIter->first->getName() << "  ";
  }
  errs() << "}\n";
  for (map<BasicBlock*, DataFlowResultForBlock>::iterator resultsIter = dominanceResults.resultsByBlock.begin();
         resultsIter != dominanceResults.resultsByBlock.end();
         ++resultsIter) {
    char str[100];
    BasicBlock* idom = immDoms[resultsIter->first];
    if (idom) {
      sprintf(str, "%s is idom'd by %s", ((std::string)resultsIter->first->getName()).c_str(), ((std::string)idom->getName()).c_str());
      errs() << str << "\n";
    }
    else {
      sprintf(str, "%s has no idom", ((std::string)resultsIter->first->getName()).c_str());
      errs() << str << "\n";
    }
//      sprintf(str, "Dominators for %-20s:", ((std::string)resultsIter->first->getName()).c_str());
//      errs() << str << bitVectorToStr(resultsIter->second.in) << "\n";
  }
}

std::set<Value*>  LoopInvariantCodeMotion::computeLoopInvariantStatements(Loop* L, map<Value*, ReachingDefinitionInfo> reachingDefs) {
  std::set<Value*> loopInvariantStatements;

  std::vector<BasicBlock*> loopBlocks = L->getBlocks();

  //Initialize invariant statement set
  for (std::vector<BasicBlock*>::iterator blockIter = loopBlocks.begin(); blockIter != loopBlocks.end(); ++blockIter) {
    for (BasicBlock::iterator instIter = (*blockIter)->begin(), e = (*blockIter)->end(); instIter != e; ++instIter) {
      Value* v = instIter;

      //First, check if this is an easy invariance case
      if (isa<Constant>(v) || isa<Argument>(v) || isa<GlobalValue>(v))
        loopInvariantStatements.insert(v);

      //Otherwise, check more complex conditions for typical instructions:
      //Statement A=B+C+D+... is invariant if all the reaching defs for all its operands (B, C, D, ...) are outside the loop
      //(and a few other misc safety conditions are met)
      else if (isa<Instruction>(v)) {
        Instruction* I = static_cast<Instruction*>(v);

//        errs() << "Considering invariance of: " << valueToStr(v) << "\n";

        bool mightBeLoopInvariant = (isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory() && !isa<LandingPadInst>(I));

        if (mightBeLoopInvariant) {
          bool allOperandsOnlyDefinedOutsideLoop = true;

          for (User::op_iterator opIter = I->op_begin(), e = I->op_end(); opIter != e; ++opIter) {
            Value* opVal = *opIter;
            ReachingDefinitionInfo varDefsInfo = reachingDefs[opVal];

            vector<Value*> varDefsAtStatement = varDefsInfo.defsByPoint[I];
            for (int i = 0; i < varDefsAtStatement.size(); i++) {
              if (isa<Instruction>(varDefsAtStatement[i])) {
                if (L->contains(((Instruction*)varDefsAtStatement[i])->getParent()))  {
                  allOperandsOnlyDefinedOutsideLoop = false;
                  break;
                }
              }
            }

            if (!allOperandsOnlyDefinedOutsideLoop)
              break;
          }

          if (allOperandsOnlyDefinedOutsideLoop)
            loopInvariantStatements.insert(v);
        }
      }
    }
  }

  //Iteratively update invariant statement set until convergence
  //(since invariant will grow monotonically, we detect this simply by seeing if it stops growing)
  bool converged = false;
  int invariantSetSize = loopInvariantStatements.size();
  while (!converged) {
    int prevInvariantSetSize = invariantSetSize;

    //Check through all statements in the loop, adding statement A=B+C+D+... to the invariant set if
    //all operands B,C,... have a single reaching definition at that statement AND those definitions are loop-invariant

    for (std::vector<BasicBlock*>::iterator blockIter = loopBlocks.begin(); blockIter != loopBlocks.end(); ++blockIter) {
      for (BasicBlock::iterator instIter = (*blockIter)->begin(), e = (*blockIter)->end(); instIter != e; ++instIter) {
        Value* v = instIter;

//        errs() << "Considering invariance of: " << valueToDefinitionVarStr(v) << "\n";

        //If already known to be invariant, skip checking again
        if (loopInvariantStatements.find(v) != loopInvariantStatements.end())
          continue;

        if (isa<Instruction>(v)) {
          Instruction* I = static_cast<Instruction*>(v);

          bool mightBeLoopInvariant = (isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory() && !isa<LandingPadInst>(I));

          if (mightBeLoopInvariant) {
            bool allOperandsHaveSingleLoopInvariantDef = true;

            for (User::op_iterator opIter = I->op_begin(), e = I->op_end(); opIter != e; ++opIter) {
              Value* opVal = *opIter;
              ReachingDefinitionInfo varDefsInfo = reachingDefs[opVal];

              //Check whether operand has single, loop-invariant definition.
              vector<Value*> varDefsAtStatement = varDefsInfo.defsByPoint[I];
              if (varDefsAtStatement.size() != 1 || loopInvariantStatements.count(varDefsAtStatement[0]) == 0) {
                allOperandsHaveSingleLoopInvariantDef = false;
                break;
              }
            }

            if (allOperandsHaveSingleLoopInvariantDef)
              loopInvariantStatements.insert(v);
          }
        }
      }
    }

    invariantSetSize = loopInvariantStatements.size();
    converged = (invariantSetSize == prevInvariantSetSize);
  }

  //DEBUGGING: Print out loop invariant statements
  errs() << "Loop invariant statements: {\n";
  for (std::set<Value*>::iterator liIter = loopInvariantStatements.begin(); liIter != loopInvariantStatements.end(); ++liIter) {
    errs() << valueToStr(*liIter) << "\n";
  }
  errs() << "}\n";

  return loopInvariantStatements;
}

set<Value*> LoopInvariantCodeMotion::computeCodeMotionCandidateStatements(Loop* L, set<Value*> invariantStatements) {
  set<Value*> motionCandidates;

  //Candidate statements for LICM must meet the following:
  //1) Must be loop invariant
  //2) Must be in a block that dominates all exits of the loop
  //3) Must be in a block that dominates all blocks where the definition variable of the statement is used
  //4) Must assign to a variable that has no other assignments in the loop

  std::vector<BasicBlock*> loopBlocks = L->getBlocks();
  SmallPtrSet<BasicBlock*, 32> loopExits = getLoopExits(L);

  for (std::vector<BasicBlock*>::iterator blockIter = loopBlocks.begin(); blockIter != loopBlocks.end(); ++blockIter) {
    for (BasicBlock::iterator instIter = (*blockIter)->begin(), e = (*blockIter)->end(); instIter != e; ++instIter) {
      Instruction* I = instIter;

      bool isInvariant = false;
      if (invariantStatements.count(I) > 0)
        isInvariant = true;


      //TODO: Check other conditions
      if (isInvariant)
        motionCandidates.insert(I);
    }
  }

  return motionCandidates;
}

bool LoopInvariantCodeMotion::applyMotionToCandidates(Loop* L, set<Value*> motionCandidates) {
  bool motionApplied = false;

  BasicBlock* preheader = L->getLoopPreheader();

  set<Instruction*> toMoveSet;

  //Algorithm: Do a DFS over the blocks of the loop and move each candidate to end of preheader if all
  //of its dependencies have also been moved to the preheader

  set<BasicBlock*> visited;

  stack<BasicBlock*> work;
  work.push(*succ_begin(preheader)); //start at loop header... the sole successor of the pre-header
  while (!work.empty()) {
    BasicBlock* block = work.top();
    work.pop();
    visited.insert(block);

    //For each instruction in the block, move to preheader if it's a code motion candidate and conditions are met
    for (BasicBlock::iterator instIter = block->begin(), e = block->end(); instIter != e; ++instIter) {
      Instruction* I = instIter;

      if (motionCandidates.count(I) > 0) {
        //TODO: Check that all invariant ops that this instruction uses are moved to preheader as well

        motionApplied = true;
        toMoveSet.insert(I);
      }
    }

    //Add successors to search
    for (succ_iterator successorBlock = succ_begin(block), E = succ_end(block); successorBlock != E; ++successorBlock) {
      if (visited.count(*successorBlock) == 0)
        work.push(*successorBlock);
    }
  }

  //Move all the to-move items now (a bit too tricky to do it while iterating over blocks)
  for (set<Instruction*>::iterator it = toMoveSet.begin(); it != toMoveSet.end(); ++it) {
    Instruction* instructionToMove = *it;

    //Insert as the next-to-last instruction of preheader (last needs to remain the block's control flow branch)
    Instruction* preheaderEnd = &(preheader->back());

    errs() << "Preheader end: " << valueToStr(preheaderEnd) << "\n";

    instructionToMove->removeFromParent();
    instructionToMove->insertBefore(preheaderEnd);
  }

  return motionApplied;
}

void LoopInvariantCodeMotion::addLoopIntoQueue(Loop* L) {
  this->LQ.push_back(L);
  for (Loop::reverse_iterator I = L->rbegin(), E = L->rend(); I != E; ++I)
    addLoopIntoQueue(*I);
}

}
