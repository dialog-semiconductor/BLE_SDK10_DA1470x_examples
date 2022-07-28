/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief SPI adapter demo application task
 *
 * Copyright (c) 2022 Dialog Semiconductor. All rights reserved.
 * 
 * This software ("Software") is owned by Dialog Semiconductor. By using this Software
 * you agree that Dialog Semiconductor retains all intellectual property and proprietary
 * rights in and to this Software and any use, reproduction, disclosure or distribution
 * of the Software without express written permission or a license agreement from Dialog
 * Semiconductor is strictly prohibited. This Software is solely for use on or in
 * conjunction with Dialog Semiconductor products.
 * 
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR AS
 * REQUIRED BY LAW, THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE PROVIDED
 * IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR BY LAW, IN NO EVENT SHALL DIALOG
 * SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 *
 ***************************************************************************************
 */

#include <string.h>
#include <stdlib.h>

#include "peripheral_setup.h"
#include "platform_devices.h"
#include "hw_gpio.h"
#include "hw_pd.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "hw_pdc.h"
#include "ad_spi.h"
#include "hw_wkup.h"
#include "hw_sys.h"
#include "sys_watchdog.h"
#include "misc.h"

/* Task priorities */
#define  mainSPI_TASK_PRIORITY       ( OS_TASK_PRIORITY_NORMAL )

/* Enable/disable asynchronous SPI operations */
#define SPI_ASYNC_EN                 ( 0 )

#define SPI_DEVICE_CLOSE_TIMEOUT_MS  ( 30 )

/* Retained symbols */
__RETAINED static OS_EVENT WKUP_EVENT;
__RETAINED static OS_EVENT MCP4921_ASYNC_EVENT;

/* SPI task handle */
__RETAINED static OS_TASK prvMCP4921_h;

#if dg_configUSE_WDOG
__RETAINED_RW int8_t idle_task_wdog_id = -1;
#endif

/* Functions prototypes */
static void prvMCP4921_Task(void *pvParameters);
int _dev_spi_close(ad_spi_handle_t dev, uint32_t timeout_ms);

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware(void);

static OS_TASK xHandle;

static void system_init(void *pvParameters)
{
        OS_BASE_TYPE status;

#if defined(CONFIG_RETARGET)
        extern void retarget_init(void);
#endif /* CONFIG_RETARGET */

        /*
         * Prepare clocks. Note: cm_cpu_clk_set() and cm_sys_clk_set() can be called only from a
         * task since they will suspend the task until the XTAL32M has settled and, maybe, the PLL
         * is locked.
         */
        cm_sys_clk_init(sysclk_XTAL32M);
        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /* Initialize platform watchdog */
        sys_watchdog_init();

#if dg_configUSE_WDOG
        idle_task_wdog_id = sys_watchdog_register(false);
        ASSERT_WARNING(idle_task_wdog_id != -1);
        sys_watchdog_configure_idle_id(idle_task_wdog_id);
#endif

        /* Prepare the hardware to run this demo */
        prvSetupHardware();

#if defined(CONFIG_RETARGET)
        retarget_init();
#endif /* CONFIG_RETARGET */

        OS_EVENT_CREATE(WKUP_EVENT);
        OS_EVENT_CREATE(MCP4921_ASYNC_EVENT);

        /*
         * Upon a wakeup cycle, wait for the XTAL32M crystal to settle.
         * BLE, USB and UART blocks require XTAL32M be settle to work properly.
         */
        pm_set_wakeup_mode(true);

        /* Set the desired sleep mode. */
        pm_sleep_mode_set(pm_mode_extended_sleep);
        pm_set_sys_wakeup_mode(pm_sys_wakeup_mode_fast);

        /* SPI task  */
        status = OS_TASK_CREATE("MCP4921",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        prvMCP4921_Task,                /* The function that implements the task. */
                        NULL,                           /* The parameter passed to the task. */
                        1024 * OS_STACK_WORD_SIZE,      /* Stack size allocated for the task in bytes. */
                        mainSPI_TASK_PRIORITY,          /* The priority assigned to the task. */
                        prvMCP4921_h );                 /* The task handle. */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* The work of the SysInit task is done */
        OS_TASK_DELETE(xHandle);
}

/**
 * @brief Basic initialization and creation of the system initialization task.
 */
int main(void)
{
        OS_BASE_TYPE status;

        /* Create SysInit task. */
        status = OS_TASK_CREATE("SysInit",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        system_init,                    /* The System Initialization task. */
                        ( void * ) 0,                   /* The parameter passed to the task. */
                        configMINIMAL_STACK_SIZE * OS_STACK_WORD_SIZE, /* Stack size allocated for the task in bytes. */
                        OS_TASK_PRIORITY_HIGHEST,       /* The priority assigned to the task. */
                        xHandle );                      /* The task handle. */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
         line will never be reached.  If the following line does execute, then
         there was insufficient FreeRTOS heap memory available for the idle and/or
         timer tasks     to be created.  See the memory management section on the
         FreeRTOS web site for more details. */
        for (;;);
}

