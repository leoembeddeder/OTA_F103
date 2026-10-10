#include "svc_dyn_did.h"
#include "uds_session.h"
#include "uds_config.h"
#include "did_store.h"

#include <string.h>

/*
 * 0x2C DynamicallyDefineDataIdentifier
 * Sub 0x01 - defineByIdentifier:
 *   [2C 01 DDIDhi DDIDlo srcDIDhi srcDIDlo position size ...]
 * Sub 0x03 - clearDynamicallyDefinedDataIdentifier:
 *   [2C 03 DDIDhi DDIDlo]
 *
 * DDID range: 0xF200-0xF3FF
 */

/* DDID source definition */
typedef struct {
    uint16_t src_did;
    uint8_t  position;   /* 1-based byte offset in source DID */
    uint8_t  size;       /* Bytes to extract */
} ddid_source_t;

typedef struct {
    uint16_t      ddid;
    ddid_source_t sources[DDID_MAX_SOURCES];
    uint8_t       source_count;
    bool          active;
} ddid_entry_t;

static ddid_entry_t s_ddids[DDID_MAX_ENTRIES];
static bool s_ddid_inited = false;

static void ddid_init(void) {
    if (s_ddid_inited) return;
    memset(s_ddids, 0, sizeof(s_ddids));
    s_ddid_inited = true;
}

static ddid_entry_t *ddid_find(uint16_t ddid) {
    for (uint8_t i = 0; i < DDID_MAX_ENTRIES; i++) {
        if (s_ddids[i].active && s_ddids[i].ddid == ddid) return &s_ddids[i];
    }
    return NULL;
}

static ddid_entry_t *ddid_alloc(void) {
    for (uint8_t i = 0; i < DDID_MAX_ENTRIES; i++) {
        if (!s_ddids[i].active) return &s_ddids[i];
    }
    return NULL;
}

/* Read a DDID by compositing source DIDs.  Called from ReadDataByIdentifier. */
bool ddid_read(uint16_t ddid, uint8_t *out, uint8_t *out_len, uint8_t max_len) {
    ddid_init();
    ddid_entry_t *e = ddid_find(ddid);
    if (!e) return false;

    uint8_t pos = 0;
    for (uint8_t i = 0; i < e->source_count; i++) {
        const did_entry_t *src = did_store_read(e->sources[i].src_did);
        if (!src) return false;

        uint8_t spos = e->sources[i].position - 1; /* Convert 1-based to 0-based */
        uint8_t slen = e->sources[i].size;
        if (spos + slen > src->len) return false;
        if (pos + slen > max_len) return false;

        memcpy(&out[pos], &src->data[spos], slen);
        pos += slen;
    }
    *out_len = pos;
    return true;
}

void svc_dyn_define_did(const uds_request_t *req, uds_response_t *resp) {
    ddid_init();

    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t sub = req->data[0];

    if (sub == 0x01) {
        /* defineByIdentifier */
        if (req->data_len < 7) { /* sub(1) + DDID(2) + at least one source(4) */
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        uint16_t ddid = ((uint16_t)req->data[1] << 8) | req->data[2];

        /* Validate DDID range */
        if (ddid < DDID_RANGE_START || ddid > DDID_RANGE_END) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        /* Parse source definitions: each is 4 bytes (srcDID(2) + position(1) + size(1)) */
        uint16_t src_data_len = req->data_len - 3;
        if (src_data_len % 4 != 0) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        uint8_t num_sources = (uint8_t)(src_data_len / 4);
        if (num_sources > DDID_MAX_SOURCES) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        /* Find or allocate DDID entry */
        ddid_entry_t *e = ddid_find(ddid);
        if (!e) {
            e = ddid_alloc();
            if (!e) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
                resp->len = 3;
                return;
            }
        }

        e->ddid = ddid;
        e->source_count = num_sources;
        e->active = true;

        for (uint8_t i = 0; i < num_sources; i++) {
            uint16_t off = 3 + i * 4;
            e->sources[i].src_did  = ((uint16_t)req->data[off] << 8) | req->data[off + 1];
            e->sources[i].position = req->data[off + 2];
            e->sources[i].size     = req->data[off + 3];
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(ddid >> 8);
        resp->data[3] = (uint8_t)(ddid & 0xFF);
        resp->len = 4;

    } else if (sub == 0x03) {
        /* clearDynamicallyDefinedDataIdentifier */
        if (req->data_len < 3) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        uint16_t ddid = ((uint16_t)req->data[1] << 8) | req->data[2];
        ddid_entry_t *e = ddid_find(ddid);
        if (!e) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3;
            return;
        }

        e->active = false;

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(ddid >> 8);
        resp->data[3] = (uint8_t)(ddid & 0xFF);
        resp->len = 4;

    } else {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3;
    }
}
