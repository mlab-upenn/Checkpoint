#include "llvm/InitializePasses.h"
#include "llvm-c/Initialization.h"
#include "llvm/IR/DataLayout.h"

using namespace llvm;

/// initializeCustom - Initialize all passes in the Custom library
void llvm::initializeCustom(PassRegistry &Registry) {
  initializeCheckpointPass(Registry);
  initializeMakecallsPass(Registry);
}

/// LLVMInitializeCustom - C binding for initializeCustom
void LLVMInitializeCustom(LLVMPassRegistryRef R) {
  initializeCustom(*unwrap(R));
}
