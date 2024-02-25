A Lox compiler (from [Crafting Interpreters part
III](https://craftinginterpreters.com/a-bytecode-virtual-machine.html)).

## Differences from the book

* [Chapter 14 (Chunks of Bytecode)](https://craftinginterpreters.com/chunks-of-bytecode.html): arrays for bytes, numbers and values.
* [Chapter 15 (A Virtual Machine)](https://craftinginterpreters.com/a-virtual-machine.html): no big difference.
* [Chapter 16 (Scanning on Demand)](https://craftinginterpreters.com/scanning-on-demand.html): string interpolation (really implemented in chapter 19); some extra tokens for new operators and ∞.
* [Chapter 17 (Compiling Expressions)](https://craftinginterpreters.com/compiling-expressions.html): simpler implementation of a Pratt parser from the original paper; with right-associative `**` for `pow()`.
* [Chapter 18 (Types of Values)](https://craftinginterpreters.com/types-of-values.html): use [NaN boxing](https://craftinginterpreters.com/optimization.html#nan-boxing) instead of tagged unions.
* [Chapter 19 (Strings)](https://craftinginterpreters.com/strings.html): use `*` for concatenation, and use an array of values to store objects rather than a linked list. Added `**` for strings, `'` for quoting values (_i.e._, turning them to strings), `||` for string length and absolute value for numbers
(`|-|"foo" * "bar"|| = 6`), string interpolation, special values for the empty string (ε) and short strings
(6-character strings that can fit in a single value), and flexible array members.
* [Chapter 20 (Hash Tables)](https://craftinginterpreters.com/hash-tables.html): use values for keys (including a special `VALUE_NONE`, different from `VALUE_NIL`, for keys that are not found), and deduplicate values in chunks with another hash table (which works for strings since they have been interned already). Then replaced hash tables with [hash array-mapped tries](https://infoscience.epfl.ch/record/64398?ln=en) (HAMTs).
* [Chapter 21 (Global Variables)](https://craftinginterpreters.com/global-variables.html) and [Chapter 22 (Local Variables)](https://craftinginterpreters.com/local-variables.html): HAMTs for scopes, var object that keeps track of the variable name and whether it is writable (`var` is, `let` is not). Use a persistent HAMT for scope management (simply add to the HAMT when in a local scope, then revert to the previous version when leaving).
* [Chapter 23 (Jumping Back and Forth)](https://craftinginterpreters.com/jumping-back-and-forth.html): if and while without parens for predicate; for loop with parens, mostly like the book. Challenges: switch (with default and explicit fallthrough); TODO: break and continue.
* [Chapter 24 (Calls and Functions)](https://craftinginterpreters.com/calls-and-functions.html): WIP
