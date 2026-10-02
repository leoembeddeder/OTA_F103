#include "svc_io_control.h"
#include "uds_session.h"
#include "io_store.h"

#include <string.h>

/*
 * 0x2F InputOutputControlByIdentifier
 * Request:  [2F outputIDhi outputIDlo controlOption [controlData] [mask]]
 *
 * Control options:
 *   0x00 - returnControlToECU (unfreeze, ECU controls)
 *   0x01 - resetToDefault
 *   0x02 - freezeCurrentState
 *   0x03 - shortTermAdjustment (set value, optionally masked)
 *
 * Response: [6F outputIDhi outputIDlo controlOption currentValue...]
 */

void svc_io_control(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 3) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint16_t out_id = ((uint16_t)req->data[0] << 8) | req->data[1];
    uint8_t control = req->data[2];

    io_output_t *out = io_store_find(out_id);
    if (!out) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3;
        return;
    }

    /* Security check */
    if (out->requires_security && !uds_security_is_unlocked()) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SECURITY_ACCESS_DENIED;
        resp->len = 3;
        return;
    }

    switch (control) {
    case 0x00:
        /* Return control to ECU */
        out->frozen = false;
        break;

    case 0x01:
        /* Reset to default */
        memcpy(out->value, out->default_value, out->size);
        out->frozen = false;
        break;

    case 0x02:
        /* Freeze current state */
        out->frozen = true;
        break;

    case 0x03: {
        /* Short-term adjustment */
        if (req->data_len < 3 + out->size) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }
        const uint8_t *ctrl_data = &req->data[3];

        /* Check for optional mask */
        if (req->data_len >= 3 + out->size * 2) {
            const uint8_t *mask = &req->data[3 + out->size];
            for (uint8_t i = 0; i < out->size; i++) {
                out->value[i] = (out->value[i] & ~mask[i]) | (ctrl_data[i] & mask[i]);
            }
        } else {
            memcpy(out->value, ctrl_data, out->size);
        }
        out->frozen = true;
        break;
    }

    default:
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3;
        return;
    }

    /* Build positive response with current value */
    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = (uint8_t)(out_id >> 8);
    resp->data[2] = (uint8_t)(out_id & 0xFF);
    resp->data[3] = control;
    memcpy(&resp->data[4], out->value, out->size);
    resp->len = 4 + out->size;
}
