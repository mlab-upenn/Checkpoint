# Building Checkpoint #

This file details building Checkpoint as a plugin to LLVM's opt tool as well as
including Checkpoint in a statically-linked build of opt. In the future, I hope
to include instructions for building a standalone Checkpoint executable.

The instructions for building a plugin come mostly from LLVM's own pass building
tutorial, found [online](http://llvm.org/docs/WritingAnLLVMPass.html) or
included with the LLVM soruce. Instructions for building a static library for
linking into opt or a standalone tool come from a combination of the
aformentioned tutorial, and a [blog post](http://tinyurl.com/7akkcbc) on the
topic. Most of the information is reproduced here directly in the event that the
remote resorces become unavailable.

This guide was tested on LLVM 3.4.2, the latest (stable) version of LLVM at the
time of writing.

### Getting the Sources ###

1. Download the LLVM source from [here](http://llvm.org/releases/download.html)
or with `wget http://llvm.org/releases/3.4.2/llvm-3.4.2.src.tar.gz`.
2. Extract with `tar -xf llvm-3.4.2.src.tar.gz`
3. Navigate to `lib/Transforms` in the LLVM source tree.
4. Clone the Checkpoint repository with
`git clone https://github.com/mlab/Checkpoint.git`

### Building Checkpoint as a plugin ###

1. Checkout the `dll` branch with `git checkout dll`. This branch is prepped
for creating a plugin for opt, while the `master` branch is prepped for building
a static library.
2. Modify the build files in `lib/Transforms` so Checkpoint is build along with
the other plugins.
    * In `LLVMBuild.txt`, add `Checkpoint` to the list of subdirectories.
    * In `CMakeLists.txt`, add an `add_subdirectory(Checkpoint)` entry.
    * In `Makefile`, add `Checkpoint` to the list of `PARALLEL_DIRS`.
3. Create a build directory, `mkdir ~/llvm-build`, and run the configure script
from the source directory in the new build directory:
`cd llvm-build && ../path/to/configure`.  Alternatively, you can use cmake
instead: `cd llvm-build && cmake ../path/to/source`. Check out the LLVM getting
started guide for build options that can be used with configure or cmake.
4. 
