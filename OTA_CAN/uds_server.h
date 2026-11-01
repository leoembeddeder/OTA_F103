#ifndef UDS_SERVER_H
#define UDS_SERVER_H

#include "uds_types.h"
#include "uds_callbacks.h"

/**
 * Initialise UDS server with application config.
 * Installs base library services and initialises state.
 * Pass NULL for config to use defaults (no app hooks).
 */
void uds_server_init(const uds_app_config_t *config);

/**
 * Register an additional service (or override a library-provided one).
 * Returns true on success, false if the service table is full.
 */
bool uds_server_register(const uds_service_entry_t *entry);

/**
 * Process a raw UDS request (after ISO-TP reassembly).
 * Fills resp with positive or negative response.
 * Returns true if a response should be sent (false = suppress).
 */
bool uds_server_process(const uint8_t *data, uint16_t len,
                        bool functional, uds_response_t *resp);

/**
 * Call periodically from main loop for session/security timers.
 */
void uds_server_poll(void);

#endif /* UDS_SERVER_H */
