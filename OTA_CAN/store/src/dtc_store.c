#include "dtc_store.h"
#include <string.h>

static dtc_entry_t s_dtcs[DTC_MAX_ENTRIES];
static uint8_t     s_dtc_count = 0;

void dtc_store_init(void) {
    s_dtc_count = 0;
}

void dtc_store_add(uint32_t dtc, uint8_t status,
                   const uint8_t *snap, uint8_t snap_len,
                   const uint8_t *ext, uint8_t ext_len) {
    if (s_dtc_count >= DTC_MAX_ENTRIES) return;
    dtc_entry_t *e = &s_dtcs[s_dtc_count++];
    memset(e, 0, sizeof(*e));
    e->dtc    = dtc & 0x00FFFFFF;
    e->status = status;

    if (snap && snap_len > 0) {
        e->has_snapshot = true;
        e->snapshot_len = (snap_len > DTC_SNAPSHOT_SIZE) ? DTC_SNAPSHOT_SIZE : snap_len;
        memcpy(e->snapshot, snap, e->snapshot_len);
    }
    if (ext && ext_len > 0) {
        e->has_extdata = true;
        e->extdata_len = (ext_len > DTC_EXTDATA_SIZE) ? DTC_EXTDATA_SIZE : ext_len;
        memcpy(e->extdata, ext, e->extdata_len);
    }
}

void dtc_store_restore_from_nv(void) {
    const store_persistence_t *p = store_get_persistence();
    if (!p || !p->dtc_load) return;

    for (uint8_t i = 0; i < s_dtc_count; i++) {
        uint8_t nv_status = p->dtc_load(s_dtcs[i].dtc);
        if (nv_status != 0xFF) {
            s_dtcs[i].status = nv_status;
        }
    }
}

uint16_t dtc_store_count_by_mask(uint8_t status_mask) {
    uint16_t count = 0;
    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtcs[i].status & status_mask) {
            count++;
        }
    }
    return count;
}

const dtc_entry_t *dtc_store_get_by_index(uint8_t index) {
    if (index >= s_dtc_count) return NULL;
    return &s_dtcs[index];
}

const dtc_entry_t *dtc_store_find(uint32_t dtc) {
    uint32_t d = dtc & 0x00FFFFFF;
    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtcs[i].dtc == d) return &s_dtcs[i];
    }
    return NULL;
}

uint8_t dtc_store_count(void) {
    return s_dtc_count;
}

uint8_t dtc_store_get_status_mask(void) {
    return 0xFF;
}

void dtc_store_clear_all(void) {
    const store_persistence_t *p = store_get_persistence();
    uint8_t n = (s_dtc_count <= DTC_MAX_ENTRIES) ? s_dtc_count : DTC_MAX_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        s_dtcs[i].status = 0;
        if (p && p->dtc_save) {
            p->dtc_save(s_dtcs[i].dtc, 0);
        }
    }
}

void dtc_store_clear_group(uint32_t group) {
    if (group == 0xFFFFFF) {
        dtc_store_clear_all();
        return;
    }
    const store_persistence_t *p = store_get_persistence();
    uint8_t group_hi = (uint8_t)(group >> 16);
    uint8_t n = (s_dtc_count <= DTC_MAX_ENTRIES) ? s_dtc_count : DTC_MAX_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t dtc_hi = (uint8_t)(s_dtcs[i].dtc >> 16);
        if (dtc_hi == group_hi) {
            s_dtcs[i].status = 0;
            if (p && p->dtc_save) {
                p->dtc_save(s_dtcs[i].dtc, 0);
            }
        }
    }
}


/* Update status on each sensor / diagnostic check sample */
void dtc_process_sample(dtc_runtime_item_t *item, bool sample_failed, int8_t step_fail, int8_t step_pass) {
	if (sample_failed) 
	{
		if ((int32_t)item->fault_detection_counter + step_fail >= FDC_THRESHOLD_FAILED) 
		{
			item->fault_detection_counter = FDC_THRESHOLD_FAILED;
	        /* Qualify fault */
	        item->status |= (DTC_STATUS_TEST_FAILED |
	                         DTC_STATUS_TEST_FAILED_THIS_CYCLE |
	                         DTC_STATUS_PENDING |
	                         DTC_STATUS_CONFIRMED |
	                         DTC_STATUS_FAILED_SINCE_CLEAR);
	        item->status &= (uint8_t)~(DTC_STATUS_NOT_COMPLETED_CYCLE |
	                                   DTC_STATUS_NOT_COMPLETED_CLEAR);
	        item->aging_counter = 0U; /* Reset aging */
		} 
		else 
		{
	        item->fault_detection_counter += step_fail;
	    }
	} 
	else 
	{
	    if ((int32_t)item->fault_detection_counter - step_pass <= FDC_THRESHOLD_PASSED) 
		{
	        item->fault_detection_counter = FDC_THRESHOLD_PASSED;

	        /* Qualify pass */
	        item->status &= (uint8_t)~DTC_STATUS_TEST_FAILED;
	        item->status &= (uint8_t)~(DTC_STATUS_NOT_COMPLETED_CYCLE |
	                                   DTC_STATUS_NOT_COMPLETED_CLEAR);
	    } 
		else 
		{
	        item->fault_detection_counter -= step_pass;
	    }
	}

}

/* Call on driving cycle start (Ignition ON / KL15 active) */
void dtc_operation_cycle_start(dtc_runtime_item_t *items, uint8_t count) {
	for (uint8_t i = 0U; i < count; i++) 
	{
		items->status &= (uint8_t)~(DTC_STATUS_TEST_FAILED | DTC_STATUS_TEST_FAILED_THIS_CYCLE);
		items->status |= DTC_STATUS_NOT_COMPLETED_CYCLE;
		items->fault_detection_counter = 0;
	}
}

/* Call on driving cycle end (Ignition OFF / KL15 inactive) */
void dtc_operation_cycle_end(dtc_runtime_item_t *items, uint8_t count) {
	for (uint8_t i = 0U; i < count; i++) 
	{
		/* Unlearning / Aging: increment aging if confirmed and didn't fail this cycle */
		if ((items->status & DTC_STATUS_CONFIRMED) != 0U) 
		{
			if ((items->status & DTC_STATUS_TEST_FAILED_THIS_CYCLE) == 0U) 
			{
				items->aging_counter++;
				if (items->aging_counter >= DTC_AGING_CYCLES_MAX) 
				{
					items->status &= (uint8_t)~DTC_STATUS_CONFIRMED;
					items->aging_counter = 0U;
				}
			}
		}
	}
}



