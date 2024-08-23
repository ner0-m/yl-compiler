# Build a Compiler using LLVM

This is a small hobby project that is based on
[this](https://github.com/isuckatcs/how-to-compile-your-language)
implementation and tutorial.

This was a really nice project to dip into writing a compiler in general, but
also to emit LLVM IR and how to setup LLVM.

The compiler frontend even supports some more advanced features such control
flow analysis (detects assignment to immutable variables, uninitialized
variables, unreachable code and more) and basic constant folding.

For exampels of the language, checkout the `examples` directory.

## Build

If you want to try it out, you need make or ninja, CMake, a recent C++ compiler
(should support C++23 with `std::print`), and LLVM (> 18). I've tested the project
with CMake version 3.30.2, clang 18.1.3, GCC 14.2.1 and LLVM 18.1.3. 

then run the following commands to configure and build the project:

```bash
cmake -S . -B build
cmake --build build
```

You can compile and execute a .yl file with:

```bash
./build/yl -o fib examples/fibonacci_sequence.yl
./fib
```

If you want to see the generated LLVM IR, pass `-llvm-dump` to the compiler

You can run the tests, using `lit` (see e.g. [here](https://llvm.org/docs/CommandGuide/lit.html)).
Simply run:

```bash
cmake --build build --target check
```

which will build and run alles tests.
