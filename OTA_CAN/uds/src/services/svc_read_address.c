#include "svc_read_address.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include <string.h>

/* Validate accessible memory ranges for STM32F103 (Medium/High density) */
static bool is_valid_read_address(uint32_t addr, uint32_t size) {
	/* 32-bit addition overflow protection */
	if ((addr + size) < addr) {
		return false;
	}
	/* SRAM: 0x20000000 to 0x20005000 (20KB) */
	if ((addr >= 0x20000000U) && ((addr + size) <= 0x20005000U)) {
		return true;
	}
	/* Flash: 0x08000000 to 0x08010000 (64KB) */
	if ((addr >= 0x08000000U) && ((addr + size) <= 0x08010000U)) {
		return true;
	}
	return false;
}

void svc_read_memory_by_address(const uds_request_t *req, uds_response_t *resp) {
	/* 1. Header length validation (minimum 3 bytes: ALFID + 1-byte addr + 1-byte size) */
	if (req->data_len < 3U) {
	resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	resp->data[1] = req->sid;
	resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
	resp->len = 3U;
	return;
	}

	/* 2. Decode ALFID */
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

	/* 3. Strict message length check */
	if (req->data_len != (uint16_t)(1U + addr_len + size_len)) {
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

	/* 5. Address range and non-zero size validation */
	if ((size == 0U) || !is_valid_read_address(address, size)) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
	    resp->len = 3U;
	    return;
	}

	/* 6. Verify response fits in ISO-TP TX buffer */
	if ((1U + size) > ISOTP_TX_BUF_SIZE) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_RESPONSE_TOO_LONG;
	    resp->len = 3U;
	    return;
	}

	/* 7. Positive response: 0x63 + raw memory content */
	resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid); /* 0x63 */
	memcpy(&resp->data[1], (const void *)address, size);
	resp->len = (uint16_t)(1U + size);
}
