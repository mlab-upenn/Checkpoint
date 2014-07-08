//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

// Right now we assume this pass runs on modules containing only one function with a single integer argument
#define DEBUG_TYPE "makecalls"
//#include "llvm/ADT/Statistic.h" for adding statistic counters
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include <cassert> // assert
#include <cmath> // exp2
using namespace llvm;

namespace {
  const unsigned numcalls = 30; // number of calls to generate.  Parameter samples will be evenly distributed across the calls

  class Makecalls : public ModulePass {
  public: 
    static char ID; // Pass identification, replacement for typeid
    Makecalls() : ModulePass(ID) {}
    
    virtual bool runOnModule(Module &M) {
      // get function and types of parameters
      Module::iterator function = M.begin(); // get function
      assert(function == M.end() && "More than one function in this module.\n"); // only support one function per module
      assert(function->doesNotThrow() && "Can't work with functions that use exceptions.\n"); // only support functions that don't unwind
      Function::ArgumentListType& arguments = function->getArgumentList(); // get arguments
      assert(arguments.size() == 1 && "Function has more than one parameter.\n"); // only support functions with one argument
      Argument& func_input = arguments.front(); // get argument to modulate
      Type* input_type = func_input.getType(); // get the type of the argument
      assert(input_type->isIntegerTy() && "Input parameter is not an integer.\n"); // only support functions with integer argument
      
      // generate call parameters
      const unsigned bit_width = input_type->getIntegerBitWidth(); // find the size of the argument so we know what range we can apply
      const uint64_t distribution = (uint64_t)((exp2(bit_width)-1) / numcalls); // find the max value that fits into the argument
      // create main function for calls
      // add calls to basic block, add basic block to main function
      return true;                                                            // we modified the program
    }
  };
}

char Makecalls::ID = 0;
static RegisterPass<Makecalls> X("makecalls", "calls defined function many times with different parameters");
