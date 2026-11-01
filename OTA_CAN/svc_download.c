#include "svc_download.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "uds_config.h"

/*
 * 0x34 RequestDownload
 * 0x36 TransferData
 * 0x37 RequestTransferExit
 *
 * Download/upload transfer framework.
 *
 * Downloads require an app callback (transfer_request/data/exit).
 * If no callback is registered or the app declines, the download
 * is rejected — there is no generic fallback.
 *
 * Uploads read from simulated ECU memory (no app callback needed).
 */

/* ── 0x34 RequestDownload ─────────────────────────────────────────── */

void svc_request_download(const uds_request_t *req, uds_response_t *resp) {
    transfer_ctx_t *xfer = uds_transfer_get();

    if (xfer->state != TRANSFER_IDLE) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
        resp->len = 3;
        return;
    }

    if (req->data_len < 3) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t data_fmt = req->data[0];
    uint8_t addr_len_fmt = req->data[1];
    uint8_t addr_bytes = addr_len_fmt & 0x0F;
    uint8_t size_bytes = (addr_len_fmt >> 4) & 0x0F;

    if (req->data_len < 2 + addr_bytes + size_bytes) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint32_t address = 0;
    for (uint8_t i = 0; i < addr_bytes; i++) {
        address = (address << 8) | req->data[2 + i];
    }

    uint32_t size = 0;
    for (uint8_t i = 0; i < size_bytes; i++) {
        size = (size << 8) | req->data[2 + addr_bytes + i];
    }

    /* Dispatch to app transfer callback */
    const uds_app_config_t *app = uds_get_app_config();
    if (app && app->transfer_request) {
        if (app->transfer_request(address, size, data_fmt, req, resp)) {
            /* App took over the transfer */
            xfer->state       = TRANSFER_APP_MANAGED;
            xfer->address     = address;
            xfer->total_size  = size;
            xfer->transferred = 0;
            xfer->block_seq   = 1;
            return;
        }
        /* App declined — check if it already set a NRC */
        if (resp->len > 0) return;
    }

    /* No app handler or app declined without setting a response */
    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
    resp->data[1] = req->sid;
    resp->data[2] = NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED;
    resp->len = 3;
}

/* ── 0x36 TransferData ────────────────────────────────────────────── */

void svc_transfer_data(const uds_request_t *req, uds_response_t *resp) {
    transfer_ctx_t *xfer = uds_transfer_get();

    if (xfer->state == TRANSFER_IDLE) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
        resp->len = 3;
        return;
    }

    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t block_seq = req->data[0];
    if (block_seq != xfer->block_seq) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_WRONG_BLOCK_SEQUENCE_COUNTER;
        resp->len = 3;
        return;
    }

    if (xfer->state == TRANSFER_APP_MANAGED) {
        /* Dispatch to app */
        uint16_t data_len = (req->data_len > 1) ? (req->data_len - 1) : 0;
        const uint8_t *data = &req->data[1];
        const uds_app_config_t *app = uds_get_app_config();

        if (app && app->transfer_data) {
            if (!app->transfer_data(data, data_len, block_seq, req, resp)) {
                /* App signaled error (resp already filled) */
                return;
            }
        }

        xfer->transferred += data_len;
        xfer->block_seq++;

        /* If app didn't fill response, provide default */
        if (resp->len == 0) {
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = block_seq;
            resp->len = 2;
        }

    } else {
        /* Upload: dispatch to app */
        uint16_t max_chunk = ISOTP_TX_BUF_SIZE - 2;
        const uds_app_config_t *app = uds_get_app_config();

        if (app && app->upload_data) {
            uint16_t chunk = 0;
            if (!app->upload_data(&resp->data[2], &chunk, max_chunk,
                                  block_seq, req, resp)) {
                /* App signaled error (resp already filled) */
                return;
            }
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = block_seq;
            resp->len = 2 + chunk;
            xfer->transferred += chunk;
        } else {
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = block_seq;
            resp->len = 2;
        }

        xfer->block_seq++;
    }
}

/* ── 0x37 RequestTransferExit ─────────────────────────────────────── */

void svc_transfer_exit(const uds_request_t *req, uds_response_t *resp) {
    transfer_ctx_t *xfer = uds_transfer_get();

    if (xfer->state == TRANSFER_IDLE) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
        resp->len = 3;
        return;
    }

    if (xfer->state == TRANSFER_APP_MANAGED) {
        /* Dispatch to app */
        const uds_app_config_t *app = uds_get_app_config();
        if (app && app->transfer_exit) {
            if (!app->transfer_exit(req, resp)) {
                /* App signaled error (resp filled) */
                uds_transfer_reset();
                return;
            }
        }
        /* If app didn't fill response, provide default */
        if (resp->len == 0) {
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->len = 1;
        }
        uds_transfer_reset();
        return;
    }

    /* Upload exit — dispatch to app */
    const uds_app_config_t *app = uds_get_app_config();
    if (app && app->upload_exit) {
        if (!app->upload_exit(req, resp)) {
            uds_transfer_reset();
            return;
        }
    }
    if (resp->len == 0) {
        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->len = 1;
    }
    uds_transfer_reset();
}
