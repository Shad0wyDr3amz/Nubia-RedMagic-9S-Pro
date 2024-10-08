/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_work.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
/* HBM UEVENT */

enum hbm_event{
	LCD_STATUS_HBM_OFF_EVENT = 0,
	LCD_STATUS_HBM_ON_EVENT,
	LCD_STATUS_HBM_FG_RELEASE_EVENT,
	LCD_STATUS_HBM_FG_PRESS_EVENT,
	LCD_STATUS_HBM_MAX
};

enum aod_event {
	AOD_DATA_APPEAR = 0,
	AOD_DATA_DISAPPEAR,
	AOD_PM_ON,
	AOD_PM_OFF,
};

static const char *aod_event_type[] = {
	"AOD_DATA_APPEAR",
	"AOD_DATA_DISAPPEAR",
	"AOD_PM_ON",
	"AOD_PM_OFF"
};

void zte_panel_hbm_send_uevent(int mode, int ret);

void zte_panel_fps_send_uevent(int fps);

void zte_panel_hdr_send_uevent(int hdr_value);

void dimming_enable_work_handler(struct work_struct *work);

void zte_fod_uiready_event_work_handler(struct work_struct *work);

void zte_aod_event_work_handler(struct dsi_panel *panel, int aod_event);

void aod_enter_handler(struct work_struct *work);

void aod_exit_handler(struct work_struct *work);


