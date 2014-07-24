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

#define DEBUG_TYPE "checkpoint"
//#include "llvm/ADT/Statistic.h" for adding statistic counters
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"      // CallInst
#include "llvm/Support/InstIterator.h" // inst_iterator, methods on I and E
#include "llvm/Support/Debug.h"        // DEBUG(), bdgs()
#include "llvm/IR/Module.h"            // getOrInsertFunction
using namespace llvm;

namespace {
  class Checkpoint : public FunctionPass {
    Constant* checkpoint_func; // functions initialized by doInitialization, calls to these are inserted by the pass
    Constant* init_func;
    Constant* deinit_func;

  public: 
    static char ID; // Pass identification, replacement for typeid
    Checkpoint() : FunctionPass(ID) {}

    // makes sure there are declarations for the profiling calls in this module
    virtual bool doInitialization(Module & M) {
      // get types for function declaration
      Type* local_void = Type::getVoidTy(M.getContext());                            // get void type
      Type* string_pointer = Type::Int8PtrTy(M.getContext());                        // get string pointer type
      auto function_arguments = std::vector<Type*>(2, string_pointer);               // create a list of arguments: 2 string pointers
      FunctionType* void_function_type 
        = FunctionType::get(local_void, ArrayRef<Type*>(), false);                   // get function type for init/deinit calls
      FunctionType* string_function_type
        = FunctionType::get(local_void, ArrayRef<Type*>(function_arguments), false); // get function type for profiling calls
        
      
      // insert function declarations
      checkpoint_func = M.getOrInsertFunction("checkpoint", string_function_type);                 // insert function declaration, and save function for CallInst later
      init_func = M.getOrInsertFunction("initialize", void_function_type);
      deinit_func = M.getOrInsertFunction("print_results", void_function_type);
      return true;                                                                                 // we modified the program
    }

    // inserts calls to entry_func at the beginning of F and to exit_func before each return
    bool insertAtEntryAndReturn(Function &F, Constant* entry_func, Constant* exit_func) {
      // insert at start of function
      Instruction* first_insn = F.front().getFirstNonPHI();                   // phi nodes never happen at start of function
      CallInst::Create(entry_func, "", first_insn);                           // add call before the first instruction
      
      // insert at return of function
      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {   // for each insn in the function
        ReturnInst* current_insn = dyn_cast<ReturnInst>(&*I);
        if (!current_insn) continue;                                          // skip this insn if it isn't a return
        CallInst::Create(exit_func, "", current_insn);                        // otherwise, insert a call before the return
      }
      return true;                                                            // we've modified the program 
    }

    virtual bool runOnFunction(Function &F) {
      if (F.hasInternalLinkage()) return false;                               // only instrument functions present in the source.
      DEBUG(dbgs() << "Adding checkpoints to " << F.getName() << "\n");       // conditionally included on debug builds
      insertAtEntryAndReturn(F, checkpoint_func, checkpoint_func);            // insert calls to checkpoint function
      if (F.getName() == "main")                                              // if this is the main function ...
        insertAtEntryAndReturn(F, init_func, deinit_func);                    // then also add init and deinit
      return true;                                                            // we modified the program
    }
  };
}

char Checkpoint::ID = 0;
static RegisterPass<Checkpoint> X("checkpoint", "checkpoint instrumentation for runtime profiling");