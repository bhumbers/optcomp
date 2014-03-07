////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_LICM_H__
#define __CLASSICAL_DATAFLOW_LICM_H__

#include <deque>

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
#include "reaching-defs.h"

#include <iomanip>
#include <queue>
#include <map>

using namespace llvm;
using namespace std;

namespace {

/** Runs LICM on a particular function
 * Note that this borrows from LLVM's LoopPass & LPPassManager, in that we run optimizations on each loop
 * in the function. However, this FunctionPass was used so that a reaching definition analysis could be executed
 * on the whole function before the per-loop transforms. */
class LoopInvariantCodeMotion : public FunctionPass {
 public:
  static char ID;

  LoopInvariantCodeMotion();

  bool doInitialization(Module& M);
  virtual bool runOnFunction(Function& F);
  virtual void getAnalysisUsage(AnalysisUsage& AU) const;

 protected:
  deque<Loop *> LQ;

  /** Returns the set of blocks which are part of the given loop and which have at least one successor outside the loop */
  SmallPtrSet<BasicBlock*, 32> getLoopExits(Loop* L);

  /** Returns block dominance info using dataflow framework */
  DataFlowResult computeDominance(Loop* L);

  /** Computes immediate dominance info given dataflow results with basic dominance info */
  map<BasicBlock*, BasicBlock*> computeImmediateDominance(DataFlowResult dominanceResults);

  void printDominanceInfo(DataFlowResult dominanceResults, map<BasicBlock*, BasicBlock*> immDoms);

  /** Returns set of statements (instructions) in given loop which are considered loop invariant */
  set<Value*> computeLoopInvariantStatements(Loop* L, map<Value*, ReachingDefinitionInfo> reachingDefs);

  /** Returns the set of statements (instructions) in given loop which are valid candidates for movement to loop preheader according to LICM*/
  set<Value*> computeCodeMotionCandidateStatements(Loop* L, DataFlowResult dominanceResults, set<Value*> invariantStatements);

  /** Applies LICM to given candidates where possible (basically, if all dependencies have also been moved).
   * Returns true if any motions were applied, which modifies the loop code */
  bool applyMotionToCandidates(Loop* L, set<Value*> motionCandidates);

  /** Recurse through all subloops and all loops  into LQ. (Source: LoopPass.cpp) */
  void addLoopIntoQueue(Loop* L);
};

char LoopInvariantCodeMotion::ID = 0;
RegisterPass<LoopInvariantCodeMotion> X("cd-licm", "15-745 Loop Invariant Code Motion");

}

#endif
