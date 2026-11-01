#ifndef UDS_CALLBACKS_H
#define UDS_CALLBACKS_H

#include "uds_types.h"

/*
 * Application callback typedefs and configuration struct.
 *
 * These allow the UDS library to call into application-specific code
 * without compile-time dependencies on app modules.
 */

/* ── DID hooks ────────────────────────────────────────────────────── */

/**
 * Called when did_store doesn't have the DID.
 * App resolves NV factory, FW meta, computed CAN IDs, etc.
 * Returns true if handled; fills out_data/out_len.
 * On failure, set *out_nrc to the appropriate NRC.
 */
typedef bool (*uds_did_read_fn)(uint16_t did,
                                uint8_t *out_data, uint8_t *out_len,
                                uint8_t max_len, uint8_t *out_nrc);

/**
 * Called when did_store doesn't have the DID for writing.
 * App handles factory DID write path, etc.
 * Returns true if handled.
 */
typedef bool (*uds_did_write_fn)(uint16_t did,
                                 const uint8_t *data, uint8_t len,
                                 const uds_request_t *req,
                                 uds_response_t *resp);

/**
 * Called after a successful did_store write (post-write hook).
 * App can handle side effects (e.g., address change on FD10 write).
 */
typedef void (*uds_did_post_write_fn)(uint16_t did,
                                      const uint8_t *data, uint8_t len);


/* ── Transfer hooks (for app-managed downloads like FW OTA) ───────── */

/**
 * Called during RequestDownload.
 * Return true to take over the transfer (state becomes TRANSFER_APP_MANAGED).
 * Return false to let the library handle generically.
 */
typedef bool (*uds_transfer_request_fn)(uint32_t address, uint32_t size,
                                        uint8_t data_fmt,
                                        const uds_request_t *req,
                                        uds_response_t *resp);

/**
 * Called for each TransferData block when app-managed.
 * Return true on success, false to abort (resp already filled with NRC).
 */
typedef bool (*uds_transfer_data_fn)(const uint8_t *data, uint16_t len,
                                     uint8_t block_seq,
                                     const uds_request_t *req,
                                     uds_response_t *resp);

/**
 * Called on TransferExit when app-managed.
 * Return true on success, false on failure (resp filled with NRC).
 */
typedef bool (*uds_transfer_exit_fn)(const uds_request_t *req,
                                     uds_response_t *resp);

/* ── Upload hooks (for app-managed uploads) ──────────────────────── */

/**
 * Called during RequestUpload.
 * Return true to accept the upload (state becomes TRANSFER_UPLOAD).
 * Return false to reject. If declining, optionally fill resp with NRC.
 */
typedef bool (*uds_upload_request_fn)(uint32_t address, uint32_t size,
                                       uint8_t data_fmt,
                                       const uds_request_t *req,
                                       uds_response_t *resp);

/**
 * Called for each TransferData block during upload.
 * Write up to max_len bytes into out_data, set *out_len to actual count.
 * Return true on success, false on error (fill resp with NRC).
 */
typedef bool (*uds_upload_data_fn)(uint8_t *out_data, uint16_t *out_len,
                                    uint16_t max_len,
                                    uint8_t block_seq,
                                    const uds_request_t *req,
                                    uds_response_t *resp);

/**
 * Called on TransferExit for upload.
 * Return true on success, false on error (fill resp with NRC).
 */
typedef bool (*uds_upload_exit_fn)(const uds_request_t *req,
                                    uds_response_t *resp);

/* ── Application configuration struct ─────────────────────────────── */

typedef struct {
    /* DID hooks — called when did_store doesn't have the DID */
    uds_did_read_fn         did_read_hook;
    uds_did_write_fn        did_write_hook;
    uds_did_post_write_fn   did_post_write_hook;

    /* Security — NULL = use built-in XOR with secret bytes */
    const uint8_t           *security_secret;
    uint8_t                  security_secret_len;

    /* Transfer — for app-managed downloads (FW OTA) */
    uds_transfer_request_fn  transfer_request;
    uds_transfer_data_fn     transfer_data;
    uds_transfer_exit_fn     transfer_exit;

    /* Upload — for app-managed uploads */
    uds_upload_request_fn    upload_request;
    uds_upload_data_fn       upload_data;
    uds_upload_exit_fn       upload_exit;
} uds_app_config_t;

/**
 * Get the current app config (set via uds_server_init).
 * Returns NULL if not initialized.
 */
const uds_app_config_t *uds_get_app_config(void);

#endif /* UDS_CALLBACKS_H */
