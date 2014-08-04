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

This guide is written for commit `50f2f1434c74cc5ba20008bd097421a28d0f0738` on
the LLVM github mirror. It will be revised for stable LLVM 3.5 and future
versions of LLVM when they become available.

### Getting and building LLVM ###

You'll need Python, Git, a C/C++ compiler toolchain, and either gmake or cmake
for this part. The rest of this guide will assume you've started this process in
your home directory `~\`. First, get the LLVM source.
```
git clone https://github.com/llvm-mirror/llvm.git
cd llvm
git checkout 50f2f1
```
Make a build directory and generate build files using either the provided
configure script or with `cmake`.
```
mkdir ../build
cd ../build
```
and `cmake ../llvm/` or `../llvm/configure`.

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
