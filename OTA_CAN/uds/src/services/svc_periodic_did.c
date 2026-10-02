#include "svc_periodic_did.h"
#include "uds_session.h"
#include "uds_config.h"

/*
 * 0x2A ReadDataByPeriodicIdentifier
 * Request:  [2A transmissionMode PID1 PID2 ...]
 *   0x01 = slow (~1Hz), 0x02 = medium (~5Hz), 0x03 = fast (~10Hz), 0x04 = stop
 * Response: [6A transmissionMode]
 *
 * PIDs are 1-byte periodic identifiers (lower byte of DID 0xF4xx).
 * Actual periodic transmission handled in main loop.
 */

void svc_read_periodic_id(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t mode = req->data[0];

    if (mode == 0x04) {
        /* Stop all periodic transmissions */
        uds_periodic_clear_all();
        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = mode;
        resp->len = 2;
        return;
    }

    if (mode < 0x01 || mode > 0x03) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3;
        return;
    }

    if (req->data_len < 2) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    /* Rate index: 0=slow, 1=medium, 2=fast */
    periodic_rate_t *rate = uds_periodic_get(mode - 1);
    if (!rate) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
        resp->len = 3;
        return;
    }

    /* Add PIDs to the rate list */
    for (uint16_t i = 1; i < req->data_len && rate->count < PERIODIC_MAX_PIDS; i++) {
        uint8_t pid = req->data[i];
        /* Check for duplicates */
        bool dup = false;
        for (uint8_t j = 0; j < rate->count; j++) {
            if (rate->pids[j] == pid) { dup = true; break; }
        }
        if (!dup) {
            rate->pids[rate->count++] = pid;
        }
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = mode;
    resp->len = 2;
}
