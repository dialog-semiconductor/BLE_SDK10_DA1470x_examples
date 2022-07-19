/**
 ****************************************************************************************
 *
 * @file CompassTask.c
 *
 * @brief Compass Task generates compass simulation data
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
 ****************************************************************************************
 */
/*
 *      INCLUDES
 *****************************************************************************************
 */
#include <stdio.h>
#include "osal.h"
#include "demo.h"
#ifdef PERFORMANCE_METRICS
#include "metrics.h"
#endif

extern void compass_event(int16_t *data);

/*
 *      DEFINES
 *****************************************************************************************
 */
#if !COMPASS_ROTATION_USES_CANVAS
#define COMPASS_DATA_DELAY_MS           (60) //measured rotation time for compass image is ~70msec
#else
#define COMPASS_DATA_DELAY_MS           (30) //measured rotation time for compass image is ~35msec
#endif
#define STOP_COMPASS_DATA_NOTIFY_MASK         (1 << 4)

static OS_TASK_FUNCTION(compass_data, argument);

/*
 *  STATIC VARIABLES
 *****************************************************************************************
 */
static INITIALISED_PRIVILEGED_DATA OS_TASK compass_data_task_h = NULL;

/*
 *   GLOBAL FUNCTIONS
 *****************************************************************************************
 */
void start_compass_data()
{
#ifdef PERFORMANCE_METRICS
        OS_ENTER_CRITICAL_SECTION();
        metrics_set_tag(METRICS_TAG_COMPASS_ROTATION);
        OS_LEAVE_CRITICAL_SECTION();
#endif

        OS_TASK_CREATE("CompassData",             /* The text name assigned to the task, for
                                                     debug only; not used by the kernel. */
                       compass_data,              /* The function that implements the task. */
                       NULL,                      /* The parameter passed to the task. */
                       2096,                      /* The number of bytes to allocate to the
                                                     stack of the task. */
                       OS_TASK_PRIORITY_NORMAL,   /* The priority assigned to the task. */
                       compass_data_task_h);      /* The task handle */
        OS_ASSERT(compass_data_task_h);
}

void stop_compass_data()
{
        OS_TASK_NOTIFY(compass_data_task_h, STOP_COMPASS_DATA_NOTIFY_MASK, OS_NOTIFY_SET_BITS);
}

/*
 *   STATIC FUNCTIONS
 *****************************************************************************************
 */

/**
 * @brief  Compass simulation data generation task
 * @param  argument: pointer that is passed to the thread function as start argument.
 */
static OS_TASK_FUNCTION(compass_data, argument)
{
        int16_t angle = 0;
        uint32_t notif;

        for (;;) {
                OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_MS_2_TICKS(COMPASS_DATA_DELAY_MS));

                if (notif & STOP_COMPASS_DATA_NOTIFY_MASK) {
                        OS_TASK_DELETE(compass_data_task_h);
                }
                else {
                        if (angle < 360) {
                                angle++;
                        }
                        else {
                                angle = 0;
#ifdef PERFORMANCE_METRICS
                                OS_ENTER_CRITICAL_SECTION();
                                metrics_set_tag(METRICS_TAG_NO_LOGGING);
                                OS_LEAVE_CRITICAL_SECTION();
#endif
                        }

                        compass_event(&angle);
                }
        }
}
