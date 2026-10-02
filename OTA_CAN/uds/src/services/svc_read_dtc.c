#include "svc_read_dtc.h"
#include "dtc_store.h"

#include <string.h>

/*
 * 0x19 ReadDTCInformation
 * Sub-functions:
 *   0x01 - reportNumberOfDTCByStatusMask
 *   0x02 - reportDTCByStatusMask
 *   0x04 - reportDTCSnapshotRecordByDTCNumber
 *   0x06 - reportDTCExtDataRecordByDTCNumber
 */

void svc_read_dtc_info(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t sub = req->data[0];
    uint8_t avail_mask = dtc_store_get_status_mask();

    switch (sub) {
    case 0x01: {
        /* reportNumberOfDTCByStatusMask */
        if (req->data_len < 2) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }
        uint8_t req_mask = req->data[1];
        uint16_t count = dtc_store_count_by_mask(req_mask);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = avail_mask;
        resp->data[3] = 0x01; /* DTC format: ISO 14229-1 */
        resp->data[4] = (uint8_t)(count >> 8);
        resp->data[5] = (uint8_t)(count & 0xFF);
        resp->len = 6;
        break;
    }

    case 0x02: {
        /* reportDTCByStatusMask */
        if (req->data_len < 2) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }
        uint8_t req_mask = req->data[1];

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = avail_mask;
        uint16_t pos = 3;

        uint8_t total = dtc_store_count();
        for (uint8_t i = 0; i < total; i++) {
            const dtc_entry_t *e = dtc_store_get_by_index(i);
            if (!e || !(e->status & req_mask)) continue;

            if (pos + 4 > ISOTP_TX_BUF_SIZE) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_RESPONSE_TOO_LONG;
                resp->len = 3;
                return;
            }

            resp->data[pos++] = (uint8_t)(e->dtc >> 16);
            resp->data[pos++] = (uint8_t)(e->dtc >> 8);
            resp->data[pos++] = (uint8_t)(e->dtc & 0xFF);
            resp->data[pos++] = e->status;
        }

        resp->len = pos;
        break;
    }

    case 0x04: {
        /* reportDTCSnapshotRecordByDTCNumber */
        if (req->data_len < 4) { /* sub + 3-byte DTC */
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }
        uint32_t dtc = ((uint32_t)req->data[1] << 16) |
                        ((uint32_t)req->data[2] << 8)  |
                        req->data[3];

        const dtc_entry_t *e = dtc_store_find(dtc);
        if (!e) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(e->dtc >> 16);
        resp->data[3] = (uint8_t)(e->dtc >> 8);
        resp->data[4] = (uint8_t)(e->dtc & 0xFF);
        resp->data[5] = e->status;
        uint16_t pos = 6;

        if (e->has_snapshot) {
            resp->data[pos++] = 0x01; /* Snapshot record number */
            if (pos + e->snapshot_len <= ISOTP_TX_BUF_SIZE) {
                memcpy(&resp->data[pos], e->snapshot, e->snapshot_len);
                pos += e->snapshot_len;
            }
        }

        resp->len = pos;
        break;
    }

    case 0x06: {
        /* reportDTCExtDataRecordByDTCNumber */
        if (req->data_len < 4) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }
        uint32_t dtc = ((uint32_t)req->data[1] << 16) |
                        ((uint32_t)req->data[2] << 8)  |
                        req->data[3];

        const dtc_entry_t *e = dtc_store_find(dtc);
        if (!e) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(e->dtc >> 16);
        resp->data[3] = (uint8_t)(e->dtc >> 8);
        resp->data[4] = (uint8_t)(e->dtc & 0xFF);
        resp->data[5] = e->status;
        uint16_t pos = 6;

        if (e->has_extdata) {
            resp->data[pos++] = 0x01; /* Extended data record number */
            if (pos + e->extdata_len <= ISOTP_TX_BUF_SIZE) {
                memcpy(&resp->data[pos], e->extdata, e->extdata_len);
                pos += e->extdata_len;
            }
        }

        resp->len = pos;
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
