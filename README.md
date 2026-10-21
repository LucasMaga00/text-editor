# Text editor in C

A small terminal text editor built in C as a learning project. It supports
editing and saving files, scrolling, incremental search, and syntax highlighting
for C/C++ files (`.c`, `.h`, and `.cpp`), including multiline comments.

## Build and run

Requires Make, Clang with C23 support, and a POSIX-compatible terminal.

```sh
make
./main
```

Start with `./main` (or `make run`) to create a new document. To open an existing
file, pass its path:

```sh
./main test.txt
```

## Controls

| Key | Action |
| --- | --- |
| Arrow keys | Move the cursor |
| Home / End | Jump to the start / end of a line |
| Page Up / Page Down | Scroll by a page |
| Enter | Insert a newline |
| Backspace / Delete | Delete text or join lines |
| Ctrl+S | Save; prompts for a filename for a new document |
| Ctrl+F | Search; use arrows to navigate matching lines |
| Enter / Esc during search | Accept the match / cancel and restore the cursor |
| Ctrl+Q | Quit; repeat when warned about unsaved changes |

## Tests and cleanup

```sh
make test   # Run editor regression tests
make clean  # Remove build and debug outputs
```
