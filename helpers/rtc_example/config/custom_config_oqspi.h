/**
 ****************************************************************************************
 *
 * @file custom_config_oqspi.h
 *
 * @brief Custom configuration file for non-FreeRTOS applications executing from OQSPI.
 *
 * Copyright (C) 2021 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef CUSTOM_CONFIG_OQSPI_H_
#define CUSTOM_CONFIG_OQSPI_H_

#include "bsp_definitions.h"

#define CONFIG_RETARGET

#define dg_configUSE_HW_TIMER (1)

#define dg_configUSE_HW_QSPI (1)
#define dg_configUSE_HW_QSPI2 (1)

#define dg_configFLASH_AUTODETECT (1)
#define dg_configQSPIC2_DEV_AUTODETECT (1)

#define dg_configUSE_LP_CLK                     ( LP_CLK_32768 )
#define dg_configEXEC_MODE                      MODE_IS_CACHED
#define dg_configCODE_LOCATION                  NON_VOLATILE_IS_OQSPI_FLASH

#define dg_configUSE_WDOG                       (0)

#define dg_configFLASH_CONNECTED_TO             (FLASH_CONNECTED_TO_1V8)
#define dg_configOQSPI_FLASH_AUTODETECT         (1)
#define dg_configOQSPI_FLASH_POWER_DOWN         (1)

#define dg_configPOWER_1V8_ACTIVE               (1)
#define dg_configPOWER_1V8_SLEEP                (1)

#define dg_configUSE_SW_CURSOR                  (1)
#define dg_configUSE_SYS_TRNG                   (0)

/*************************************************************************************************\
 * FreeRTOS specific config
 */
#define OS_FREERTOS                              /* Define this to use FreeRTOS */
#define configTOTAL_HEAP_SIZE                    14000   /* This is the FreeRTOS Total Heap Size */

/*************************************************************************************************\
 * Peripheral specific config
 */
#define dg_configRF_ENABLE_RECALIBRATION        (0)

#define dg_configFLASH_ADAPTER                  (0)
#define dg_configNVMS_ADAPTER                   (0)
#define dg_configNVMS_VES                       (0)

#define dg_configUSE_HW_USB                     (0)

/* Include bsp default values */
#include "bsp_defaults.h"
/* Include middleware default values */
#include "middleware_defaults.h"

#endif /* CUSTOM_CONFIG_OQSPI_H_ */
