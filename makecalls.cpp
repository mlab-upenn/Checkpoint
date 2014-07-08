//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//
#define numcalls 10
#define domain_min
#define domain_max
#define DEBUG_TYPE "makecalls"
//#include "llvm/ADT/Statistic.h" for adding statistic counters
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include <cassert>
using namespace llvm;

namespace {
  class Makecalls : public ModulePass {
  public: 
    static char ID; // Pass identification, replacement for typeid
    Makecalls() : ModulePass(ID) {}
    
    virtual bool runOnModule(Module &M) {
      // get function and types of parameters
      Module::iterator function = M.begin(); // right now we assume only one function per module, so this is the function we will modulate
      {
        Module::iterator end = M.end();
        assert(function == end && "More than one function in this module.\n");
        assert(function->doesNotThrow() && "Can't work with functions that use exceptions.\n");
      }
      Function::ArgumentListType& arguments = function->getArgumentList(); // get arguments
      assert(arguments.size() == 1 && "Function has more than one parameter.\n");
      Argument& func_input = arguments.front(); // we assume the function only has one parameter, and this is it
      Type* input_type = func_input.getType(); // get the type of the input parameter
      assert(input_type->isIntegerTy() && "Input parameter is not an integer.\n");
      unsigned width = input_type->getIntegerBitWidth(); // find the size of the input parameter so we know what range we can apply
      // create main function for calls
      // add calls to basic block, add basic block to main function
      return true;                                                            // we modified the program
    }
  };
}

char Makecalls::ID = 0;
static RegisterPass<Makecalls> X("makecalls", "calls defined function many times with different parameters");
