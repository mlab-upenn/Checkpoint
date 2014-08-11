# Installing Checkpoint #

This file details building Checkpoint as a plugin to LLVM's opt tool as well as
including Checkpoint in a statically-linked build of opt. In the future, I hope
to include instructions for building a standalone Checkpoint executable.

The instructions for building a plugin come mostly from LLVM's own pass building
tutorial, found [online](http://llvm.org/docs/WritingAnLLVMPass.html) or
included with the LLVM soruce. Instructions for building a static library for
linking into opt or a standalone tool come from a combination of the
aformentioned tutorial and a [blog post](http://tinyurl.com/7akkcbc) on the
topic. Most of the information is reproduced here directly in the event that the
remote resorces become unavailable.

This guide is written for the `release_35` branch on the LLVM git mirror. It
will be revised for stable LLVM 3.5 and future versions of LLVM when they
become available.

### Getting and building LLVM ###

You'll need Python, Git, the Clang compiler toolchain, and either gmake or cmake
for this part. The rest of this guide will assume you've started this process in
your home directory `~\`. First, get the LLVM source.
```
git clone https://github.com/llvm-mirror/llvm.git
cd llvm
git checkout release_35
```
Make a build directory and generate build files using either the provided
configure script or with `cmake`.
```
mkdir ../build
cd ../build
```
and `cmake ../llvm/` or `../llvm/configure`. If you're planning on installing
your copy of LLVM, look up the proper flags for configure or cmake for
optimizing LLVM, as the default build is unoptimized and runs very slowly.
The cmake files and the output from `../llvm/configure --help` will contain the
flags for an optimized build.

Build LLVM and run the testsuite. Both of these operations will take a while, so
be patient. On [multicore](https://en.wikipedia.org/wiki/Multicore_processor),
[superthreading](https://en.wikipedia.org/wiki/Super-threading), or
[SMT](http://en.wikipedia.org/wiki/Simultaneous_multithreading) machines,
supplying `make` with the `-j` flag can significantly improve compile time by
splitting the build into multiple jobs.
```
make -j
make check
```

If you're on a UNIX-like system, used cmake in the previous steps, and are
getting compiler errors, it could be that the POSIX-standard compiler `c++` is
old, which cmake uses by defualt. Make sure you've installed a recent version of
clang and point the `c++` link to wherever `clang++` is installed. For example,
on Ubuntu you can do `sudo update-alternatives --config c++` and select
/usr/bin/clang++ from the table.

If you want to reference llvm without supplying full paths, you can install
LLVM tools and libraries with `make install`.

### Building Checkpoint as a plugin ###

Navigate back to the source tree, clone Checkpoint and checkout the `plugin`
branch. This branch contains changes to make Checkpoint suitable for building as
a plugin. We'll set up Checkpoint in `lib/Transforms`, a common location for
building opt passes.
```
cd ~/llvm/lib/Transforms
git clone https://github.com/mlab/Checkpoint.git
cd Checkpoint
git checkout plugin
```
Modify the build files in `lib/Transforms` so Checkpoint is build along with
the other plugins.

* In `LLVMBuild.txt`, add `Checkpoint` to the list of subdirectories.
* In `CMakeLists.txt`, add an `add_subdirectory(Checkpoint)` entry.
* In `Makefile`, add `Checkpoint` to the list of `PARALLEL_DIRS`.

Navigate back to `~/build/lib/Transforms` and run `make` to build Checkpoint. If
you used cmake and you're getting compiler erros, see the note above about
updating your POSIX compiler link.

### Building Checkpoint into opt ###

Building Checkpoint into opt requires modifying some LLVM internal headers and
the opt source so Checkpoint is linked when opt is build. Additionally,
Checkpoint must export some initialization routines for use by opt. All the
necessary changes to Checkpoint are in the master branch.

First, clone Checkpoint into the LLVM source tree
```
cd ~/llvm/lib/Transforms
git clone https://github.com/mlab/Checkpoint.git
```
Modify the build files in `lib/Transforms` just as you did in the plugin build.

First, we must create a header file in `include/llvm/Transforms`, Checkpoint.h:
```C++
#ifndef LLVM_CHECKPOINT_H
#define LLVM_CHECKPOINT_H

namespace llvm {
    FunctionPass* createCheckpointPass();
}

#endif
```

Now modify the header files in `include/llvm` so opt gets the proper
initialization declarations.

* in `InitializePasses.h`, add `void initializeCheckpointPass(PassRegistry&);`
to the `llvm` namespace.
* in `LinkAllPasses.h`, add `#include "llvm/Transforms/Checkpoint.h"` to the
includes list.
* in `LinkAllPasses.h`, add `(void) llvm::createCheckpointPass();` to the
`ForcePassLinking` routine.

Now, we'll modifty opt to initialize Checkpoint. Edit `tools/opt/opt.cpp` and
add `initializeCheckpointPass(Registry);` among the other pass initialization
calls.

Finally, modify the build files in `tools/opt` to let the build system know that
opt now depends on Checkpoint.

* in `Makefile`, add `checkpoint` to `LINK_COMPONENTS`.
* in `CMakeLists.txt`, add `Checkpoint` to `LLVM_LINK_COMPONENTS`.

Note the capitalization of Checkpoint used in each instance above.

### Using Checkpoint ###

Use Checkpoint like any other opt pass. If you built Checkpoint as a plugin,
use opt's `-load` flag to specify the location of checkpoint, and `-checkpoint`
to enable the pass itself:

`cat infile.bc | opt -load /path/to/Checkpoint.so -checkpoint -o outfile.bc`

If you linked Checkpoint into opt statically, you should omit the `-load` flag.
