A Lox compiler (from [Crafting Interpreters part
III](https://craftinginterpreters.com/a-bytecode-virtual-machine.html)).

## Differences from the book

* [Chapter 14 (Chunks of Bytecode)](https://craftinginterpreters.com/chunks-of-bytecode.html): arrays for bytes, numbers and values.
* [Chapter 15 (A Virtual Machine)](https://craftinginterpreters.com/a-virtual-machine.html): no big difference.
* [Chapter 16 (Scanning on Demand)](https://craftinginterpreters.com/scanning-on-demand.html): string interpolation (really implemented in chapter 19).
* [Chapter 17 (Compiling Expressions)](https://craftinginterpreters.com/compiling-expressions.html): simpler implementation of a Pratt parser from the original paper; with right-associative `**` for `pow()`.
* [Chapter 18 (Types of Values)](https://craftinginterpreters.com/types-of-values.html): use [NaN boxing](https://craftinginterpreters.com/optimization.html#nan-boxing) instead of tagged unions.
* [Chapter 19 (Strings)](https://craftinginterpreters.com/strings.html): use `*` for concatenation, and use an array of values to store objects rather than a linked list. Added `**` for strings, `'` for quoting values (_i.e._, turning them to strings), string interpolation, and flexible array members.
* [Chapter 20 (Hash Tables)](https://craftinginterpreters.com/hash-tables.html): use values for keys (including a special `VALUE_NONE`, different from `VALUE_NIL`, for keys that are not found), and deduplicate values in chunks with another hash table (which works for strings since they have been interned already). Then replaced hash tables with [hash array-mapped tries](https://infoscience.epfl.ch/record/64398?ln=en) (HAMTs).
