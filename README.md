# Checkpoint #

This is a pass for LLVM that instruments LLVM code with checkpoints for
measuring runtime. The goal is to provide on-line profiling for runtime
analysis of the client. 

Checkpoint uses a simple API to notify the runtime
analyzer when the client program has entered or exited a function, and to allow
the runtime to do basic initialization and cleanup.

### Build and Use ###

Consult the LLVM docs to get up and running with compiling LLVM passes. This
pass compiles using the same steps presented in the "Hello World" example, with
some modifications detailed [here](http://uu-kk.blogspot.com/2012/02/llvm-pass\
-on-windows-integrating-with.html). Detailed build instructions are available
in the `INSTALL.md` file. Finished client programs need to implement the
checkpoint API, which is constantly evolving to provide more information to the
runtime library. Currently, the checkpoint API consists of three callbacks:

```C
void initialize()
void checkpoint(const char *, const char *)
void print_results()
```

`checkpoint` is currently called with either `"Entering "` or `"Exiting "` and
the name of the current function, in that order.  In this way, a runtime can
trace the execution of the program in realtime.  Runtimes should make sure the
`checkpoint` call is reletively fast so program performance is not adversely
impacted.

### Contact ###

gland at seas dot upenn dot edu

feel free to contact me with questions.
