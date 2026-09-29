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
* **Builtins:** Native implementations of `cd`, `echo`, and `exit`.

## Building and Running

Ensure you have `gcc` installed. Clone the repository and compile the source files directly:

```bash
git clone https://github.com/ISMAILSHADOW/ShadowShell.git
cd ShadowShell
gcc main.c ast.c executor.c globals.c tokenizer.c -o ShadowShell
```

Run the shell:
```bash
./shadowshell
```
