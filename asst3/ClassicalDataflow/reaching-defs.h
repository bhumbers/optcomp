////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_REACHING_DEFS_H__
#define __CLASSICAL_DATAFLOW_REACHING_DEFS_H__

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "dataflow.h"

#include <map>

using namespace llvm;
using namespace std;

namespace llvm {

struct ReachingDefinitionInfo {
  //The variable for which the definitions apply
  Value* variable;

  //Mapping from program points (just above instruction key) to definitions that reach that point (values) for this variable
  map<Instruction*, vector<Value*> > defsByPoint;

  ReachingDefinitionInfo() {
    variable = 0;
  }
};

/** A modified version of our reaching definitions function from A2 which
 * now returns a mapping from variables to reaching definitions.
 * Includes fixes for more correct handling of definitions both with and without SSA form. */
class ReachingDefinitions {

 public:

  /** For the given function, returns lookup to reaching definitions for each variable*/
  map<Value*, ReachingDefinitionInfo> computeReachingDefinitions(Function& F);
};

}

#endif
