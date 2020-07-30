# Coding Style

In general, follow the [Linux kernel coding style][1], with the following
exceptions:

- Add braces to every if, else, do, while, for and switch body, even for
single-line code blocks.
- Use spaces instead of tabs to align comments after declarations, as needed.
- Use tabs for leading alignments and spaces to align everything else.
- Use C89-style single line comments, `/*  */`. Don't use C99-style single line
comments (`//`).

[1]: https://www.kernel.org/doc/html/latest/process/coding-style.html
