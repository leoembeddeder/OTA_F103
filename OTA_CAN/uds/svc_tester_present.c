#include "svc_tester_present.h"
#include "uds_session.h"

/*
 * 0x3E TesterPresent
 * Request:  [3E sub]     sub: 00 = normal, 80 = suppress positive response
 * Response: [7E sub]
 */

void svc_tester_present(const uds_request_t *req, uds_response_t *resp) {
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

    if (sub != 0x00) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3;
        return;
    }

    /* Refresh session timer */
    uds_session_refresh();

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->len = 2;
    resp->suppress = suppress;
}
