# Simple Compiler

A Simple Compiler. (Compiles to x64 fasm assembly).

Check [example.scl](https://github.com/smyk07/simple-compiler/blob/main/example.scl) for an example of how the language is written.

## Inspiration

[How to build a compiler from scratch by Alex The Dev](https://youtu.be/HOe2YFnzO2I)

## Usage

Compile the compiler:

```
make
```

Compile the `example.scl` file:

```
./compiler example.scl
```

Run the `./example` executable:

```
./example
```

Cleanup:

```
make clean-all              # clean all compile files
make clean-obj              # clean only the obj files
```
