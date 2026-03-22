#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../ir/ir.h"

// Optimization levels
typedef enum {
    OPT_LEVEL_NONE = 0,
    OPT_LEVEL_O1 = 1,
    OPT_LEVEL_O2 = 2,
    OPT_LEVEL_O3 = 3
} OptLevel;

// Optimize IR module
IRModule *optimizer_optimize(IRModule *module, OptLevel level);

#endif // OPTIMIZER_H
