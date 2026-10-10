#include "store_types.h"
#include <stddef.h>

static const store_persistence_t *s_persistence = NULL;

void store_set_persistence(const store_persistence_t *p) {
    s_persistence = p;
}

const store_persistence_t *store_get_persistence(void) {
    return s_persistence;
}
