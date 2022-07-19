/**
 ****************************************************************************************
 *
 * @file lpm012m503a.h
 *
 * @brief LCD configuration for LPM012M503A LCD
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
#ifndef LPM012M503A_H_
#define LPM012M503A_H_

#include <stdint.h>
#include "platform_devices.h"

#if dg_configUSE_LPM012M503A

#if dg_configLCDC_ADAPTER

#define GDI_DISP_COLOR                  (HW_LCDC_OCM_RGB222)
#define GDI_DISP_RESX                   (240)
#define GDI_DISP_RESY                   (240)
#define GDI_USE_CONTINUOUS_MODE         (1)
#define GDI_LCDC_CONFIG                 (&lpm012m503a_cfg)
#define GDI_USE_CONTINUOUS_MODE         (0)

static GDI_DRV_CONF_ATTR ad_lcdc_driver_conf_t lpm012m503a_drv = {
        .hw_init.phy_type = HW_LCDC_PHY_CLASSIC_PARALLEL,
        .hw_init.format = HW_LCDC_OCM_8RGB888,  //! LCD controller requires that the RGB888 is selected
        .hw_init.cfg_extra_flags = 0,
        .hw_init.mode = HW_LCDC_MODE_INVPIXCLK,
        .hw_init.write_freq = LCDC_FREQ_1_6MHz,
        .ext_clk = HW_LCDC_EXT_CLK_OFF,
        .te_enable = false,
        .te_mode = HW_LCDC_TE_POL_LOW,
        .display.resx = GDI_DISP_RESX,
        .display.resy = GDI_DISP_RESY,

        .display.fpx = 4,
        .display.blx = 2,
        .display.bpx = 2,

        .display.fpy = 2,
        .display.bly = 1,
        .display.bpy = 1,
};

static const ad_lcdc_controller_conf_t lpm012m503a_cfg = {
        .io = &lpm012m503a_io,
        .drv = &lpm012m503a_drv,
};

static const uint8_t screen_init_cmds[] = {
        LCDC_EXT_CLK_SET(true),
};

static const uint8_t screen_power_on_cmds[] = {
        LCDC_GPIO_SET_ACTIVE(LPM012M503A_PEN_PORT, LPM012M503A_PEN_PIN),
};

static const uint8_t screen_enable_cmds[] = {
        LCDC_EXT_CLK_SET(true),
};

static const uint8_t screen_disable_cmds[] = {
        LCDC_EXT_CLK_SET(false),
};

static const uint8_t screen_power_off_cmds[] = {
        LCDC_GPIO_SET_INACTIVE(LPM012M503A_PEN_PORT, LPM012M503A_PEN_PIN),
};

static const uint8_t screen_clear_cmds[] = {
        /* Screen does not have a special command for clearing */
};

__STATIC_INLINE void screen_set_partial_update_area(hw_lcdc_frame_t *frame)
{
        frame->startx = 0;
        frame->starty = 0;
        frame->endx = GDI_DISP_RESX - 1;
        frame->endy = GDI_DISP_RESY - 1;
}

#endif /* dg_configLCDC_ADAPTER */

#endif /* dg_configUSE_LPM012M503A */

#endif /* LPM012M503A_H_ */
