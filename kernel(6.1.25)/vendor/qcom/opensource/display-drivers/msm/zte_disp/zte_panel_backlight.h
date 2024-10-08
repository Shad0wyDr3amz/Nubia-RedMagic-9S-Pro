/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/

void zte_dsi_panel_convert_hdr_bl(struct dsi_panel *panel, u32 *bl_lvl);

void zte_dsi_panel_convert_limit_bl(struct dsi_panel *panel, u32 *bl_lvl);

int zte_dsi_panel_update_backlight(struct dsi_panel *panel, u32 bl_lvl);

void zte_lcd_gamespace_bl_limit(struct dsi_panel *panel, u32 bl_limit);