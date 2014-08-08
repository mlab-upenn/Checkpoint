# Lifetime of an Anytime Program #

Our goal for the anytime CPS project at mLab is to design a process and
associated tools for making anytime programs. This process will be applicable
for both anytime-by-design programs and programs written as traditional
algorithms that we want to adapt for anytime computation. In our design,
preparing a program for anytime operation happens throughout the lifetime of the
program, from design through to execution. Generally, the process is as follows:

1. __Design time__
    * Program written
    * Anytime control points identified
2. __Compile time__
    * Program instrumented with analysis & communication
    * Anytime transformations made to provided control points
3. __Run time__
    * Characterization of program output and resource consumption
    * Control program operation to meet resource constraints

Some work on the tools necessary for this process is in progress; I'll explain
how our current progress fits into this framework and what still needs to be
done to realize a fully-automated pipeline for anyime program creation.

### Input Program ###

Our process takes as input a program that has a number of promising
opportunities for anytime adaptation identified and annotated. Because these
annotations will be used to guide programmatic transformation of the input
program, they must be machine-readable in some way. Our current tools leverage
the LLVM suite, so candidate annotation techniques need to interoperate with
LLVM easily.

One candidate annotation technique (for C and C++ programs) is the
GNU annotate attribute. Consider the C source code:
```C
float function() {
  __attribute__((annotate("My number!")))
  float number;
  number = 2e4 + 3.21;
  return number;
}
```
The `__attribute__` above the number declaration designates that number with a
string that can be recovered in LLVM. Below is a diff of the corresponding LLVM
code for the function above, with and without the attribute line.

![llvmdiff](/docs/llvmdiff.png)

Notice how the inclusion of the annotation added an LLVM intrinsic right before
our use of `number`, with a reference to the string we included. This addition
doesn't modify program behavior at all, but we can use it to trigger our own
anytime transformations using hints in the string.

### Compilation Process ###

During the compile process, we use the hints provided by the input user to take
a section of the program and generate estimation-quality implementations of
that section as an alternate execution path. For an algorithm that already has
anytime properties, the estimation-quality path might set a single quality
metric low, while the precision-quality path might set the quality metric high.
`EXAMPLE OF LLVM ALTERNATE STORE PATTERN`
For algorithms that are not anytime by design, we can generate our own
estimation versions using techniques like loop perforation.
`EXAMPLE OF LLVM LOOP PERFORATION`

The program itself doens't choose which execution path to take; that decision
is delegated to a separate controller program which communicates with the main
program through some IPC mechanism. After generating the alternate execution
paths in the main program, we instrument the main program with IPC calls to the
controller at the start of each set of alternate paths. The main program then
chooses one of the alternate paths based on a response from the controller
program.
`EXMAPLE OF IPC CALL INSERTION`

One way for the controller to choose the alternate path directly is to implement
the alternate paths as separate functions, with the controller swapping in and
out calls to the various implementations transparently. This has the advantage
of requiring less transformation to the main program, but limits the scope of
the possible alternative paths.
`EXAMLE OF FUNCTION SWAP`

Finally, the controller needs to measure the resource consumption and output of
the main program to make informed decisions about alternate execution paths.
This necessitates a further set of transformations to the main program that
send information about its resource usage to the controller. We have implemented
a compile-time transformation for LLVM that instruments code with hooks on each
function call and return. These hooks could be used to send messages to the
controller about the runtime of each function as soon as that function exits,
allowing the controller to make decisions based on the running time for each
alternate execution path.
`EXAMPLE OF CHECKPOINT`

The overall goal of the compile-time process is to take a single-strategy
program and turn it into a multiple-strategy program that communicates with
other programs for decision making.

### Runtime Control ###


