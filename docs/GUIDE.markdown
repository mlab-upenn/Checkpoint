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

![llvmdiff](http://i57.tinypic.com/2uq0ilf.png)

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
On the program above, we might generate a transformation like this:
```
declare i8 @check_anytime(...)

define float @function() {
  %1 = call i8 @check_anytime()
  %2 = icmp eq i8 %1, 0
  %out = select i1 %2, float 0x3FCC8E8A80000000, float 0x40D388CD80000000
  ret %out
}
```
Here, instead of returning a single number, we select between two numbers based
on the response from some decision-making function. The number we return could
represent a framerate, recursion depth, or some other natrual anytime control
parameter.

For algorithms that are not anytime by design, we can generate our own
estimation versions using techniques like loop perforation. Take this example
program:
```C
void function(float array[], int size) {
  for (int i = 0; i < size; i++)
    array[i] = array[i] / 2;
}
```
This example translates to the LLVM code below.
```
define void @function(float* %array, i32 %size) {
  %1 = icmp sgt i32 %size, 0
  br i1 %1, label %.lr.ph, label %._crit_edge

.lr.ph:
  %i.01 = phi i32 [ %5, %.lr.ph ], [ 0, %0 ]
  %2 = getelementptr float* %array, i32 %i.01
  %3 = load float* %2, align 4, !tbaa !1
  %4 = fmul float %3, 5.000000e-01
  store float %4, float* %2, align 4, !tbaa !1
  %5 = add nsw i32 %i.01, 1
  %6 = icmp slt i32 %5, %size
  br i1 %6, label %.lr.ph, label %._crit_edge

._crit_edge:
  ret void
} 
```
On each iteration of the loop, it loads a float value from the array, divides by
2, stores it back, adds 1 to the current index, and branches if the new index is
less than `%size`. Many computations of this form can produce results of
acceptable quality even when iterations of the loop are skipped
([note](people.csail.mit.edu/rinard/paper/sas11.pdf)). We can change the `add`
instruction that increments the loop counter from `%5 = add nsw i32 %i.01, 1` to
`%5 = add nsw i32 %i.01, 2` to skip every other iteration of the loop. The
resulting loop can have better performance at the expense of imprecision, and
might be a beneficial alternative in time-constrained situations.

The program itself doens't choose which execution path to take; that decision
is delegated to a separate controller program which communicates with the main
program through some IPC mechanism. After generating the alternate execution
paths in the main program, we instrument the main program with IPC calls to the
controller at the start of each set of alternate paths. The main program then
chooses one of the alternate paths based on a response from the controller
program. In the loop perforation example, we'd make an additional transformation
to get the loop increment from an external source.
```
%5 = add nsw i32 %i.01, 1
```
becomes
```
%choice = call i32 @get_anytime_choice()
%5 = add nsw i32 $i.01, i32 %choice
```
One way for the controller to choose the alternate path directly is to implement
the alternate paths as separate functions, with the controller swapping in and
out calls to the various implementations transparently. This has the advantage
of requiring less transformation to the main program, but limits the scope of
the possible alternative paths. Given a program with a plain function call:
```C
...
void function(&data, 100);
...
```
We transform our call like so:
```
extern void (*fp)(void*, int);
...
get_lock(fp_lock);
void (*call)(void*, int) = fp;
release_lock(fp_lock);
call($data, 100);
```
In addition to the call transformation, we run a controller that sets `fp`,
effectively choosing the execution path of the main thread. The various
implementations available to the controller can be generated with the same
anytime transformations used in the previous example.
```
void (*fp)(void*, int);

void impl1(void * data, int size) { ... }
void impl2(void * data, int size) { ... }
void impl3(void * data, int size) { ... }

void control_loop() {
  if (decision_logic) {
    get_lock(fp_lock);
    fp = &impl1;
  }
  else {
    get_lock(fp_lock);
    fp = &impl2;
  }
  release_lock(fp_lock);
}
```
Finally, the controller needs to measure the resource consumption and output of
the main program to make informed decisions about alternate execution paths.
This necessitates a further set of transformations to the main program that
send information about its resource usage to the controller. We have implemented
a compile-time transformation for LLVM that instruments code with hooks on each
function call and return. These hooks could be used to send messages to the
controller about the runtime of each function as soon as that function exits,
allowing the controller to make decisions based on the running time for each
alternate execution path.

Again, let's revisit the example function:
```C
void function(float array[], int size) {
  for (int i = 0; i < size; i++)
    array[i] = array[i] / 2;
}
```
And here is a diff of the function's LLVM code, with and without the hooks.

![checkpointdiff](http://i61.tinypic.com/2ai4zr8.png)

the LLVM pass inserts the calls to checkpoint with string parameters indicating
the function name and whether we are entering or exiting the function.
Additionally, if we're instrumenting the "main" function, calls to init and
deinit functions are inserted as well. 

The overall goal of the compile-time process is to take a single-strategy
program and turn it into a multiple-strategy program that communicates with
other programs for decision making.

### Runtime Control ###


