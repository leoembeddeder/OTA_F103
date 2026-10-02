#include "stm32f103_flash_port.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "uds_wear_leveling.h"


#define STM32F103_PAGE_SIZE 1024U /* 1 KB per page for Medium-Density STM32F103 */

static uint32_t stm32f103_sector_size(uint32_t addr) {
    (void)addr;
    return STM32F103_PAGE_SIZE;
}

static int stm32f103_read(uint32_t addr, void *buf, size_t len) {
    if (buf == NULL) {
        return UDS_PARAM_INVALID_PARAM;
    }
    /* STM32 internal flash is directly memory-mapped */
    memcpy(buf, (const void *)addr, len);
    return UDS_PARAM_OK;
}

static int stm32f103_erase(uint32_t addr) {
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0U;

    HAL_FLASH_Unlock();
    memset(&erase_init, 0, sizeof(erase_init));
    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = addr & ~(STM32F103_PAGE_SIZE - 1U); /* Align to page boundary */
    erase_init.NbPages     = 1U;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    return (status == HAL_OK) ? UDS_PARAM_OK : UDS_PARAM_ERR;
}

static int stm32f103_write(uint32_t addr, const void *buf, size_t len) {
    if (buf == NULL) {
        return UDS_PARAM_INVALID_PARAM;
    }

    HAL_FLASH_Unlock();
    
    const uint8_t *src = (const uint8_t *)buf;
    size_t i = 0U;

    /* STM32F1 requires 16-bit half-word programming */
    while ((i + 2U) <= len) {
        uint16_t halfword = (uint16_t)(src[i] | ((uint16_t)src[i + 1U] << 8U));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, (uint64_t)halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            return UDS_PARAM_ERR;
        }
        i += 2U;
    }

    /* Program trailing odd byte padded with 0xFF */
    if (i < len) {
        uint16_t halfword = (uint16_t)(src[i] | 0xFF00U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, (uint64_t)halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            return UDS_PARAM_ERR;
        }
    }

    HAL_FLASH_Lock();
    return UDS_PARAM_OK;
}

static const UdsFlashPort s_stm32f103_port = {
    .erase       = stm32f103_erase,
    .read        = stm32f103_read,
    .write       = stm32f103_write,
    .sector_size = stm32f103_sector_size,
    .program_granule = 2U,
    .erased_byte = 0xFFU,
};

const UdsFlashPort *stm32f103_get_flash_port(void) {
    return &s_stm32f103_port;
}




/* Use the last 2 pages of a 64 KB STM32F103C8 (Pages 62 & 63: 0x0800F800 - 0x0800FFFF) */
#define NV_STORAGE_BASE   0x0800F800U
#define NV_SECTOR_COUNT   2U

typedef struct {
    uint32_t boot_counter;
    uint32_t vin_number;
    uint8_t  ecu_config[8];
} AppEcuParams;


void stm32f103_flash_example(void)
{
    UdsParamStore store;
    AppEcuParams params;
    const UdsFlashPort *port = stm32f103_get_flash_port();

    /* 1. Initialize store (scans flash, validates CRC, recovers latest slot) */
    int rc = uds_param_init(&store, port, NV_STORAGE_BASE, NV_SECTOR_COUNT, sizeof(AppEcuParams));
    if (rc != UDS_PARAM_OK) {
        /* Error handling */
        while (1);
    }

    /* 2. Load existing parameters */
    if (uds_param_load(&store, &params) != UDS_PARAM_OK) {
        /* First boot / empty flash: initialize default parameters */
        params.boot_counter = 0U;
        params.vin_number   = 12345678U;
        memset(params.ecu_config, 0xAA, sizeof(params.ecu_config));
    }
    printf("params.boot_counter: %d\r\n", params.boot_counter);
	printf("params.vin_number: %d\r\n", params.vin_number);

    /* 3. Modify parameters */
    params.boot_counter++;

    /* 4. Save safely (two-phase commit to next slot, rotating sectors as needed) */
    rc = uds_param_save(&store, &params);
    if (rc == UDS_PARAM_OK) 
	{
        print_flash_status(&store);
    }
	else 
	{
        printf("Save failed with status: %d\r\n", rc);
	}
}



