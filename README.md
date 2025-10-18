# Simple Compiler

A Simple Compiler. (Compiles to x64 fasm assembly).
Only linux is supported for now.

## Inspiration

[How to build a compiler from scratch by Alex The Dev](https://youtu.be/HOe2YFnzO2I)

## Prerequisites for building

- `make`
- `clang`
- `bear` (for `compile_commands.json`)

## Tools required

- `fasm`

## Usage

Build and install the compiler:

```
make install
```

Compile the examples in `./examples` like so:

```
sclc -i ./lib ./examples/n_prime_numbers.scl
```

Run the executable:

```
./examples/n_prime_numbers
```

Cleanup:

```
make clean
```
