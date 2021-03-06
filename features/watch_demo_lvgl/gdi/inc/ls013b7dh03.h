/**
 ****************************************************************************************
 *
 * @file ls013b7dh03.h
 *
 * @brief LCD configuration for LS013B7DH03 LCD
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

#ifndef LS013B7DH03_H_
#define LS013B7DH03_H_

#include <stdint.h>
#include "platform_devices.h"

#if dg_configUSE_LS013B7DH03

#if dg_configLCDC_ADAPTER

/*********************************************************************
 *
 *       Defines
 *
 *********************************************************************
 */
#define GDI_DISP_COLOR           (HW_LCDC_OCM_L1)
#define GDI_DISP_RESX            (128)
#define GDI_DISP_RESY            (128)
#define GDI_DISP_OFFSETX         (0)
#define GDI_DISP_OFFSETY         (0)
#define GDI_LCDC_CONFIG          (&ls013b7dh03_cfg)
#define GDI_USE_CONTINUOUS_MODE  (0)

/*
 * static const ad_io_conf_t ls013b7dh03_gpio_cfg[] = {
 *       { LS013B7DH03_SCK_PORT,      LS013B7DH03_SCK_PIN,      { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_CLK, true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true  }},
 *       { LS013B7DH03_SDA_PORT,      LS013B7DH03_SDA_PIN,      { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_DO,  true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true  }},
 *       { LS013B7DH03_CS_PORT,       LS013B7DH03_CS_PIN,       { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_EN,  true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true  }},
 *       { LS013B7DH03_EXTCOMIN_PORT, LS013B7DH03_EXTCOMIN_PIN, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD,         false}, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false }},
 *       { LS013B7DH03_DISP_PORT,     LS013B7DH03_DISP_PIN,     { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO,        false}, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false }},
 * };

 * const ad_lcdc_io_conf_t ls013b7dh03_io = {
 *       .voltage_level = HW_GPIO_POWER_V33,
 *       .io_cnt = ARRAY_LENGTH(ls013b7dh03_gpio_cfg),
 *       .io_list = ls013b7dh03_gpio_cfg,
 * };
 */

static GDI_DRV_CONF_ATTR ad_lcdc_driver_conf_t ls013b7dh03_drv = {
        .hw_init.phy_type = HW_LCDC_PHY_SHARP_SPI,
        .hw_init.format = GDI_DISP_COLOR,
        .hw_init.resx = GDI_DISP_RESX,
        .hw_init.resy = GDI_DISP_RESY,
        .hw_init.cfg_extra_flags = 0,
        .hw_init.mode = HW_LCDC_MODE_DISABLE,
        .hw_init.write_freq = LCDC_FREQ_1MHz,   //Max @ 1.1MHz
        .ext_clk = HW_LCDC_EXT_CLK_62_5HZ,      //Max @ 65Hz
        .te_enable = false,
        .te_mode = HW_LCDC_TE_POL_LOW,
};

static const ad_lcdc_controller_conf_t ls013b7dh03_cfg = {
        .io = &ls013b7dh03_io,
        .drv = &ls013b7dh03_drv,
};

static const HW_LCDC_OUTPUT_COLOR_MODE screen_color_modes[] = {
        HW_LCDC_OCM_L1,
};

__STATIC_INLINE bool screen_set_color_mode(HW_LCDC_OUTPUT_COLOR_MODE color_mode)
{
        switch (color_mode) {
        case HW_LCDC_OCM_L1:
                break;
        default:
                return false; //! Unsupported color mode
        }

        return true;
}

static const uint8_t screen_init_cmds[] = {
        LCDC_JDI_CLEAR(),
};

static const uint8_t screen_power_on_cmds[] = {
        LCDC_GPIO_SET_ACTIVE(LS013B7DH03_DISP_PORT, LS013B7DH03_DISP_PIN),
};

static const uint8_t screen_enable_cmds[] = {
        LCDC_EXT_CLK_SET(true),
};

static const uint8_t screen_disable_cmds[] = {
        LCDC_EXT_CLK_SET(false),
};

static const uint8_t screen_power_off_cmds[] = {
        LCDC_GPIO_SET_INACTIVE(LS013B7DH03_DISP_PORT, LS013B7DH03_DISP_PIN),
};

static const uint8_t screen_clear_cmds[] = {
        LCDC_JDI_CLEAR(),
};

__STATIC_INLINE void screen_set_partial_update_area(hw_lcdc_frame_t *frame)
{
        /* Minimum addressable item is one line */
        frame->startx = 0;
        frame->endx = GDI_DISP_RESX - 1;
}

#endif /* dg_configLCDC_ADAPTER */

#endif /* dg_configUSE_LS013B7DH03 */

#endif /* LS013B7DH03_H_ */
