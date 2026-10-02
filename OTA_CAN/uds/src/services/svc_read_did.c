#include "svc_read_did.h"
#include "svc_dyn_did.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "uds_config.h"
#include "did_store.h"

#include <string.h>

/*
 * 0x22 ReadDataByIdentifier (library-generic version)
 *
 * Resolution order:
 *   1. did_store_read(did)  — static/sensor DIDs
 *   2. DDID range (0xF200-0xF3FF) — dynamically defined DIDs
 *   3. app did_read_hook  — NV factory, FW meta, computed CAN IDs, etc.
 *   4. NRC_REQUEST_OUT_OF_RANGE if all fail
 */

static bool check_did_access(const did_entry_t *entry, uint8_t *nrc) {
    /* ISO 14229-1: identification DIDs (0xF180-0xF1FF) are always readable */
    if (entry->did >= 0xF180 && entry->did <= 0xF1FF) {
        return true;
    }

    uds_session_t sess = uds_session_get();

    switch (entry->access) {
    case DID_ACCESS_PUBLIC:
        return true;
    case DID_ACCESS_EXTENDED:
        if (sess == UDS_SESSION_EXTENDED ||
            sess == UDS_SESSION_ENGINEERING ||
            sess == UDS_SESSION_PROGRAMMING) {
            return true;
        }
        *nrc = NRC_CONDITIONS_NOT_CORRECT;
        return false;
    case DID_ACCESS_PROTECTED:
        if (!uds_security_is_unlocked()) {
            *nrc = NRC_SECURITY_ACCESS_DENIED;
            return false;
        }
        return true;
    }
    return true;
}

void svc_read_data_by_id(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 2 || (req->data_len % 2) != 0) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    const uds_app_config_t *app = uds_get_app_config();

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    uint16_t pos = 1;

    for (uint16_t i = 0; i < req->data_len; i += 2) {
        uint16_t did = ((uint16_t)req->data[i] << 8) | req->data[i + 1];

        /* 1. Try did_store */
        const did_entry_t *entry = did_store_read(did);
        if (entry) {
            uint8_t nrc;
            if (!check_did_access(entry, &nrc)) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = nrc;
                resp->len = 3;
                return;
            }

            if (pos + 2 + entry->len > ISOTP_TX_BUF_SIZE) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_RESPONSE_TOO_LONG;
                resp->len = 3;
                return;
            }

            resp->data[pos++] = (uint8_t)(did >> 8);
            resp->data[pos++] = (uint8_t)(did & 0xFF);
            memcpy(&resp->data[pos], entry->data, entry->len);
            pos += entry->len;
            continue;
        }

        /* 2. Try DDID range */
        if (did >= DDID_RANGE_START && did <= DDID_RANGE_END) {
            uint8_t ddid_buf[DID_MAX_DATA_LEN];
            uint8_t ddid_len = 0;
            if (ddid_read(did, ddid_buf, &ddid_len, sizeof(ddid_buf))) {
                if (pos + 2 + ddid_len > ISOTP_TX_BUF_SIZE) {
                    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                    resp->data[1] = req->sid;
                    resp->data[2] = NRC_RESPONSE_TOO_LONG;
                    resp->len = 3;
                    return;
                }
                resp->data[pos++] = (uint8_t)(did >> 8);
                resp->data[pos++] = (uint8_t)(did & 0xFF);
                memcpy(&resp->data[pos], ddid_buf, ddid_len);
                pos += ddid_len;
                continue;
            }
            /* DDID defined but read failed — fall through to app hook */
        }

        /* 3. Try app did_read_hook */
        if (app && app->did_read_hook) {
            uint8_t hook_buf[DID_MAX_DATA_LEN];
            uint8_t hook_len = 0;
            uint8_t hook_nrc = NRC_REQUEST_OUT_OF_RANGE;
            if (app->did_read_hook(did, hook_buf, &hook_len,
                                   sizeof(hook_buf), &hook_nrc)) {
                if (pos + 2 + hook_len > ISOTP_TX_BUF_SIZE) {
                    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                    resp->data[1] = req->sid;
                    resp->data[2] = NRC_RESPONSE_TOO_LONG;
                    resp->len = 3;
                    return;
                }
                resp->data[pos++] = (uint8_t)(did >> 8);
                resp->data[pos++] = (uint8_t)(did & 0xFF);
                memcpy(&resp->data[pos], hook_buf, hook_len);
                pos += hook_len;
                continue;
            }
            /* Hook returned false — use its NRC */
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = hook_nrc;
            resp->len = 3;
            return;
        }

        /* 4. Not found anywhere */
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3;
        return;
    }

    resp->len = pos;
}