#if (SPI_ASYNC_EN == 1)
/*
 * User-defined callback function called once SPI async. write operations are complete.
 *
 * \param[in] ud            Pointer to user data passed to the callback
 * \param[in] num_of_bytes  Data transferred through the SPI bus
 */
void MCP4921_write_async_cb(void *ud, uint16_t num_of_bytes)
{
       (void)ud;
       ASSERT_WARNING(num_of_bytes == sizeof(uint16_t));

       if (MCP4921_ASYNC_EVENT) {
               OS_EVENT_SIGNAL_FROM_ISR(MCP4921_ASYNC_EVENT);
       }
}
#endif /* SPI_ASYNC_EN */

/* Helper function to access the MCP4921 DAC module */
static void MCP4921_write_helper(const void *dev, const uint8_t *buf, size_t size)
{
        ASSERT_WARNING(buf != NULL);
        ASSERT_WARNING(size > 0);
        int ret;

        /* Open the SPI device */
        ad_spi_handle_t MCP4921_h = ad_spi_open((ad_spi_controller_conf_t *)dev);

        /* Enable the target SPI device */
        ad_spi_activate_cs(MCP4921_h);

#if (SPI_ASYNC_EN == 0)
        /*
         * Perform asynchronous SPI write operations, that is, the task is blocking
         * until the transaction is complete.
         */
        ret = ad_spi_write(MCP4921_h, buf, size);
#else
        /*
         * Perform asynchronous SPI write operations, that is, the task does not
         * block waiting for the transaction to finish. Once the transaction is
         * complete the registered callback function is called indicating the
         * completion of underlying operation.
         */
        ret = ad_spi_write_async(MCP4921_h, buf, size, MCP4921_write_async_cb, NULL);

        /*
         * In the meantime and while SPI transactions are performed in the background,
         * application task can proceed to other operations/calculations.
         * It is essential that, the new operations do not involve SPI transactions
         * on the already occupied bus!!!
         */
         OS_EVENT_WAIT(MCP4921_ASYNC_EVENT, OS_EVENT_FOREVER);
#endif /* SPI_ASYNC_EN */

         /* Disable the target SPI device */
         ad_spi_deactivate_cs(MCP4921_h);

         /* Close the SPI device */
         _dev_spi_close(MCP4921_h, SPI_DEVICE_CLOSE_TIMEOUT_MS);

         if (ret == 0) {
                 DBG_PRINTF("Value written to MCP4921 DAC : 0x%04X\n\r", ((uint16_t *)buf)[0]);
         } else {
                 DBG_PRINTF("\n\rUnsuccessful SPI write operation: %d\n\r", ret);
         }
}

/* Recommended routine to close an SPI device with timeout */
int _dev_spi_close(ad_spi_handle_t dev, uint32_t timeout_ms)
{
        OS_TICK_TIME timeout = OS_GET_TICK_COUNT() + OS_MS_2_TICKS(timeout_ms);

        while (ad_spi_close(dev, false) != AD_SPI_ERROR_NONE) {
                if (timeout <= OS_GET_TICK_COUNT()) {
                        ad_spi_close(dev, true);
                        return AD_SPI_ERROR_NONE;
                }
                OS_DELAY_MS(1);
        }
        return AD_SPI_ERROR_NONE;
}

/**
 * @brief Task to control the MCP482 DAC module
 */
static void prvMCP4921_Task(void *pvParameters)
{
        uint16_t reg_val;
        uint16_t dig_val;

        DBG_PRINTF("\n\r*** MCP4921 SPI Demonstration Example ***\n\r\n");

        for (;;) {
                /*
                 * Suspend task execution - As soon as WKUP ISR
                 * is triggered, the task resumes its execution.
                 */
                OS_EVENT_WAIT(WKUP_EVENT, OS_EVENT_FOREVER);

                /* Select an arbitrary value */
                dig_val = (uint16_t)rand();

                reg_val = MCP4921_SET_REG(dig_val, MCP4921_AB_CONTROL_BIT_SET | MCP4921_GA_CONTROL_BIT_SET |
                                                                                MCP4921_SHDN_CONTROL_BIT_SET);
                /* Perform an SPI write operation */
                MCP4921_write_helper(MCP4921_DEVICE, (const uint8_t *)&reg_val, sizeof(uint16_t));
        }
}

/* WKUP KEY interrupt handler */
static void wkup_cb(void)
{
        /* Clear the WKUP interrupt flag!!! */
        hw_wkup_reset_key_interrupt();

        if (WKUP_EVENT) {
                OS_EVENT_SIGNAL_FROM_ISR(WKUP_EVENT);
        }
}

