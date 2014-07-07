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

#define DEBUG_TYPE "makecalls"
//#include "llvm/ADT/Statistic.h" for adding statistic counters
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
using namespace llvm;

namespace {
  class Makecalls : public ModulePass {
  public: 
    static char ID; // Pass identification, replacement for typeid
    Makecalls() : ModulePass(ID) {}
    
    virtual bool runOnModule(Module &M) {
      // get functions and create calls 
      // create main function for calls
      return true;                                                            // we modified the program
    }
  };
}

char Makecalls::ID = 0;
static RegisterPass<Makecalls> X("makecalls", "calls defined function many times with different parameters");
