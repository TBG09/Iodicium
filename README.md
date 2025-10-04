# Iodicium Documents
### This documentation will tell you basically everything you need to know.
## Some notes
- This documentation version covers version 1.5.6 (build 0168).
- The command-line tool is the primary way to interact with the Iodicium toolchain.


## Command Structure
 
The Iodicium executable follows a standard command-line structure with global options and specific subcommands for different actions like compiling and running code.
 
```
iodicium [global-options] <command> [command-options]
```
 
### Global Options
These options can be used with the base `iodicium` command.
 
| Flag              | Description                                                  |
|-------------------|--------------------------------------------------------------|
| `-d`, `--debug`   | Enable debug output for compilation and the runtime process. |
| `-v`, `--version` | Show version information and exit.                           |
| `-h`, `--help`    | Show the main help message.                                  |
 
---
 
### Commands
 
#### `compile`
Compiles an Iodicium project into an executable (`.iode`) or a library (`.iodl`).
 
**Usage:** `iodicium compile <project> [options]`
 
| Argument/Option     | Description                                                  |
|---------------------|--------------------------------------------------------------|
| `<project>`         | **(Required)** Path to the `Iodicium.toml` project file.     |
| `-ob`, `--obfuscate`| Obfuscate variable names in the compiled output.             |
| `-h`, `--help`      | Show the help message for the `compile` command.             |
 
#### `run`
Executes a compiled Iodicium executable (`.iode`) file.
 
**Usage:** `iodicium run <file> [options]`
 
| Argument/Option     | Description                                                  |
|---------------------|--------------------------------------------------------------|
| `<file>`            | **(Required)** The `.iode` file to execute.                  |
| `--memory <limit>`  | Set the VM memory limit (e.g., `256M`, `1G`).                |
| `-h`, `--help`      | Show the help message for the `run` command.                 |

---

## Code Documentation

This section details the syntax and features of the Iodicium language itself.

### Comments

Iodicium supports two styles of single-line comments. The compiler will ignore any text following `//` or `#` until the end of the line.

```iodicium
// This is a comment.
# This is also a comment.

#import "path" // The # is only special for imports at the start of a line.
```

### Keywords

The language reserves a few keywords for specific functionality:

| Keyword  | Description                               |
|----------|-------------------------------------------|
| `def`    | Defines a function.                       |
| `var`    | Declares a mutable (re-assignable) variable. |
| `val`    | Declares an immutable (read-only) variable.  |
| `return` | Returns a value from a function.          |

### Variables

Variables can be declared as either mutable (`var`) or immutable (`val`). Type annotations and initial values are both optional.

```iodicium
// A mutable variable with an explicit type and initial value.
var myMutableNumber: Number = 100
myMutableNumber = 200 // This is valid.

// An immutable variable. Its type is inferred from the value.
val myImmutableString = "Hello, World!"
// myImmutableString = "New" // This would cause a compile-time error.

// A variable declaration without an initial value.
val myUninitializedVar: String
```

### Functions

Functions are defined using the `def` keyword. Parameter and return type annotations are optional.

#### Function Definition
A function with a body is defined using curly braces `{}`.

```iodicium
// A simple function with two parameters and an explicit return type.
def add(a: Number, b: Number): Number {
    return a + b
}

// A function without parameters or a return value.
def sayHello() {
    writeOut("Hello World")
    flush()
}
```

#### Function Declaration
You can also declare a function without a body. This is useful for defining an interface that will be implemented elsewhere, such as in native code.

```iodicium
// Declares a function 'nativePrint' that takes one 'String' argument.
def nativePrint(message: String)
```

### Modules and Exports

Iodicium has a module system that allows you to control which functions and variables are visible outside of a file.

*   `#import "path"`: Imports another Iodicium file. The path is relative to the project file.
*   `@export`: Place this annotation before a `def`, `val`, or `var` to make it accessible to other files that import it.
*   `@exportall`: This module-level annotation exports all top-level declarations that follow it within the file.

```iodicium
// --- my_module.iod ---
@exportall // Everything below will be exported

val PI = 3.14159

def privateHelper() { /* ... */ }

@export // This is redundant due to @exportall, but still valid
def sayHi(name: String) {
    print("Hi, " + name)
}

// --- main.iod ---
#import "my_module.iod"

print(PI) // Access the exported constant
sayHi("Iodicium") // Call the exported function
```