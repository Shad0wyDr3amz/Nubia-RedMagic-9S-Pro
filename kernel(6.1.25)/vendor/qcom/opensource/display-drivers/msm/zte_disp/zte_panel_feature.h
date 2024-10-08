/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_feature.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
/* ACL Status */
enum acl_status{
   ACL_OFF = 0,
   ACL_LOW,
   ACL_MID,
   ACL_HIGH,
   ACL_MAX
};

/* AOD Status */
enum aod_status{
    AOD_BL_LOW = 0,
    AOD_BL_MID,
    AOD_BL_HIGH,
    AOD_BL_OFF,
    AOD_BL_MAX
};

enum {
	ZTE_LCD_INFO_CTRL = 0,
	ZTE_LCD_HBM_CTRL,
	ZTE_LCD_HDR_CTRL,
	ZTE_LCD_COLOR_GAMUT_CTRL,
	ZTE_LCD_FPS_CTRL,
	ZTE_LCD_ACL_CTRL,
	ZTE_LCD_AOD_BL,
	ZTE_LCD_LSPOT,
	ZTE_LCD_OD,
	ZTE_LCD_RECOVERY_BL,
	ZTE_LCD_AOD_OFF_HBM_ON,
	ZTE_DIM_ON,
	ZTE_LCD_STATE_CTRL,
	ZTE_LCD_BL_LIMIT,
	ZTE_LCD_SECOND_IRQ_FPS,
	ZTE_LCD_SPR_CTRL,
	ZTE_LCD_MAX_CTRL
};

static const char *feature_name[ZTE_LCD_MAX_CTRL] = {
	"lcd_id",
	"lcd_hbm",
	"lcd_hdr",
	"lcd_color_gamut",
	"lcd_fps",
	"lcd_acl",
	"lcd_aod_bl",
	"lcd_lspot",
	"lcd_od",
	"bl_restore",
	"aodoff_hbmon",
	"dim_on",
	"lcd_state",
	"lcd_bl_limit",
	"lcd_second_irq_fps",
	"lcd_spr",
};

enum hdr_type {
	HDR_VIDEO_OFF = 0,
	HDR_VIDEO_ON,
	LBM = 0x2,
	SUNLIGHT_OFF = 0x3,
	MBM = 0x4,
	HBM = 0x6,
};

int zte_hbm_ctrl_display(struct dsi_panel *panel, u32 setHbm, bool from_node);

int zte_set_disp_parameter(struct dsi_panel *panel, u32 feature, u32 feature_mode, bool from_node);