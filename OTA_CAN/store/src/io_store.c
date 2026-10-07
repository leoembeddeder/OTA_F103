#include "io_store.h"
#include <string.h>

static io_output_t s_outputs[IO_OUTPUT_MAX];
static uint8_t     s_output_count = 0;

void io_store_init(void) {
    s_output_count = 0;
}

void io_store_add(uint16_t id, uint8_t size, const uint8_t *defval, bool sec) {
    if (s_output_count >= IO_OUTPUT_MAX) return;
    io_output_t *o = &s_outputs[s_output_count++];
    memset(o, 0, sizeof(*o));
    o->id = id;
    o->size = (size > IO_OUTPUT_MAX_SIZE) ? IO_OUTPUT_MAX_SIZE : size;
    memcpy(o->value, defval, o->size);
    memcpy(o->default_value, defval, o->size);
    o->requires_security = sec;
    o->frozen = false;
}

io_output_t *io_store_find(uint16_t id) {
    for (uint8_t i = 0; i < s_output_count; i++) {
        if (s_outputs[i].id == id) return &s_outputs[i];
    }
    return NULL;
}

uint8_t io_store_count(void) {
    return s_output_count;
}

io_output_t *io_store_get_by_index(uint8_t index) {
    if (index >= s_output_count) return NULL;
    return &s_outputs[index];
}
