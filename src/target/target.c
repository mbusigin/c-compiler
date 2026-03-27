/**
 * target.c - Target registry and common functions
 */

#include "target.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>

// Target registry entry
typedef struct TargetRegistryEntry {
    char *name;
    TargetCreator creator;
    struct TargetRegistryEntry *next;
} TargetRegistryEntry;

static TargetRegistryEntry *target_registry = NULL;

// Register a target
void target_register(const char *name, TargetCreator creator) {
    TargetRegistryEntry *entry = xmalloc(sizeof(TargetRegistryEntry));
    entry->name = xstrdup(name);
    entry->creator = creator;
    entry->next = target_registry;
    target_registry = entry;
}

// Create a target by name
Target *target_create_by_name(const char *name) {
    for (TargetRegistryEntry *entry = target_registry; entry; entry = entry->next) {
        if (strcmp(entry->name, name) == 0) {
            return entry->creator();
        }
    }
    return NULL;
}

// Get list of available target names
const char **target_get_available_names(int *count) {
    int n = 0;
    for (TargetRegistryEntry *entry = target_registry; entry; entry = entry->next) {
        n++;
    }
    
    const char **names = xmalloc(sizeof(char*) * (n + 1));
    int i = 0;
    for (TargetRegistryEntry *entry = target_registry; entry; entry = entry->next) {
        names[i++] = entry->name;
    }
    names[n] = NULL;
    
    *count = n;
    return names;
}

// Destroy a target
void target_destroy(Target *target) {
    if (!target) return;
    
    target_cleanup(target);
    
    if (target->private_data) {
        free(target->private_data);
    }
    
    free(target);
}
