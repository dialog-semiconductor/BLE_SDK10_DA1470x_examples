/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief FreeRTOS template application with retarget
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "hw_wkup.h"
#include "hw_sys.h"
#include "sys_watchdog.h"

#include "adf_config.h"
#include "app_debug_freertos_tcb.h"



/******** Notification Bitmasks ********/
#define WKUP_KEY_PRESS_EVENT_NOTIF      (1 << 2)
#define WKUP_KEY_RELEASE_EVENT_NOTIF    (1 << 3)
#define LONG_PRESS_TIMER_NOTIF          (1 << 4)

/* Task priorities */
#define mainTEMPLATE_TASK_PRIORITY              ( OS_TASK_PRIORITY_NORMAL )

/* The rate at which data is template task counter is incremented. */
#define mainCOUNTER_FREQUENCY_MS                OS_MS_2_TICKS(200)
/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );
/*
 * Task functions .
 */
static void ADF_Task( void *pvParameters );

static OS_TASK xHandle;
/* Application task handle */
__RETAINED static OS_TASK task_h;
/* Timer used to distinguish between long and short press */
__RETAINED static OS_TIMER button_tim;
__RETAINED bool button_pressed;

#if dg_configUSE_WDOG
__RETAINED_RW int8_t idle_task_wdog_id = -1;
#endif

static void system_init( void *pvParameters )
{
        OS_TASK task_h = NULL;

#if defined CONFIG_RETARGET
        extern void retarget_init(void);
#endif

        cm_sys_clk_init(sysclk_XTAL32M);

        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /* Initialize platform watchdog */
        sys_watchdog_init();

#if dg_configUSE_WDOG
        /* Register the Idle task first */
        idle_task_wdog_id = sys_watchdog_register(false);
        ASSERT_WARNING(idle_task_wdog_id != -1);
        sys_watchdog_configure_idle_id(idle_task_wdog_id);
#endif


        /* Prepare the hardware to run this demo. */
        prvSetupHardware();

        pm_set_wakeup_mode(true);

        /* Set the desired sleep mode. */
        pm_sleep_mode_set(pm_mode_extended_sleep);

        /* Set the desired wakeup mode. */
        pm_set_sys_wakeup_mode(pm_sys_wakeup_mode_normal);

#if defined CONFIG_RETARGET
        retarget_init();
#endif


        /* Start main task here (text menu available via UART1 to control application) */
        OS_TASK_CREATE( "ADF Task",            /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        ADF_Task,                /* The function that implements the task. */
                        NULL,                           /* The parameter passed to the task. */
                        1024 * OS_STACK_WORD_SIZE,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        mainTEMPLATE_TASK_PRIORITY,     /* The priority assigned to the task. */
                        task_h );                       /* The task handle */
        OS_ASSERT(task_h);

        /* the work of the SysInit task is done */
        OS_TASK_DELETE( xHandle );
}

/**
 * @brief Template main creates a SysInit task, which creates a Template task
 */
int main( void )
{
        OS_BASE_TYPE status;

        /*
         * Start Tracking debug information.  If all tasks are wished to be tracked, call here, if
         * just application tasks are required, call variably in SysInit
         */
        adf_start_tracking_init();


        /* Start the two tasks as described in the comments at the top of this
        file. */
        status = OS_TASK_CREATE("SysInit",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        system_init,                    /* The System Initialization task. */
                        ( void * ) 0,                   /* The parameter passed to the task. */
                        configMINIMAL_STACK_SIZE * OS_STACK_WORD_SIZE,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        OS_TASK_PRIORITY_HIGHEST,       /* The priority assigned to the task. */
                        xHandle );                      /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);



        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );

}

/* WKUP KEY interrupt handler */
static void _wkup_key_cb(void)
{
        uint32_t event = 0;

        /* Clear the WKUP interrupt flag!!! */
        hw_wkup_reset_key_interrupt();


        /*
         * Check whether the button is pressed or released based on the
         */
        event = (hw_wkup_get_trigger(KEY1_PORT, KEY1_PIN) == HW_WKUP_TRIG_LEVEL_LO_DEB) ?
                                WKUP_KEY_PRESS_EVENT_NOTIF : WKUP_KEY_RELEASE_EVENT_NOTIF;

        /* Change the trigger polarity so that the WKUP controller is triggered on the opposite edge */
        hw_wkup_set_trigger(KEY1_PORT, KEY1_PIN,
               (event == WKUP_KEY_PRESS_EVENT_NOTIF) ? HW_WKUP_TRIG_LEVEL_HI_DEB : HW_WKUP_TRIG_LEVEL_LO_DEB);


        /* Notify the application task */
        OS_TASK_NOTIFY_FROM_ISR(task_h, event, OS_NOTIFY_SET_BITS);
}

