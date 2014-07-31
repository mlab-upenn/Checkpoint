# Building Checkpoint #

This file details building Checkpoint as a plugin to LLVM's opt tool as well as
including Checkpoint in a statically-linked build of opt. In the future, I hope
to include instructions for building a standalone Checkpoint executable.

The instructions for building a plugin come mostly from LLVM's own pass building
tutorial, found [online](llvm.org/docs/WritingAnLLVMPass.html) or included with
the LLVM soruce. Instructions for building a static library for linking into opt
or a standalone tool come from a combination of the aformentioned tutorial, and
a [blog post](http://uu-kk.blogspot.com/2012/02/llvm-pass-on-windows-integratin\
g-with.html) on the topic. Most of the information is reproduced here directly
in the event that the remote resorces become unavailable.

This guide was tested on LLVM 3.4.2, the latest (stable) version of LLVM at the
time of writing.

### Getting the Sources ###

1. Download the LLVM source from [here](llvm.org/releases/download.html) or with
`wget http://llvm.org/releases/3.4.2/llvm-3.4.2.src.tar.gz`.
2. Extract with `tar -xf llvm-3.4.2.src.tar.gz`
3. Navigate to `lib/Transforms` in the LLVM source tree.
4. Clone the Checkpoint repository with
`git clone https://github.com/mlab/checkpoint.git`

