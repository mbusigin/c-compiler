This directory contains small C puzzle tests for compiler development.

Categories:
- `run-*`: compile and run; expected output is noted in the file comment.
- `sem-*`: compile successfully; semantic target is noted in the file comment.
- `diag-*`: should produce a diagnostic or represents undefined behavior that
  a compiler may warn about.
- `algo-*`: algorithm and data-structure oriented compile-and-run tests.
- `adv-*`: trees, hashing, graph traversal, and dynamic programming tests.
- `stress-*`: parser, sema, codegen, memory-model, and constant-eval stress
  tests.
- `neg-*`: negative and edge-case algorithm tests.

Most `run-*` files print a concrete result. A few rely on common 64-bit
assumptions and say so explicitly.
