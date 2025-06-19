# Simple Compiler

A Simple Compiler. (Compiles to x64 fasm assembly).

Check [example.scl](https://github.com/smyk07/simple-compiler/blob/main/example.scl) for an example of how the language is written.

## Inspiration

[How to build a compiler from scratch by Alex The Dev](https://youtu.be/HOe2YFnzO2I)

## Prerequisites

- `make`
- `clang`

## Usage

Build the compiler:

```
make
```

Compile the examples in `./examples` like so:

```
./sclc ./examples/n_prime_numbers.scl
```

Run the executable:

```
./examples/n_prime_numbers
```

Cleanup:

```
make clean
```
