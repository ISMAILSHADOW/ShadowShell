# ShadowShell

A Unix shell written from scratch in C. Custom tokenizer, recursive-descent
parser, and direct `fork` / `execvp` / `waitpid` process management with job
control.

This is a learning project. It is my first non-trivial C program, written to
understand how a shell talks to the operating system: how a process is created,
how its file descriptors are set up, how process groups and job control work,
and how signals travel between parent and child. I wrote the tokenizer
and parser by hand rather than pulling in a library because I wanted to
understand every step between a keystroke and a `fork()`.

## Demo

    $ ls | grep .c | wc -l
    7
    $ (cd /tmp && pwd)
    /tmp
    $ sleep 30 &
    [0] 12345
    $ jobs
    [0] is running. PID: 12345 PGID: 12345
    $ fg
    ^C Interrupt
    $ exit

## What it supports

- Pipelines with any number of stages: `a | b | c`
- I/O redirection with optional file descriptors:
  `<`, `>`, `>>`, `<&`, `>&`, `&>`, `&>>`, `<>`
- Subshells: `(cd /tmp && ls)`
- Logical operators with short-circuit evaluation: `&&`, `||`
- Background execution with `&`
- Job control: process groups, `tcsetpgrp`, `SIGTSTP` handling,
  `fg`, `bg`, `jobs`
- Builtins: `cd`, `exit`, `true`, `false`, `fg`, `bg`, `jobs`
- Quoted strings in command arguments: `'...'` and `"..."`

## What it does not support

Deliberate non-goals, not oversights:

- Environment variables: `$VAR`, `$?`, `$$`, `$!`
- Command substitution: `$(...)`, backticks
- Glob expansion: `*.c`
- Variable assignment: `FOO=bar cmd`
- Control flow: `if`, `for`, `while`, `case`
- Functions, aliases, `trap`, `set`
- Here-documents and here-strings (removed from the grammar on purpose)
- Escape sequences inside strings: `\n`, `\"`
- Line continuation: `\` at end of line

## How to read the code

Start at `main.c`. The whole shell is driven by one call:

    int ret = execute_command(line);

`execute_command` lives at the bottom of `executor.c` and does four things:
parse, execute, clean up. Follow those three calls.

1. `run_parser(line)` in `ast.c` calls `init_parser`, which calls
   `init_scanner` in `tokenizer.c` and `scan_token()` to get the first token,
   then `command_list()` to build the AST. The precedence chain is
   `command_list` -> `logical` -> `negation` -> `pipeline` -> `command`.
   Each layer handles one operator precedence level. The AST node type is a
   tagged union in `ast.h`.

2. `execute_ast(root)` is a switch on `node->type`. Commands and subshells
   go through `run_subprocess`. Pipelines go through `run_pipeline`, which
   sets up `pipe()`s and `fork()`s each stage into the same process group.
   Background nodes call `run_subprocess` with `background = true`, which
   skips the wait.

3. **Arena allocator** (`DataStructures/Arenas/Arena.c`): The code uses an Arena allocator to simplify memory management. 
one `malloc` at
  startup, a bump pointer, a reset per command. This was much faster than manually using malloc and free on each object individually and much easier to handle. I previously had several memory leaks which the arena resolved.

4. `arena_reset(&command_arena)`. Every allocation during a single command
   (tokens, AST nodes, argument strings, redirect arrays, the backup fd
   table) lives in one arena. It is reset once per line. Nothing is freed
   individually.



If you want the two files that carry the interesting mechanics: `executor.c`
(how a shell talks to the kernel) and `tokenizer.c` (how a shell reads input
without a regex library).


## Building And Running
```bash
./build.sh &&  ./ShadowShell
```
Run a script:
```bash
./ShadowShell script.sh
```
Tests:
```bash
./build.sh test && ./ShadowShell
```
Scripts can also be run directly with a shebang line:
```bash
#!/path/to/ShadowShell
```
## Roadmap

I plan to add ASAN (address sanitizer) to my arena implementation and build a fuzzer with AFL++ for the shell later on as my goal is to work towards security research.