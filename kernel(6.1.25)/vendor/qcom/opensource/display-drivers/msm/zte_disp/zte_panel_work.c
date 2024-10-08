#include <drm/drm_mipi_dsi.h>
#include "../dsi/dsi_display.h"
#include "../sde/sde_trace.h"
#include "zte_display_panel.h"

struct device *dsi_uevent_device = NULL;
void zte_panel_hbm_send_uevent(int mode, int ret)
{
	char *envp[3];

	if (mode == LCD_STATUS_HBM_OFF_EVENT)
		envp[0] = "HBM_STATUS=OFF";
	else if (mode == LCD_STATUS_HBM_ON_EVENT)
		envp[0] = "HBM_STATUS=ON";
	else if (mode == LCD_STATUS_HBM_FG_RELEASE_EVENT)
		envp[0] = "HBM_STATUS=FG_RELEASE";
	else if (mode == LCD_STATUS_HBM_FG_PRESS_EVENT)
		envp[0] = "HBM_STATUS=FG_PRESS";
	else
		envp[0] = "HBM_STATUS=ERR";

	if (ret == 0)
		envp[1] = "HBM_SET_RESULT=SUCCESSFUL";
	else
		envp[1] = "HBM_SET_RESULT=FAILED";

	envp[2] = NULL;

	if (dsi_uevent_device) {
            kobject_uevent_env(&dsi_uevent_device->kobj, KOBJ_CHANGE, envp);
            pr_info("[MSM_LCD]HBM: uevent = %s , %s\n", envp[0], envp[1]);
        }
	else
		pr_info("[MSM_LCD]HBM: dsi_uevent_device is NULL\n");
}

void zte_panel_fps_send_uevent(int fps)
{
	char *envp[3];
	if (fps == 60)
		envp[0] = "LCD_FPS=60";
	else if (fps == 90)
		envp[0] = "LCD_FPS=90";
	else if (fps == 120)
		envp[0] = "LCD_FPS=120";
	else if (fps == 144)
		envp[0] = "LCD_FPS=144";

	envp[1] = NULL;
	envp[2] = NULL;

	if (dsi_uevent_device){
		kobject_uevent_env(&dsi_uevent_device->kobj, KOBJ_CHANGE, envp);
		pr_info("[MSM_LCD]FPS: send fps %d\n",fps);
	} else
		pr_info("[MSM_LCD]FPS: uevent_device is NULL, LCD state send faild\n");
}

void zte_panel_hdr_send_uevent(int hdr_value)
{
	char *envp[3];
	if (hdr_value == 1)
		envp[0] = "HDR_SUNLIGHT=ON";
	else if (hdr_value == 0)
		envp[0] = "HDR_SUNLIGHT=OFF";

	envp[1] = NULL;
	envp[2] = NULL;

	if (dsi_uevent_device){
		kobject_uevent_env(&dsi_uevent_device->kobj, KOBJ_CHANGE, envp);
		pr_info("[MSM_LCD]HDR_SUNLIGHT: send hdr_value uevent %s\n", envp[0]);
	} else
		pr_info("[MSM_LCD]HDR_SUNLIGHT: uevent_device is NULL, LCD state send faild\n");
}

void dimming_enable_work_handler(struct work_struct *work) {
	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, dim_work.work);

	if (!panel) {
		DSI_ERR("MSM_LCD no primary panel device\n");
		return;
	}

    pr_info("[MSM_LCD] handle dim\n");

    zte_set_disp_parameter(panel, ZTE_DIM_ON, 1, false);

    return;
}

void aod_enter_handler(struct work_struct *work) {

	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, enter_aod_work.work);

	if (!panel) {
		DSI_ERR("MSM_LCD no primary panel device\n");
		return;
	}

    pr_info("[MSM_LCD] handle aod_enter\n");

    zte_set_disp_parameter(panel, ZTE_LCD_AOD_BL, panel->disp_feature->zte_lcd_aod_bl, false);

	panel->enter_aod_worked = true;

    return;
}

void aod_exit_handler(struct work_struct *work) {

	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, exit_aod_work.work);

	if (!panel) {
		DSI_ERR("MSM_LCD no primary panel device\n");
		return;
	}

    pr_info("[MSM_LCD] handle aod_exit\n");

    zte_set_disp_parameter(panel, ZTE_LCD_RECOVERY_BL, panel->saved_backlight, false);

	panel->exit_aod_worked = true;
	panel->in_aod = false;

    return;
}

void zte_fod_uiready_event_work_handler(struct work_struct *work)
{
	struct dsi_display *display = NULL;
	struct dsi_panel *panel = NULL;

	display = get_main_display();

	if (!display) {
		DSI_ERR("MSM_LCD no primary display device\n");
		return;
	}

	panel = display->panel;
	if (!panel) {
		DSI_ERR("MSM_LCD no primary panel device\n");
		return;
	}

	pr_info("[MSM_LCD] handle fod ui ready event\n");

	SDE_ATRACE_BEGIN("zte_fod_event");
	if (panel->fod_layer)
		zte_panel_hbm_send_uevent(LCD_STATUS_HBM_FG_PRESS_EVENT,0);
	else
		zte_panel_hbm_send_uevent(LCD_STATUS_HBM_FG_RELEASE_EVENT,0);
	SDE_ATRACE_END("zte_fod_event");
	return;
}


void zte_aod_event_work_handler(struct dsi_panel *panel, int aod_event)
{
	static bool aod_status,  aod_data_status;
	int frame_time_us = 0;
	int panel_pm;

	if (!panel) {
		DSI_ERR("MSM_LCD no panel device\n");
		return;
	}

	panel_pm = panel->disp_feature->zte_panel_state;

	/* when lp cmds sent to ddic, will insert black frame. */
	frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);

    pr_info("[MSM_LCD] handle aod event = %s\n", aod_event_type[aod_event]);
    switch (aod_event)
	{
		case AOD_DATA_APPEAR:
			panel->exit_aod_worked = false;
			aod_data_status = true;
			if (aod_status && !panel->enter_aod_worked) {
				schedule_delayed_work(&panel->enter_aod_work, usecs_to_jiffies(frame_time_us));
			}
			break;
		case AOD_DATA_DISAPPEAR:
			panel->enter_aod_worked = false;
			aod_data_status = false;
			if (panel_pm == SDE_MODE_DPMS_ON) {
				if (!aod_status && !panel->exit_aod_worked) {
					schedule_delayed_work(&panel->exit_aod_work, usecs_to_jiffies(frame_time_us));
				}
			}
			break;
		case AOD_PM_ON:
			panel->exit_aod_worked = false;
			aod_status = true;
			if (aod_data_status && !panel->enter_aod_worked) {
				schedule_delayed_work(&panel->enter_aod_work, usecs_to_jiffies(frame_time_us << 1));
			}
			break;
		case AOD_PM_OFF:
			panel->enter_aod_worked = false;
			aod_status = false;
			if (panel_pm == SDE_MODE_DPMS_ON) {
				if (!aod_data_status && !panel->exit_aod_worked) {
					schedule_delayed_work(&panel->exit_aod_work, usecs_to_jiffies(frame_time_us << 1));
				}
			}
			break;
		default:
			break;
	}
	return;
}
