# Scull

<img src="logo/logo.jpg" alt="SCULL Logo" height="150" title="Thanks, Vismita!">

Yet another systems programming language.

```
-include "io.scl"

fn main() : i32 {
  printf("Hello, World!\n")
  return 0
}
```

Compile and run:

```
# clone and cd to the repo
$ git clone https://github.com/scull-lang/scull.git
$ cd scull

# build llvm and sclc
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build . --target llvm_project
$ cmake --build . --target sclc
$ cmake --build . --target run-tests
$ cmake --build . --target examples

# compile the program
$ ./build/sclc-bin/sclc -i ./lib hello.scl

# run the compiled binary
$ ./hello
```

More examples in `./examples`.

## Inspiration

- [The C Programming Language](<https://en.wikipedia.org/wiki/C_(programming_language)>)
- [The Go Programming Language](https://go.dev)
- [How to build a compiler from scratch by Alex The Dev](https://youtu.be/HOe2YFnzO2I)

## Prerequisites

- [`cmake`](https://cmake.org) (3.20+)
- [`clang`](https://clang.llvm.org) (falls back to `cc`/`c++` if not found)
- [`ninja`](https://ninja-build.org)