/* Initialize the WKUP controller */
static void _wkup_init(void)
{

        hw_wkup_init(NULL);

        /* Configure Button1 (S3) to switch between normal and fast wakeup modes. */
        hw_wkup_set_trigger(KEY1_PORT, KEY1_PIN, HW_WKUP_TRIG_LEVEL_LO_DEB);

        hw_wkup_set_key_debounce_time(20);

        hw_wkup_register_key_interrupt(_wkup_key_cb, 1);
        hw_wkup_enable_key_irq();

}

/* Alert timeout timer callback */
static void button_tim_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        OS_TASK_NOTIFY(task, LONG_PRESS_TIMER_NOTIF, OS_NOTIFY_SET_BITS);
}

static void report_reset_information(void)
{
        uint16_t adf_actual_len;

        /*
         * Get serialized data size, for ADF dataLONG_PRESS_TIMER_NOTIF
         */
        uint16_t adf_length = adf_get_serialized_size();

        /*
         * Allocate memory for the reset data
         */

        uint8_t *reset_data = OS_MALLOC(adf_length);

        /*
         * Get serialized data
         */
        adf_get_serialized_reset_data(reset_data, &adf_actual_len, adf_length);

#ifdef ADF_PRINTF
        /*
         * Print verbose output of reset data
         */
        adf_print_verbose(reset_data, adf_length);
#endif

        /*
         * Free Data
         */
        OS_FREE(reset_data);

}

/**
 * @brief Template task increases a counter every mainCOUNTER_FREQUENCY_MS ms
 */
static void ADF_Task( void *pvParameters )
{
        int8_t wdog_id;

        /*
         * Rerort ADF Data
         */
        report_reset_information();

        task_h = OS_GET_CURRENT_TASK();
        _wkup_init();

        button_tim = OS_TIMER_CREATE("but", OS_MS_2_TICKS(2000), OS_TIMER_FAIL,
                                                        (void *) OS_GET_CURRENT_TASK(), button_tim_cb);

        /* Register task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        for ( ;; ) {
                OS_BASE_TYPE ret __UNUSED;
                uint32_t notif;

                /* Notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* Suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for the task notification. Therefore, the return value must
                 * always be OS_OK
                 */
                OS_ASSERT(ret == OS_OK);

                /* Resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & WKUP_KEY_PRESS_EVENT_NOTIF) {

                        /*
                         * Start long press timer
                         */
                        OS_TIMER_START(button_tim, OS_TIMER_FOREVER);
                        button_pressed = true;

                        printf("Button Pressed... \r\n\r\n");
                }
                if (notif & WKUP_KEY_RELEASE_EVENT_NOTIF) {

                        printf("Triggering HardFault manually \r\n\r\n");
                        /*
                         * Timer never expired before release, so throw a hardfault
                         *
                         *Set the HardFaultPended bit of SHCSR ARM Register to cause a Hardfault. */
                        SCB->SHCSR |= SCB_SHCSR_HARDFAULTPENDED_Msk;
                        while(1){};

                }

                if (notif & LONG_PRESS_TIMER_NOTIF)
                {
                        printf("Triggering NMI Fault manually... \r\n\r\n");

                        __disable_irq();

                        while(1){};
                }


        }
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
static void prvSetupHardware( void )
{

        /* Init hardware */
        pm_system_init(periph_init);


        /*
         * On Pro DevKit revision B there is no external pull-up resistor connected
         * to P0.6 pin. Thus, an internal pull-up resistor is enabled.
         */
        hw_gpio_set_pin_function(KEY1_PORT, KEY1_PIN,  KEY1_MODE,
                KEY1_FUNC);

        /* In order to make a GPIO active it must be set in latch enabled state */
        hw_gpio_pad_latch_enable(KEY1_PORT, KEY1_PIN);

        /*
         * A GPIO must be set in latch disabled state before M33 entering sleep.
         * Otherwise, the pin will be set to its default state (input pull down).
         */
        hw_gpio_pad_latch_disable(KEY1_PORT, KEY1_PIN);


}

/**
 * @brief Malloc fail hook
 */
void vApplicationMallocFailedHook( void )
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
void vApplicationIdleHook( void )
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
void vApplicationStackOverflowHook( OS_TASK pxTask, char *pcTaskName )
{
        ( void ) pcTaskName;
        ( void ) pxTask;

        /* Run time stack overflow checking is performed if
        configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
        function is called if a stack overflow is detected. */
        ASSERT_ERROR(0);
}

/**
 * @brief Application tick hook
 */
void vApplicationTickHook( void )
{
}


