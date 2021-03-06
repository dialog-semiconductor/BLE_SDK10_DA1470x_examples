/**
 ****************************************************************************************
 *
 * @file t1d54bp002.h
 *
 * @brief LCD configuration for T1D54BP002 with IC ST7789V2
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
#ifndef T1D54BP002_H_
#define T1D54BP002_H_

#include <stdint.h>
#include "platform_devices.h"

#if dg_configUSE_T1D54BP002

#if dg_configLCDC_ADAPTER

/*********************************************************************
 *
 *       Defines
 *
 *********************************************************************
 */
#define GDI_DISP_COLOR                  (HW_LCDC_OCM_8RGB565)
#define GDI_DISP_RESX                   (240)
#define GDI_DISP_RESY                   (240)
#define GDI_DISP_OFFSETX                (0)
#define GDI_DISP_OFFSETY                (0)
#define GDI_LCDC_CONFIG                 (&t1d54bp002_cfg)
#define GDI_USE_CONTINUOUS_MODE         (0)

/*
 * static const ad_io_conf_t t1d54bp002_gpio_cfg[] = {
 *       { T1D54BP002_SCK_PORT, T1D54BP002_SCK_PIN, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_CLK, true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true }},
 *       { T1D54BP002_SDA_PORT, T1D54BP002_SDA_PIN, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_DO,  true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true }},
 *       { T1D54BP002_RST_PORT, T1D54BP002_RST_PIN, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO,        true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true }},
 *       { T1D54BP002_CS_PORT,  T1D54BP002_CS_PIN,  { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_LCD_SPI_EN,  true }, { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true }},
 * };

 * const ad_lcdc_io_conf_t t1d54bp002_io = {
 *       .voltage_level = HW_GPIO_POWER_V33,
 *       .io_cnt = ARRAY_LENGTH(t1d54bp002_gpio_cfg),
 *       .io_list = t1d54bp002_gpio_cfg,
 * };
 */

static GDI_DRV_CONF_ATTR ad_lcdc_driver_conf_t t1d54bp002_drv = {
        .hw_init.phy_type = HW_LCDC_PHY_MIPI_SPI3,
        .hw_init.format = GDI_DISP_COLOR,
        .hw_init.resx = GDI_DISP_RESX,
        .hw_init.resy = GDI_DISP_RESY,
        .hw_init.cfg_extra_flags = 0,
        .hw_init.mode = HW_LCDC_MODE_DISABLE,
        .hw_init.write_freq = LCDC_FREQ_48MHz,
        .ext_clk = HW_LCDC_EXT_CLK_OFF,
        .te_enable = false,
        .te_mode = HW_LCDC_TE_POL_LOW,
};

static const ad_lcdc_controller_conf_t t1d54bp002_cfg = {
        .io = &t1d54bp002_io,
        .drv = &t1d54bp002_drv,
};

typedef enum {
        T1D54BP002_PIXEL_FORMAT_RGB444 = 0x03,
        T1D54BP002_PIXEL_FORMAT_RGB565 = 0x05,
        T1D54BP002_PIXEL_FORMAT_RGB666 = 0x06,
} T1D54BP002_DSPI_PIXEL_FORMAT;

/* Required (and supported) output color modes */
static const HW_LCDC_OUTPUT_COLOR_MODE screen_color_modes[] = {
        HW_LCDC_OCM_8RGB444,
        HW_LCDC_OCM_8RGB565,
        HW_LCDC_OCM_8RGB666,
};

/* Callback function to configure display's color mode */
__STATIC_INLINE bool screen_set_color_mode(HW_LCDC_OUTPUT_COLOR_MODE color_mode)
{
        uint8_t value = 0;

        /* Exercise color mode */
        switch (color_mode) {
        case HW_LCDC_OCM_8RGB444:
                value = T1D54BP002_PIXEL_FORMAT_RGB444;
                break;
        case HW_LCDC_OCM_8RGB565:
                value = T1D54BP002_PIXEL_FORMAT_RGB565;
                break;
        case HW_LCDC_OCM_8RGB666:
                value = T1D54BP002_PIXEL_FORMAT_RGB666;
                break;
        default:
                return false; // Unsupported color mode
        }
        hw_lcdc_dcs_cmd_params(HW_LCDC_MIPI_DCS_SET_PIXEL_FORMAT, &value, sizeof(value));

        return true;
}

static const uint8_t screen_init_cmds[] = {
        /* Power on */
        LCDC_GPIO_SET_INACTIVE(T1D54BP002_RST_PORT, T1D54BP002_RST_PIN),
        LCDC_DELAY_MS(10),
        LCDC_GPIO_SET_ACTIVE(T1D54BP002_RST_PORT, T1D54BP002_RST_PIN),
        LCDC_DELAY_MS(120),

        LCDC_MIPI_SW_RST(),
        LCDC_DELAY_MS(100),

        LCDC_MIPI_CMD_DATA(0xB2, 0x3F, 0x3F, 0x00, 0x00),

        LCDC_MIPI_EXIT_INVERT(),
        LCDC_MIPI_CMD_DATA(0xC0, 0x3C), //inverse inversion command

        LCDC_MIPI_SET_POSITION(GDI_DISP_OFFSETX, GDI_DISP_OFFSETY,
                GDI_DISP_RESX + GDI_DISP_OFFSETX - 1, GDI_DISP_RESY + GDI_DISP_OFFSETY - 1),

        LCDC_MIPI_SET_MODE(GDI_DISP_COLOR == HW_LCDC_OCM_8RGB444 ? T1D54BP002_PIXEL_FORMAT_RGB444 :
                           GDI_DISP_COLOR == HW_LCDC_OCM_8RGB565 ? T1D54BP002_PIXEL_FORMAT_RGB565 :
                                                                   T1D54BP002_PIXEL_FORMAT_RGB666),
};

static const uint8_t screen_power_on_cmds[] = {
};

static const uint8_t screen_enable_cmds[] = {
        LCDC_MIPI_CMD_DATA(HW_LCDC_MIPI_DCS_EXIT_SLEEP_MODE),
        LCDC_DELAY_MS(120),
        LCDC_MIPI_CMD_DATA(HW_LCDC_MIPI_DCS_SET_DISPLAY_ON),
        LCDC_DELAY_MS(80),
};

static const uint8_t screen_disable_cmds[] = {
        LCDC_MIPI_CMD_DATA(HW_LCDC_MIPI_DCS_SET_DISPLAY_OFF),
        LCDC_DELAY_MS(50),
        LCDC_MIPI_CMD_DATA(HW_LCDC_MIPI_DCS_ENTER_SLEEP_MODE),
        LCDC_DELAY_MS(120),
};

static const uint8_t screen_power_off_cmds[] = {
};

static const uint8_t screen_clear_cmds[] = {
};

__STATIC_INLINE void screen_set_partial_update_area(hw_lcdc_frame_t *frame)
{
        UNUSED_ARG(frame); //! Just to suppress compiler warnings
}

#endif /* dg_configLCDC_ADAPTER */

#endif /* dg_configUSE_T1D54BP002 */

#endif /* T1D54BP002_H_ */
