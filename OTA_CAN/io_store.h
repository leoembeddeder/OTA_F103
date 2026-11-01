#ifndef IO_STORE_H
#define IO_STORE_H

#include "store_types.h"

typedef struct {
    uint16_t id;
    uint8_t  value[IO_OUTPUT_MAX_SIZE];
    uint8_t  default_value[IO_OUTPUT_MAX_SIZE];
    uint8_t  size;              /* 1, 2, or 4 bytes              */
    bool     requires_security; /* Needs security unlock         */
    bool     frozen;            /* Tester has frozen this output  */
} io_output_t;

/** Reset the I/O output store (clears all entries). */
void io_store_init(void);

/** Add an I/O output to the store. */
void io_store_add(uint16_t id, uint8_t size, const uint8_t *defval, bool sec);

/** Find output by ID.  Returns NULL if not found. */
io_output_t *io_store_find(uint16_t id);

/** Get output count. */
uint8_t io_store_count(void);

/** Get output by index. */
io_output_t *io_store_get_by_index(uint8_t index);

#endif /* IO_STORE_H */
