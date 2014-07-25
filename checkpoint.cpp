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
#include "llvm/IR/Constants.h"         // ConstantDataArray, ConstantInt, ConstantExpr
using namespace llvm;

namespace {
  class Checkpoint : public FunctionPass {
    Constant* checkpoint_func; // functions initialized by doInitialization, calls to these are inserted by the pass
    Constant* init_func;
    Constant* deinit_func;

    Constant* enter_string; // arguments to checkpoint function
    Constant* exit_string;
    std::vector<Constant*> idx; // indexes for getting pointers to data

  public: 
    static char ID; // Pass identification, replacement for typeid
    Checkpoint() : FunctionPass(ID) {}

    // makes sure there are declarations for the profiling calls in this module
    virtual bool doInitialization(Module & M) {
      LLVMContext& Mc = M.getContext(); // get module's LLVMContext

      // get types for function declaration
      Type* local_void = Type::getVoidTy(Mc);                            // get void type
      Type* string_pointer = Type::getInt8PtrTy(Mc);                        // get string pointer type
      auto function_arguments = std::vector<Type*>(2, string_pointer);               // create a list of arguments: 2 string pointers
      FunctionType* void_function_type 
        = FunctionType::get(local_void, ArrayRef<Type*>(), false);                   // get function type for init/deinit calls
      FunctionType* string_function_type
        = FunctionType::get(local_void, ArrayRef<Type*>(function_arguments), false); // get function type for profiling calls
        
      // insert function declarations
      checkpoint_func = M.getOrInsertFunction("checkpoint", string_function_type);                 // insert function declaration, and save function for CallInst later
      init_func = M.getOrInsertFunction("initialize", void_function_type);
      deinit_func = M.getOrInsertFunction("print_results", void_function_type);

      // initialize enter and exit strings
      Constant* enter_data = ConstantDataArray::getString(Mc, "Entering "); // get string data
      Constant* exit_data = ConstantDataArray::getString(Mc, "Exiting ");
      Constant* zero = ConstantInt::get(IntegerType::get(Mc, 1), 0); // get the zero constant for this module
      idx.push_back(zero); // two values of 0 give us the first position in the string
      idx.push_back(zero);
      enter_string = ConstantExpr::getGetElementPtr(enter_data, idx); // get a pointer to the start of enter string
      exit_string = ConstantExpr::getGetElementPtr(exit_data, idx); // get a pointer to the start of exit string
      return true;                                                                                 // we modified the program
    }

    virtual bool runOnFunction(Function &F) {
      if (F.hasInternalLinkage()) return false;                               // only instrument functions present in the source.
      DEBUG(dbgs() << "Adding checkpoints to " << F.getName() << "\n");       // conditionally included on debug builds
      
      // make function name string parameter
      auto fname = F.getName();
      auto & fctx = F.getContext();
      Constant* fname_data = ConstantDataArray::getString(fctx, fname);
      Constant* fname_string = ConstantExpr::getGetElementPtr(fname_data, idx);
      
      // get call insertion points
      Instruction* first_insn = F.front().getFirstNonPHI();                   // get the first instruction of the function
      std::vector<Instruction*> returns;
      for (inst_iterator I = inst_begin(F); I != inst_end(F); ++I) {
        if (isa<ReturnInst>(&*I)) returns.push_back(&*I);                     // fill a vector with all the return instructions
      }
      
      // insert calls
      CallInst::Create(checkpoint_func,                                       // insert call at start of function
                      {enter_string, fname_string},
                      "",
                      first_insn);
      if (F.getName() == "main")                                              // if we're in the main function
        CallInst::Create(init_func, "", first_insn);                          // also insert the init call
      for (auto ret = returns.begin(); ret != returns.end(); ++ret) {         // for each return instruction
        CallInst::Create(checkpoint_func,                                     // insert a checkpoint before the return
                        {exit_string, fname_string},
                        "",
                        *ret);
        if (F.getName() == "main")                                            // if we're in the main function
          CallInst::Create(deinit_func, "", *ret);                            // also insert a deinit before the return
      }
      return true;                                                            // we modified the program
    }
  };
}

char Checkpoint::ID = 0;
static RegisterPass<Checkpoint> X("checkpoint", "checkpoint instrumentation for runtime profiling");
