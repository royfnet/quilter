# quilter

**quilter** is a command-line tool and interactive shell for running, scripting, and testing various commands, with support for JSON-to-XML conversion, configuration management, and more. It is written in C++17 and is designed for extensibility and automation.

---

## Features

- Interactive shell and batch command execution
- JSON to XML conversion
- Configuration management (`set` command)
- Command scripting via files
- UTF-8/UTF-32 character code testing
- Easily extensible command system

---

## Building

**Requirements:**
- C++17 compatible compiler (tested with MSVC/Visual Studio 2022)
- CMake or Visual Studio project files

**To build with Visual Studio:**

1. Open `quilter.vcxproj` in Visual Studio 2022.
2. Build the solution.

**To build with CMake:**

1. Create a build directory:  
   `mkdir build && cd build`
2. Run CMake:  
   `cmake ..`
3. Build:  
   `cmake --build .`

---

## Usage

You can run `quilter.exe` interactively (no arguments) or pass a command and its arguments directly:

```
quilter.exe <command> [arguments...]
```

If no command is given, `quilter.exe` starts an interactive prompt.

---

## Commands

- **help**  
  Displays help for all commands or a specific command.  
  `help`  
  `help <command>`

- **exit**  
  Terminates all activities and exits the program.  
  `exit`

- **echo**  
  Echoes the parameters back to you.  
  `echo <param1> <param2> ...`

- **json**  
  Reads a JSON file and writes it out as XML.  
  `json <filename>`

- **set**  
  Sets or lists default configuration values.  
  `set <name> <value> [<name2> <value2> ...]`  
  `set`

- **test**  
  Interactive: echoes back character codes in UTF-8 and UTF-32 for input lines.  
  `test`

- **run**  
  Runs a command file (batch file of commands).  
  `run <filename>`

- **@**  
  Alias for `run`.  
  `@ <filename>`

For more information on each command, use the `help` command within the tool.

---

## Notes

- Commands can be chained in command files.
- If you run `quilter.exe` with command-line arguments, it executes the specified command and exits (unless the command is interactive).
- If no arguments are given, it starts an interactive prompt.
- The command system is extensible; new commands can be added by modifying the source (see `quilter.cpp`).

---

## Examples With Command-line Arguments

```
quilter.exe echo Hello World
quilter.exe json data.json
quilter.exe set AllowProblematicUnicode true
quilter.exe run script.cmd
```

---

## Extending

To add new commands, implement the command logic in the source (typically in `quilter.cpp`), and register the command in the command dispatch table.

---

## License

Copyright (c) 2025 Jeffrey Lomicka.  
All rights reserved.

---
