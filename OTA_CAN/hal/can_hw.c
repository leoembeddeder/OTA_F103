#include "can.h"
#include "can_hw.h"


static isotp_channel_t s_phys;

static bool isotp_tx_cb(const can_frame_t *f, void *user);
static void seed_did_store(void);





/* ── function definition ────────────────── */


bool can_hw_send(const can_frame_t *frame) 
{ 
    if (CAN_TX(frame->id, (uint8_t*)frame->data, frame->dlc) != HAL_OK)
    {
        return false;
    }
	return true; 
}


static bool isotp_tx_cb(const can_frame_t *f, void *user) {
    (void)user;
    return can_hw_send(f);
}


/* ── App config (registered with uds_server_init) ────────────────── */

static const uds_app_config_t s_app_cfg = {
    //.did_read_hook    = ota_did_read,
    //.transfer_request = ota_request_cb,
    //.transfer_data    = ota_data_cb,
    //.transfer_exit    = ota_exit_cb,
};

static void seed_did_store(void) {
    did_store_init();
    static const char *spare_part = "stm32f103-checkpoint-4b";
    did_store_add(0xF187, (const uint8_t *)spare_part,
                  (uint8_t)strlen(spare_part), false, DID_ACCESS_PUBLIC);
    static const char *sw_version = "0.1.0-lxy-over-uds";
    did_store_add(0xF195, (const uint8_t *)sw_version,
                  (uint8_t)strlen(sw_version), false, DID_ACCESS_PUBLIC);
    static const char *vendor = "tr-stm32f103";
    did_store_add(0xF18C, (const uint8_t *)vendor,
                  (uint8_t)strlen(vendor), false, DID_ACCESS_PUBLIC);
}


void UDS_APP(void)
{
	dtc_store_init();
	dtc_nvm_init();
	/* Add DTC definitions here */
	dtc_store_add(0x010000, 0x00, NULL, 0, NULL, 0);
	/* Restore statuses from wear-leveled flash */
	dtc_store_restore_from_nv();

    isotp_init(&s_phys, ISOTP_RX_ID, ISOTP_TX_ID, isotp_tx_cb, NULL);
    seed_did_store();
    uds_server_init(&s_app_cfg);


	while(1)
	{
		isotp_poll(&s_phys);

		if (isotp_rx_ready(&s_phys)) {
			uint16_t req_len;
			const uint8_t *req = isotp_rx_data(&s_phys, &req_len);
		
			printf("UDS req: %02x", req[0]);
			for (uint16_t i = 1; i < req_len && i < 4; i++) printf(" %02x", req[i]);
			if (req_len > 4) printf(" …(%u B)", req_len);
			printf("\n");
			
			uds_response_t resp;
			bool send = uds_server_process(req, req_len, false, &resp);
			isotp_rx_done(&s_phys);
		
			if (send) {
				printf("UDS rsp: %02x (%u B)\n", resp.data[0], resp.len);
				
				isotp_send(&s_phys, resp.data, resp.len);
			}
		}

		uds_server_poll();

	}
}



/* Receive FIFO 0 message pending interrupt management */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef hdr;
	can_frame_t rx;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, rx.data) == HAL_OK)
    {
    	rx.id = hdr.StdId;
		rx.dlc = hdr.DLC;

		if (rx.id == ISOTP_RX_ID || rx.id == ISOTP_FUNC_RX_ID) {
			isotp_on_rx(&s_phys, &rx);
		}
    }

}

