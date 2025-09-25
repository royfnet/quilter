# quilter

`quilter` is a command-line tool and interactive shell for running, scripting, and testing
various commands, with support for JSON-to-XML conversion, configuration management, and more.

## Usage

You can run `quilter.exe` interactively (no arguments) or pass a command and its arguments directly:

```
quilter.exe <command> [arguments...]
```

If no command is given, `quilter.exe` starts an interactive prompt.

---

## Commands

### `help`

Displays help for all commands or a specific command.

```
help
help <command>
```

### `exit`

Terminates all activities and exits the program.

```
exit
```

---

### `echo`

Echoes the parameters back to you.

```
echo <param1> <param2> ...
```

--

### `json`

Reads a JSON file and writes it out as XML.

```
json <filename>
```

---

### `set`

Sets or lists default configuration values.

If no parameters are given, lists all current default values.


```
set <name> <value> [<name2> <value2> ...]
set
```

---

### `test`

Interactive: echoes back character codes in UTF-8 and UTF-32 for input lines.

```
test
```

You will be prompted to enter lines; the tool will display their character codes.

---

### `run`

Runs a command file (batch file of commands).

```
run <filename>
```

Each line in the file is executed as a command.

---

### `@`

Alias for `run`. You can use `@filename` to run commands from a file.

```
@ <filename>
```

---

## Notes

- Commands can be chained in command files.
- If you run `quilter.exe` with command-line arguments, it executes the specified command and exits (unless the command is interactive).
- If no arguments are given, it starts an interactive prompt.
- The command system is extensible; new commands can be added by modifying the source.

---

## Example

```
quilter.exe echo Hello World
quilter.exe json data.json
quilter.exe set AllowProblematicUnicode true
quilter.exe run script.cmd
```

---

For more information on each command, use the `help` command within the tool.
