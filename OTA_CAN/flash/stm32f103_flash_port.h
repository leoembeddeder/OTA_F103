#ifndef STM32F103_FLASH_PORT_H
#define STM32F103_FLASH_PORT_H

#include "uds_wear_leveling.h"

/**
 * @brief Returns the UdsFlashPort instance configured for STM32F103 internal flash.
 */
const UdsFlashPort *stm32f103_get_flash_port(void);

void stm32f103_flash_example(void);

#endif /* STM32F103_FLASH_PORT_H */
