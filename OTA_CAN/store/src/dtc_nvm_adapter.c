#include "dtc_store.h"
#include "store_types.h"
#include "uds_wear_leveling.h"
#include "stm32f103_flash_port.h"
#include <string.h>
#include "dtc_nvm_adapter.h"

/* Top 2 pages of 64KB STM32F103 Flash (Pages 62 & 63: 0x0800F800 - 0x0800FFFF) /
#define DTC_NVM_STORAGE_BASE 0x0800F800U
#define DTC_NVM_SECTOR_COUNT 2U
#define DTC_NVM_MAGIC 0x44544331U / 'DTC1' */

typedef struct __attribute__((packed))
{
	uint32_t magic;
	uint8_t dtc_count;
	struct 
	{
		uint32_t dtc;
		uint8_t status;
	} entries[DTC_MAX_ENTRIES];
} DtcNvmPayload;

static UdsParamStore s_dtc_param_store;
static DtcNvmPayload s_dtc_nvm_cache;
static bool s_dtc_nvm_dirty = false;

/* Callback matching store_persistence_t->dtc_save */
static void dtc_nvm_save_cb(uint32_t dtc, uint8_t status) {
	for (uint8_t i = 0U; i < s_dtc_nvm_cache.dtc_count; i++) 
	{
		if (s_dtc_nvm_cache.entries[i].dtc == dtc) 
		{
			if (s_dtc_nvm_cache.entries[i].status != status) 
			{
				s_dtc_nvm_cache.entries[i].status = status;
				s_dtc_nvm_dirty = true;
			}
			return;
		}
	}
	if (s_dtc_nvm_cache.dtc_count < DTC_MAX_ENTRIES) {
	s_dtc_nvm_cache.entries[s_dtc_nvm_cache.dtc_count].dtc = dtc;
	s_dtc_nvm_cache.entries[s_dtc_nvm_cache.dtc_count].status = status;
	s_dtc_nvm_cache.dtc_count++;
	s_dtc_nvm_dirty = true;
	}
}

/* Callback matching store_persistence_t->dtc_load */
static uint8_t dtc_nvm_load_cb(uint32_t dtc) {
	for (uint8_t i = 0U; i < s_dtc_nvm_cache.dtc_count; i++) 
	{
		if (s_dtc_nvm_cache.entries[i].dtc == dtc) 
		{
			return s_dtc_nvm_cache.entries[i].status;
		}
	}
	return 0xFFU;
}

static const store_persistence_t s_dtc_persistence = {
	.did_save = NULL,
	.did_load = NULL,
	.dtc_save = dtc_nvm_save_cb,
	.dtc_load = dtc_nvm_load_cb,
};

/* 1. Call this during system initialization */
void dtc_nvm_init(void) {
	const UdsFlashPort *port = stm32f103_get_flash_port();
	int rc = uds_param_init(&s_dtc_param_store, port, DTC_NVM_STORAGE_BASE,
	DTC_NVM_SECTOR_COUNT, sizeof(DtcNvmPayload));
	if (rc != UDS_PARAM_OK) {
	return;
	}

	/* Load saved statuses from wear-leveling flash */
	if (uds_param_load(&s_dtc_param_store, &s_dtc_nvm_cache) != UDS_PARAM_OK ||
	    s_dtc_nvm_cache.magic != DTC_NVM_MAGIC) {
	    memset(&s_dtc_nvm_cache, 0, sizeof(s_dtc_nvm_cache));
	    s_dtc_nvm_cache.magic = DTC_NVM_MAGIC;
	}

	/* Register the persistence interface */
	store_set_persistence(&s_dtc_persistence);
}
/* 2. Call this on cycle end, ECU reset, or clear DTC to write atomically */
void dtc_nvm_flush(void) {
	if (!s_dtc_nvm_dirty) {
		return;
	}
	s_dtc_nvm_cache.magic = DTC_NVM_MAGIC;
	if (uds_param_save(&s_dtc_param_store, &s_dtc_nvm_cache) == UDS_PARAM_OK) {
		s_dtc_nvm_dirty = false;
	}
}

