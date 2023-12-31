# Sypherc

**Sypherc** (with an *S*) is a compiler written in C, for a C-like language
called **Sypher**. Sypher is aimed to be a general-purpose statically typed
programming language. The [`examples`](https://github.com/Ruturajn/Sypherize/tree/main/examples)
directory contains some sample programs that demonstrate all the features that the
language currently supports. See [`Usage`](https://github.com/Ruturajn/Sypherize/tree/main#usage) for more details.

> **Note**
> 
> This project is currently under development, and a few fundamental features are supported as of now.
> As the project matures, there will be better documentation detailing the language and all the related
> intricacies.

<br>

## Overwiew

This project is my take on writing a compiler by following [`Lens_r's`](https://www.youtube.com/playlist?list=PLysa8wRFCssxGKj_RxBWr3rwmjEYlJIpa)
playlist on youtube which outlines the complete process
of designing and implementing a compiler.

<br>

## Build Instructions

Dependencies:
- `GCC`
- `Make`

Optional Dependencies for building docs:
- `Doxygen`
- `Latex`

Compiling this project:

- Clone the repository.
  ```
  $ git clone https://github.com/Ruturajn/Sypherize.git
  ```
- Navigate to the `Sypherize` directory and call make.
  ```
  $ make
  ```
- Use `$ make help` to look at available targets.

<br>

## Compiling the Generated x86_64 Assembly File

- Using `gcc`:
  ```
  $ gcc code_gen.s -o code_gen
  ```

- Using GNU binutils:

  Assemble into an object file, and link to create an executable.
  ```
  $ as code_gen.S -o code_gen.o
  $ ld code_gen.o -o code_gen
  ```
  For linking with the `C` library use,
  ```
  $ ld -o code_gen code_gen.o -lmsvcrt
  ```

<br>

## Usage

```
Sypherc 0.1
Ruturajn <nanotiruturaj@gmail.com>
A Compiler for Sypher

USAGE:
    sypherc [OPTIONS] INPUT_FILE

OPTIONS:
    -cc, --call-conv <CALLING_CONVENTION>
            A valid calling convention for code generation
            VALID CALLING CONVENTIONS:
            - `default`
            - `linux`
            - `windows`

    -ad, --asm-dialect <ASSEMBLY_DIALECT>
            A valid assembly dialect for code generation
            VALID ASSEMBLY DIALECTS:
            - `default`
            - `att`
            - `intel`

    -f, --format <OUTPUT_FORMAT>
            A valid output format for code generation
            VALID FORMATS:
            - `default`
            - `x86_64-gnu-as`

    -h, --help
            Print this help information

    -i, --input <INPUT_FILE_PATH>
            Path to the input file

    -o, --output <OUTPUT_FILE_PATH>
            Path to the output file

    -v, --version
            Print out current version of Sypherize

    -V, --verbose
            Print out extra debugging information

NOTE - Everything else is treated as an input file
```

<br>

## Miscellaneous

The file [`ROAD_MAP.md`](https://github.com/Ruturajn/Sypherize/blob/main/ROAD_MAP.md)
is a summary of what I picked up from the playlist, which includes a set of
steps, that describe the whole design. It's mainly for my understanding, but
you can definitely give it a read if you like.
