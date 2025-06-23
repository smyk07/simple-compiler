# Simple Compiler

A Simple Compiler. (Compiles to x64 fasm assembly).

## Inspiration

[How to build a compiler from scratch by Alex The Dev](https://youtu.be/HOe2YFnzO2I)

## Prerequisites (to build the compiler)

- `make`
- `clang`
- `wildcard`
- `bear`

## Usage

Build the compiler:

```
make
```

Compile the examples in `./examples` like so:

```
./bin/sclc ./examples/n_prime_numbers.scl
```

Run the executable:

```
./examples/n_prime_numbers
```

Cleanup:

```
make clean
```
