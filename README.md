# ShadowShell

A custom POSIX-like Unix shell written in C from first principles. 

ShadowShell is not a basic string-splitting wrapper. It implements a custom lexical scanner and Abstract Syntax Tree (AST) parser to correctly handle complex command hierarchies, operator precedence, and process lifecycle management directly via Linux system calls.

## Core Architecture

* **Custom Lexer:** Character-by-character tokenizer with lookahead, properly handling quotes, spaces, and multi-character operators (`&&`, `>>`, etc.).
* **AST Parser:** Recursively builds an Abstract Syntax Tree to enforce operator precedence and manage nested execution contexts.
* **Process Executor:** Directly interfaces with the Linux kernel using `fork()`, `execvp()`, and `waitpid()`.

## Features

* **Pipelines:** Chains multiple processes together routing `stdout` to `stdin` via kernel `pipe()` (e.g., `ls | grep .c | wc -l`).
* **I/O Redirection:** File descriptor manipulation via `dup2()` (`>`, `<`, `>>`, `>&`, etc.).
* **Subshells:** Isolated execution environments enclosed in parentheses (`(cd /tmp && pwd)`).
* **Logical Operators:** Short-circuit evaluation for `&&` and `||`.
* **Background Jobs:** Non-blocking execution using the `&` operator with real-time status reporting.
* **Builtins:** Native implementations of `cd`, and `exit`.

## Building and Running

Clone the repository and use the build script:

```bash
git clone https://github.com/ISMAILSHADOW/ShadowShell.git
cd ShadowShell
./build.sh
```

Run the shell:
```bash
./ShadowShell
```

Tests can be ran using:
```bash
./build.sh test && ./ShadowShell
```
