#include "svc_upload.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "uds_config.h"

/*
 * 0x35 RequestUpload
 *
 * Dispatches to the app upload_request callback.
 * If no callback is registered or the app declines, the request
 * is rejected — there is no library-provided default.
 */

void svc_request_upload(const uds_request_t *req, uds_response_t *resp) {
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

    /* Dispatch to app upload callback */
    const uds_app_config_t *app = uds_get_app_config();
    if (app && app->upload_request) {
        if (app->upload_request(address, size, data_fmt, req, resp)) {
            /* App accepted the upload */
            xfer->state       = TRANSFER_UPLOAD;
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
