# Vulpes Language and Function Reference

This guide documents the Vulpes language syntax and the functions currently defined in the repository samples.

## Core Syntax
- Source files use the `.vlp` extension; line comments start with `//`.
- Literals: decimal integers (`42`), floating point (`3.14`), strings with `\"`, `\\`, `\n`, `\t` escapes, and booleans `true` / `false`.
- Expressions: unary minus (`-x`), arithmetic (`+ - * /`), comparisons (`== != < <= > >=`), assignment (`=`), function calls, and parenthesised sub‑expressions.
- Namespaced calls use `namespace.name(args)`; values cannot be namespaced without being called.

## Declarations and Types
- Variables: `var::type name = expression;` or `const::type name = expression;`. The `::type` part is optional; without it the compiler stores integers by default when no initializer is provided.
- `const` is parsed but not enforced at code generation time; treat const variables as write‑once by convention.
- Supported types map to LLVM IR as: `int -> i32`, `float -> double`, `bool -> i1`, `string -> i8*`, `void -> void`. Unrecognised types default to `int`.

## Functions
- Definition: `fx name(paramType [: paramName], ...) -> returnType { ... }`. The return type is optional and defaults to `void`.
- Parameter names are optional; if omitted, the compiler auto‑generates names (`p0`, `p1`, ...).
- Prototype (declaration only, no body): end the signature with `;`, e.g. `fx add(int:int, int:int) -> int;`.
- Return: `return expression;` or `return;` (the latter yields `void`).

## Control Flow
- Blocks are delimited with `{ ... }`.
- Conditional: `if (condition) { ... } else { ... }` (else is optional).
- While loop: `while (condition) { ... }`.
- For loop (range): `for i in start..end { ... }` iterates with `i` starting at `start` and continues while `i < end`.

## I/O and Built‑ins
- `print(...)`: with a leading string argument, braces `{}` are replaced left‑to‑right by subsequent arguments; if no string is given, a single argument is printed using `"{}"` as the format. A trailing newline is always added.
- `gather(name1, name2, ...)`: reads integers from stdin into the named variables (creating them as `int` initialized to `0` if they do not exist yet).
- `sqrt(x)`: returns the square root as `float` (`double` in IR).
- `rand(min, max)`: returns a pseudo‑random `int` in the inclusive range `[min, max]`; seeds automatically on first call.

## Modules
- Import another `.vlp` file with an alias: `mod("path/to/file.vlp")::alias;`. Imported functions are referenced via `alias.functionName(...)`.

## Sample Functions in This Repository
- `include.vlp`
  - `adder(int a, int b) -> int`: returns `a + b`.
  - `scaler(int value, int times) -> int`: repeats addition `times` times to scale `value`.
- `main.vlp`
  - `goldenRatio() -> float`: computes `(1 + sqrt(5)) / 2`.
  - `coolPrint(string s)`: prints a bannered string using `"======{}====="`.
  - `testArithmetic()`: demonstrates integer arithmetic and calls `goldenRatio()`.
  - `testComparisons()`: shows comparison operators on integers.
  - `testLoops()`: accumulates a range sum with `for` and counts down with `while`.
  - `testNamespacedCalls()`: exercises namespaced calls into the imported `math` module (`adder`, `scaler`).
  - `main()`: orchestrates the test suite; entry point when compiled.

## Minimal Example
```vlp
fx main() {
    var::int a = 5;
    var::int b = 7;
    if (a < b) {
        print("{} is less than {}", a, b);
    }
}
```
