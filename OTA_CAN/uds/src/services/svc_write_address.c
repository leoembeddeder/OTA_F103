#include "svc_write_address.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include <string.h>

/* Check if security is unlocked */
extern bool uds_security_is_unlocked(void);

void svc_write_memory_by_address(const uds_request_t *req, uds_response_t *resp) {
	/* 1. Header length check (ALFID + min 1-byte addr + min 1-byte size + min 1-byte data) */
	if (req->data_len < 4U) {
	resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	resp->data[1] = req->sid;
	resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
	resp->len = 3U;
	return;
	}

	/* 2. Security Access verification */
	if (!uds_security_is_unlocked()) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_SECURITY_ACCESS_DENIED;
	    resp->len = 3U;
	    return;
	}

	/* 3. Decode ALFID */
	uint8_t alfid = req->data[0];
	uint8_t addr_len = (uint8_t)(alfid & 0x0FU);
	uint8_t size_len = (uint8_t)((alfid >> 4U) & 0x0FU);

	if ((addr_len < 1U) || (addr_len > 4U) || (size_len < 1U) || (size_len > 4U)) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
	    resp->len = 3U;
	    return;
	}

	uint16_t header_len = (uint16_t)(1U + addr_len + size_len);
	if (req->data_len <= header_len) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
	    resp->len = 3U;
	    return;
	}

	/* 4. Decode big-endian address and size */
	uint32_t address = 0U;
	uint32_t size = 0U;
	for (uint8_t i = 0U; i < addr_len; i++) {
	    address = (address << 8U) | (uint32_t)req->data[1U + i];
	}
	for (uint8_t i = 0U; i < size_len; i++) {
	    size = (size << 8U) | (uint32_t)req->data[1U + addr_len + i];
	}

	/* 5. Verify payload length matches specified memorySize */
	uint16_t payload_len = (uint16_t)(req->data_len - header_len);
	if (payload_len != size) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
	    resp->len = 3U;
	    return;
	}

	/* 6. Address bounds check: protect vector table & critical memory */
	/* Example: Allow writable calibration/configuration RAM buffer */
	if ((address < 0x20002000U) || ((address + size) > 0x20005000U)) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
	    resp->len = 3U;
	    return;
	}

	/* 7. Perform memory write */
	memcpy((void *)address, &req->data[header_len], size);

	/* 8. Positive response: 0x7D + echo of ALFID, Address, and Size */
	resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid); /* 0x7D */
	memcpy(&resp->data[1], &req->data[0], header_len);
	resp->len = (uint16_t)(1U + header_len);
}
