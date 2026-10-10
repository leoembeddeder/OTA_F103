#include "svc_clear_dtc.h"
#include "uds_session.h"
#include "dtc_store.h"

/*
 * 0x14 ClearDiagnosticInformation
 * Request:  [14 groupHi groupMid groupLo]   (0xFFFFFF = all)
 * Response: [54]
 *
 * Requires extended or programming session.
 */

void svc_clear_dtc(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 3) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    /* Session check: require non-default session */
    uds_session_t sess = uds_session_get();
    if (sess == UDS_SESSION_DEFAULT) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
        resp->len = 3;
        return;
    }

    uint32_t group = ((uint32_t)req->data[0] << 16) |
                     ((uint32_t)req->data[1] << 8)  |
                     req->data[2];

    dtc_store_clear_group(group);

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->len = 1;
}
