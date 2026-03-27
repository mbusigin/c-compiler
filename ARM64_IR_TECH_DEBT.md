# ARM64-Specific IR Opcodes - Technical Debt Analysis

## Current State

The IR (Intermediate Representation) layer currently contains ARM64 register-specific opcodes that violate target independence:

### Problematic Opcodes

1. **IR_SAVE_X8** - Save ARM64 register x8
2. **IR_RESTORE_X8_RESULT** - Restore result from ARM64 register x8
3. **IR_SAVE_X8_TO_X20** - Save x8 to x20 (preserve across reload)
4. **IR_RESTORE_X8_FROM_X20** - Restore x8 from x20
5. **IR_SAVE_X8_TO_X22** - Save x8 to x22 (for struct member access)
6. **IR_ADD_X21** - Add using ARM64 register x21

### Locations

- **src/ir/ir.h** - Opcode definitions (lines 24-35)
- **src/ir/ir.c** - Opcode string conversions (lines 56-74)
- **src/ir/lowerer.c** - Usage in lowering (18+ locations)
- **src/backend/codegen.c** - ARM64 codegen handling (lines 570-749)
- **src/backend/wasm_codegen.c** - WASM codegen handling (lines 461-706)

## Why This Is a Problem

1. **Violates target independence**: IR should be platform-agnostic
2. **Prevents portability**: Cannot add new targets without understanding ARM64 registers
3. **Complicates optimization**: Optimizer must handle target-specific details
4. **Makes testing harder**: IR dump shows ARM64-specific details

## Recommended Solution

### Phase 1: Generic Register Allocation IR

Replace ARM64-specific opcodes with target-independent alternatives:

```c
// Instead of: IR_SAVE_X8, IR_RESTORE_X8_RESULT
IR_SPILL_REGISTER,    // Spill register to stack slot
IR_RELOAD_REGISTER,   // Reload register from stack slot

// Instead of: IR_SAVE_X8_TO_X20, IR_RESTORE_X8_FROM_X20
IR_COPY,              // Copy value between virtual registers

// Instead of: IR_ADD_X21
IR_ADD,               // Already exists, just use it correctly
```

### Phase 2: Virtual Register System

Implement virtual register allocation:

1. Lowerer generates IR with unlimited virtual registers
2. Register allocator assigns physical registers (x8, x20, etc.) for ARM64
3. Different registers for WASM (local indices)
4. Each target implements its own register allocation strategy

### Implementation Steps

1. **Analyze current usage**: Understand why each ARM64-specific opcode is needed
2. **Design replacement**: Create target-independent IR operations
3. **Update lowerer**: Replace ARM64-specific opcodes with generic ones
4. **Update backends**: ARM64 and WASM codegen handle generic opcodes
5. **Add register allocator**: Map virtual registers to physical registers per target
6. **Test thoroughly**: Ensure all 52 tests still pass

## Estimated Effort

- **Analysis**: 2-4 hours
- **Implementation**: 8-12 hours
- **Testing**: 4-6 hours
- **Total**: 14-22 hours

## Status

❌ **Not Yet Implemented** - Requires significant refactoring

This technical debt should be addressed before Sprint 6 (Self-Hosting) to ensure clean target abstraction.
