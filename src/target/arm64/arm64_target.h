/**
 * arm64_target.h - ARM64 target implementation
 */

#ifndef ARM64_TARGET_H
#define ARM64_TARGET_H

#include "../target.h"

// ARM64 target creation function
Target *target_create_arm64(void);

// ARM64-specific initialization
bool arm64_target_init(void);

#endif // ARM64_TARGET_H
