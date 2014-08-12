//===- Checkpoint.cpp - Pass for instrumenting code with callbacks --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements automatic calls to a profiling API, for profiling
// without modifying source code, using alternate runtimes, or sampling
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "checkpoint"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"      // CallInst
#include "llvm/IR/InstIterator.h"      // Iteration over instructions
#include "llvm/Support/Debug.h"        // DEBUG(), bdgs()
#include "llvm/IR/Module.h"            // getOrInsertFunction
#include "llvm/IR/Constants.h"         // ConstantDataArray, ConstantInt, ConstantExpr
#include "llvm/Transforms/Checkpoint.h"// createCheckpointPass declaration
#include "llvm/IR/GlobalVariable.h"    // GlobalVariable
#include "llvm/ADT/StringMap.h"        // string map container
using namespace llvm;

namespace {
  class Checkpoint : public FunctionPass {
    Constant* checkpoint_func;            // functions initialized by doInitialization, calls to these are inserted by the pass
    Constant* init_func;
    Constant* deinit_func;

    Constant* enter_string;               // arguments to checkpoint function
    Constant* exit_string;
    std::vector<Constant*> idx;           // indexes for getting pointers to data
    StringMap<Constant*> name_parameters; // maps function names to global vars containing the name

  public: 
    static char ID;
    Checkpoint() : FunctionPass(ID) {
      initializeCheckpointPass(*PassRegistry::getPassRegistry());
    }

    // makes sure there are declarations for the profiling calls in this module
    virtual bool doInitialization(Module & M) {
      LLVMContext& Mc = M.getContext();                                              // get module's LLVMContext

      // get types for function declaration
      Type* local_void = Type::getVoidTy(Mc);                                        // get void type
      Type* string_pointer = Type::getInt8PtrTy(Mc);                                 // get string pointer type
      auto function_arguments = std::vector<Type*>(2, string_pointer);               // create a list of arguments: 2 string pointers
      FunctionType* void_function_type 
        = FunctionType::get(local_void, ArrayRef<Type*>(), false);                   // get function type for init/deinit calls
      FunctionType* string_function_type
        = FunctionType::get(local_void, ArrayRef<Type*>(function_arguments), false); // get function type for profiling calls
        
      // insert function declarations
      checkpoint_func = M.getOrInsertFunction("checkpoint", string_function_type);   // insert function declaration, and save function for CallInst later
      init_func = M.getOrInsertFunction("initialize", void_function_type);
      deinit_func = M.getOrInsertFunction("print_results", void_function_type);

      // add enter and exit string globals
      Constant* enter_data = ConstantDataArray::getString(Mc, "Entering ");          // get string data
      Constant* exit_data = ConstantDataArray::getString(Mc, "Exiting ");
      GlobalVariable* enter_var = new GlobalVariable(
          M,                           // module
          enter_data->getType(),       // type
          true,                        // is constant?
          GlobalValue::PrivateLinkage, // linkage type
          enter_data);                 // initializer
      GlobalVariable* exit_var = new GlobalVariable(
          M,                           // module
          exit_data->getType(),        // type
          true,                        // is constant?
          GlobalValue::PrivateLinkage, // linkage type
          exit_data);                  // initializer

      // get pointers to strings for parameters to checkpoint calls
      Constant* zero = ConstantInt::get(IntegerType::get(Mc, 1), 0);         // get the zero constant for this module
      idx.push_back(zero);                                                   // two values of 0 give us the first position in the string
      idx.push_back(zero);
      enter_string = ConstantExpr::getGetElementPtr(enter_var, idx);         // get a pointer to the start of enter string
      exit_string = ConstantExpr::getGetElementPtr(exit_var, idx);           // get a pointer to the start of exit string
      
      // add global strings for function names
      for (Function& f : M.getFunctionList()) {
        if (f.empty()) continue;                                             // only get names for defined functions
        Constant* name_data = ConstantDataArray::getString(Mc, f.getName()); // get string data from name
        GlobalVariable* name_var = new GlobalVariable(
            M,
            name_data->getType(),
            true,
            GlobalValue::PrivateLinkage,
            name_data);
        Constant* name_string = ConstantExpr::getGetElementPtr(name_var, idx);
        name_parameters.insert(
            std::pair<StringRef, Constant*>(f.getName(), name_string));
      }
      
      return true;                                                            // we modified the program
    }

    virtual bool runOnFunction(Function &F) {
      if (F.hasInternalLinkage()) return false;                               // only instrument functions present in the source.
      DEBUG(dbgs() << "Adding checkpoints to " << F.getName() << "\n");       // conditionally included on debug builds
      
      // get function name string parameter
      assert(name_parameters.count(F.getName()) == 1 &&
          "Function name not in global array map");
      Constant* fname_string = name_parameters[F.getName()];

      // get call insertion points
      Instruction* first_insn = F.front().getFirstNonPHI();                   // get the first instruction of the function
      std::vector<Instruction*> returns;
      for (BasicBlock& bb : F) {                                              // for each basic block in the function,
        Instruction* i = bb.getTerminator();
        assert(i && "Malformed Basic Block");
        if (isa<ReturnInst>(i)) returns.push_back(i);                         // if the terminator is a return, store it.
      }
      
      // insert calls
      if (F.getName() == "main")                                              // if we're in the main function
        CallInst::Create(init_func, "", first_insn);                          // insert the init call
      CallInst::Create(checkpoint_func,                                       // insert call at start of function
                      {enter_string, fname_string},
                      "",
                      first_insn);
      for (Instruction* ret : returns) {                                      // for each return instruction
        CallInst::Create(checkpoint_func,                                     // insert a checkpoint before the return
                        {exit_string, fname_string},
                        "",
                        ret);
        if (F.getName() == "main")                                            // if we're in the main function
          CallInst::Create(deinit_func, "", ret);                             // also insert a deinit before the return
      }
      return true;                                                            // we modified the program
    }
  };
}

char Checkpoint::ID = 0;
INITIALIZE_PASS(Checkpoint, "checkpoint", "checkpoint instrumentation for runtime profiling", true, false);

FunctionPass* llvm::createCheckpointPass() {
  return new Checkpoint();
}

#include "llvm-c/Initialization.h"
#include "llvm/IR/DataLayout.h"

void LLVMInitializeCheckpointPass(LLVMPassRegistryRef R) {
  initializeCheckpointPass(*unwrap(R));
}