/* WKUP KEY block initializer */
static void wkup_init(void)
{
        /* Initialize the WKUP controller */
        hw_wkup_init(NULL);

        /*
         * Set debounce time expressed in ms. Maximum allowable value is 63 ms.
         * A value set to 0 disables the debounce functionality.
         */
        hw_wkup_set_key_debounce_time(10);

        /*
         * Enable interrupts produced by the KEY block of the wakeup controller (debounce
         * circuitry) and register a callback function to hit following a KEY event.
         */
        hw_wkup_register_key_interrupt(wkup_cb, 1);

        /*
         * Set the polarity (rising/falling edge) that triggers the WKUP controller.
         *
         * \note The polarity is applied both to KEY and GPIO blocks of the controller
         *
         */
        hw_wkup_set_trigger(KEY1_PORT, KEY1_PIN,HW_WKUP_TRIG_LEVEL_LO_DEB);

        /* Enable interrupts of WKUP controller */
        hw_wkup_enable_key_irq();
}

/**
 * @brief Initialize the peripherals domain after power-up.
 *
 */
static void periph_init(void)
{
}

/**
 * @brief Hardware Initialization
 */
static void prvSetupHardware(void)
{
        /*
         * The IRQ produced by the KEY sub block of the wakeup controller (debounced IO IRQ)
         * is multiplexed with other trigger sources (VBUS IRQ, SYS2CMAC IRQ, JTAG present)
         * in a single PDC peripheral trigger ID (HW_PDC_PERIPH_TRIG_ID_COMBO).
         */
#if !defined(CONFIG_USE_BLE) && (!dg_configENABLE_DEBUGGER) && (!dg_configUSE_SYS_CHARGER)

        uint32_t pdc_wkup_combo_id = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                                                           HW_PDC_PERIPH_TRIG_ID_COMBO,
                                                                           HW_PDC_MASTER_CM33, 0));
        OS_ASSERT(pdc_wkup_combo_id != HW_PDC_INVALID_LUT_INDEX);

        hw_pdc_set_pending(pdc_wkup_combo_id);
        hw_pdc_acknowledge(pdc_wkup_combo_id);
#endif

        wkup_init();

        /* Init hardware */
        pm_system_init(periph_init);

        /* Enable the COM power domain before handling GPIO */
        hw_sys_pd_com_enable();

        /* Default SPI pins state (off) */
        ad_spi_io_config(((ad_spi_controller_conf_t *)MCP4921_DEVICE)->id,
                ((ad_spi_controller_conf_t *)MCP4921_DEVICE)->io, AD_IO_CONF_OFF);

        /* Configure the KEY1 push button on Pro DevKit */
        HW_GPIO_SET_PIN_FUNCTION(KEY1);
        HW_GPIO_PAD_LATCH_ENABLE(KEY1);
        /* Latch pad status */
        HW_GPIO_PAD_LATCH_DISABLE(KEY1);

        /* Disable the COM power domain after handling GPIOs */
        hw_sys_pd_com_disable();
}

/**
 * @brief Malloc fail hook
 */
void vApplicationMallocFailedHook(void)
{
        /* vApplicationMallocFailedHook() will only be called if
        configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
        function that will get called if a call to OS_MALLOC() fails.
        OS_MALLOC() is called internally by the kernel whenever a task, queue,
        timer or semaphore is created.  It is also called by various parts of the
        demo application.  If heap_1.c or heap_2.c are used, then the size of the
        heap available to OS_MALLOC() is defined by configTOTAL_HEAP_SIZE in
        FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
        to query the size of free heap space that remains (although it does not
        provide information on how the remaining heap might be fragmented). */
        ASSERT_ERROR(0);
}

/**
 * @brief Application idle task hook
 */
void vApplicationIdleHook(void)
{
        /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
         to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
         task.  It is essential that code added to this hook function never attempts
         to block in any way (for example, call OS_QUEUE_GET() with a block time
         specified, or call OS_DELAY()).  If the application makes use of the
         OS_TASK_DELETE() API function (as this demo application does) then it is also
         important that vApplicationIdleHook() is permitted to return to its calling
         function, because it is the responsibility of the idle task to clean up
         memory allocated by the kernel to any task that has since been deleted. */
#if dg_configUSE_WDOG
        sys_watchdog_notify(idle_task_wdog_id);
#endif
}

/**
 * @brief Application stack overflow hook
 */
void vApplicationStackOverflowHook( OS_TASK pxTask, char *pcTaskName)
{
        (void)pcTaskName;
        (void)pxTask;

        /* Run time stack overflow checking is performed if
         configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
        function is called if a stack overflow is detected. */
        ASSERT_ERROR(0);
}

/**
 * @brief Application tick hook
 */
void vApplicationTickHook(void)
{
}