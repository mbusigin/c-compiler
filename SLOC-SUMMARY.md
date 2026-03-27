# SLOC Summary Report for C Compiler Project

## Executive Summary

**Total Source Lines of Code (SLOC): 14,068 lines**

This C compiler project contains approximately **14,000 lines of source code** distributed across 160 source files.

---

## SLOC Breakdown by Language

| Language | Files | Lines | Percentage |
|----------|-------|-------|------------|
| **C Source Files (.c)** | 130 | 12,958 | 92.1% |
| **C Header Files (.h)** | 30 | 1,110 | 7.9% |
| **TOTAL** | **160** | **14,068** | **100%** |

---

## SLOC Breakdown by Component

| Component | Files | Lines | Purpose |
|-----------|-------|-------|---------|
| **Compiler Source (src/)** | 62 | 12,010 | Main compiler implementation |
| **Standard Headers (include/)** | 6 | 172 | Minimal C standard library |
| **Test Cases (tests/)** | 92 | 1,886 | Integration tests & puzzles |

---

## Module Breakdown (src/)

| Module | Lines | Description |
|--------|-------|-------------|
| **Parser** | 3,132 | Parser & AST construction (26.1%) |
| **IR/Lowering** | 2,232 | Intermediate representation (18.6%) |
| **Backend** | 2,032 | Code generation (x86, WASM) (16.9%) |
| **Lexer** | 1,408 | Lexical analysis & preprocessing (11.7%) |
| **Tests** | 1,429 | Unit tests & test framework (11.9%) |
| **Semantic Analysis** | 782 | Type checking & symbol table (6.5%) |
| **Common/Utils** | 477 | Utilities & error handling (4.0%) |
| **Optimizer** | 39 | Constant folding & DCE (0.3%) |

---

## Largest Files (Top 10)

| File | Lines | Module |
|------|-------|--------|
| parser.c | 2,112 | Parser |
| lowerer.c | 1,983 | IR |
| codegen.c | 862 | Backend (x86) |
| wasm_codegen.c | 793 | Backend (WASM) |
| lexer.c | 611 | Lexer |
| preproc.c | 605 | Lexer |
| analyzer.c | 587 | Semantic Analysis |
| ast.c | 562 | Parser |
| test_codegen.c | 331 | Tests |
| driver.c | 236 | Main |

---

## Code Quality Metrics

### Code vs. Comments/Whitespace
- **Total Lines**: 12,010 (main source)
- **Code Lines**: ~10,471 (87.2%)
- **Blank Lines**: ~1,547 (12.9%)
- **Comment Lines**: ~1,269 (10.6%)

### Test Coverage
- **Test Files**: 92 files
- **Test Lines**: 1,886 lines
- **Test Ratio**: 1:6.4 (test lines : source lines)

---

## Project Context

### Size Classification
This project falls into the **"small compiler"** category:
- **Small compilers**: 5K-15K SLOC ✓
- **Medium compilers**: 15K-50K SLOC
- **Production compilers**: 1M+ SLOC

### Well-Structured Codebase
✓ Clear module separation  
✓ Good test coverage (92 test files)  
✓ Multiple backend targets (x86, WebAssembly)  
✓ Standard library implementation included  
✓ Comprehensive test suite with puzzles

---

## Additional Assets (Non-SLOC)

| Asset Type | Count | Purpose |
|------------|-------|---------|
| Markdown docs | 29 files | Documentation & project management |
| Makefile | 437 lines | Build configuration |
| Shell scripts | 351 lines | Build & validation utilities |
| LaTeX | 1 file | Paper/research documentation |

---

**Report Generated**: 2024  
**Repository**: /Users/mbusigin/c-compiler  
**Primary Language**: C (100% of source code)
