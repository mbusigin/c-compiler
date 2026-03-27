/**
 * wasm_target.h - WASM target implementation
 */

#ifndef WASM_TARGET_H
#define WASM_TARGET_H

#include "../target.h"

// WASM target creation function
Target *target_create_wasm(void);

// WASM-specific initialization
bool wasm_target_init(void);

#endif // WASM_TARGET_H
