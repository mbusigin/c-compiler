#ifndef REGALLOC_H
#define REGALLOC_H

#include <stddef.h>

// Register allocation
typedef struct {
    int virtual_reg;
    int physical_reg;  // -1 = spilled
    int spill_offset;
} RegAllocation;

// Allocate registers for a function
void regalloc_allocate(void *function);

#endif // REGALLOC_H
