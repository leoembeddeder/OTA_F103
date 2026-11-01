#include "svc_link_ctrl.h"
#include "uds_session.h"
#include "uds_config.h"

/*
 * 0x87 LinkControl
 * Sub 0x01 - verifyModeTransitionWithFixedParameter (baud rate ID)
 * Sub 0x02 - verifyModeTransitionWithSpecificParameter (3-byte baud)
 * Sub 0x03 - transitionMode (apply verified baud)
 *
 * Fixed baud rate IDs: 0x10=125k, 0x11=250k, 0x12=500k, 0x13=1000k
 */

static uint32_t fixed_baud_to_kbps(uint8_t id) {
    switch (id) {
    case 0x10: return 125;
    case 0x11: return 250;
    case 0x12: return 500;
    case 0x13: return 1000;
    default:   return 0;
    }
}

void svc_link_control(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t sub_raw = req->data[0];
    bool suppress = (sub_raw & 0x80) != 0;
    uint8_t sub = sub_raw & 0x7F;

    switch (sub) {
    case 0x01: {
        /* Verify fixed baud rate */
        if (req->data_len < 2) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        uint32_t baud = fixed_baud_to_kbps(req->data[1]);
        if (baud == 0) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        uds_link_set_pending_baud(baud);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->len = 2;
        resp->suppress = suppress;
        break;
    }

    case 0x02: {
        /* Verify specific baud rate (3-byte value in kbps) */
        if (req->data_len < 4) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        uint32_t baud = ((uint32_t)req->data[1] << 16) |
                         ((uint32_t)req->data[2] << 8) |
                         req->data[3];

        if (baud < 10 || baud > 1000) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        uds_link_set_pending_baud(baud);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->len = 2;
        resp->suppress = suppress;
        break;
    }

    case 0x03: {
        /* Transition to verified baud rate */
        if (!uds_link_has_pending()) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3;
            return;
        }

        /* In a real ECU we'd reconfigure the CAN controller here.
         * For the simulator, we just acknowledge the transition. */
        uds_link_clear_pending();

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->len = 2;
        resp->suppress = suppress;
        break;
    }

    default:
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3;
        break;
    }
}
