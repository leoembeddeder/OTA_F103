#include "svc_write_did.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "did_store.h"

#include <string.h>

/*
 * 0x2E WriteDataByIdentifier (library-generic version)
 *
 * Resolution order:
 *   1. did_store_read(did) — if found and writable, check access, write,
 *      call did_post_write_hook
 *   2. app did_write_hook — app handles factory DID write, etc.
 *   3. NRC_REQUEST_OUT_OF_RANGE if both fail
 */

void svc_write_data_by_id(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 3) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint16_t did = ((uint16_t)req->data[0] << 8) | req->data[1];
    const uint8_t *write_data = &req->data[2];
    uint8_t write_len = (uint8_t)(req->data_len - 2);

    const uds_app_config_t *app = uds_get_app_config();

    /* 1. Try did_store */
    const did_entry_t *entry = did_store_read(did);
    if (entry) {
        /* Check writable */
        if (!entry->writable) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_GENERAL_PROGRAMMING_FAILURE;
            resp->len = 3;
            return;
        }

        /* Access control */
        uds_session_t sess = uds_session_get();
        if (entry->access == DID_ACCESS_EXTENDED &&
            sess != UDS_SESSION_EXTENDED &&
            sess != UDS_SESSION_ENGINEERING &&
            sess != UDS_SESSION_PROGRAMMING) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
            resp->len = 3;
            return;
        }
        if (entry->access == DID_ACCESS_PROTECTED && !uds_security_is_unlocked()) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_SECURITY_ACCESS_DENIED;
            resp->len = 3;
            return;
        }

        if (!did_store_write(did, write_data, write_len)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
            resp->len = 3;
            return;
        }

        /* Post-write hook */
        if (app && app->did_post_write_hook) {
            app->did_post_write_hook(did, write_data, write_len);
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = (uint8_t)(did >> 8);
        resp->data[2] = (uint8_t)(did & 0xFF);
        resp->len = 3;
        return;
    }

    /* 2. Try app did_write_hook */
    if (app && app->did_write_hook) {
        if (app->did_write_hook(did, write_data, write_len, req, resp)) {
            return; /* App handled it (resp already filled) */
        }
    }

    /* 3. Not found */
    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
    resp->data[1] = req->sid;
    resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
    resp->len = 3;
}
