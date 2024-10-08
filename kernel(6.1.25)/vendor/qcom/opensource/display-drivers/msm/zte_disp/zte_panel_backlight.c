/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include <drm/drm_mipi_dsi.h>
#include "../dsi/dsi_display.h"
#include "../sde/sde_encoder.h"
#include "zte_panel_backlight.h"
void zte_dsi_panel_convert_limit_bl(struct dsi_panel *panel, u32 *bl_lvl) {
	u32 bl_limit = panel->disp_feature->zte_lcd_bl_limit;
	u32 new_bl = 0;

	if (!bl_limit)
		return;
	
	if (bl_limit == panel->bl_config.brightness_max_level)
		return;

	new_bl = mult_frac(bl_limit, panel->bl_config.bl_max_level, panel->bl_config.brightness_max_level);

	if (*bl_lvl > new_bl) {
		*bl_lvl = new_bl;
		pr_info("[MSM_LCD] panel bl = %d in blsetting [%d,%d]\n", *bl_lvl, bl_limit, new_bl);
	}
}

void zte_dsi_panel_convert_hdr_bl(struct dsi_panel *panel, u32 *bl_lvl) {
	struct dsi_backlight_config *bl = &panel->bl_config;
	u32 bl_max = bl->bl_max_level;
	u32 zte_lcd_hdr, hdr_sunlight_lvl;
    bool hdr_video_enable;
	u32 hdr_video_bl = 0;
	int len = 0;
	
	if (panel->disp_feature->hbm_config == NULL)
		return;

	hdr_video_bl = panel->disp_feature->hbm_config->hdr_video_brightness;
	len = panel->disp_feature->hbm_config->hdr_threshold_list_len;
	zte_lcd_hdr = panel->disp_feature->zte_lcd_hdr;
	hdr_sunlight_lvl = panel->disp_feature->hbm_config->hdr_sunlight_lvl;
	hdr_video_enable = panel->disp_feature->hbm_config->hdr_video_enable;

	if (zte_lcd_hdr != 0 && hdr_video_bl) {
		if (hdr_video_enable) {
			*bl_lvl = mult_frac(*bl_lvl, hdr_video_bl, bl_max);
		} else if (hdr_sunlight_lvl && len >= 2) {
            switch (hdr_sunlight_lvl)
			{
			 case 2:
			 	*bl_lvl = mult_frac(*bl_lvl, panel->disp_feature->hbm_config->hdr_threshold_list[1], bl_max);
			 	break;
			 case 4:
				*bl_lvl = mult_frac(*bl_lvl, panel->disp_feature->hbm_config->hdr_threshold_list[len >> 1], bl_max);
				break;
			 case 6 :
				*bl_lvl = mult_frac(*bl_lvl, panel->disp_feature->hbm_config->hdr_threshold_list[len-1], bl_max);
				break;
			 default:
			    *bl_lvl = mult_frac(*bl_lvl, panel->disp_feature->hbm_config->hdr_threshold_list[0], bl_max);
				break;
			 }

			 if (*bl_lvl > panel->disp_feature->hbm_config->hdr_threshold_list[len-1])
			 	*bl_lvl = panel->disp_feature->hbm_config->hdr_threshold_list[len-1];

		}
		pr_info("[MSM_LCD] panel bl = %d in [video_mode, sunlight_mode] = [%d, %d]\n", *bl_lvl,
													panel->disp_feature->hbm_config->hdr_video_enable,
													panel->disp_feature->hbm_config->hdr_sunlight_lvl);
	}
}

int zte_dsi_panel_update_backlight(struct dsi_panel *panel,
	u32 bl_lvl)
{
	int rc = 0;
	unsigned long mode_flags = 0;
	struct mipi_dsi_device *dsi = NULL;
	static u32 last_brightness = 0;
	static bool need_dim = false;
	static bool hbm_status = false;
	static u32 last_fps = 0;
	int frame_time_us = 0;

	if (!panel || (bl_lvl > 0xffff)) {
		DSI_ERR("invalid params\n");
		return -EINVAL;
	}

	dsi = &panel->mipi_device;
	panel->saved_backlight = bl_lvl;

    if (panel->disp_feature->zte_panel_state != SDE_MODE_DPMS_OFF) {
		if (panel->is_hbm_enabled || panel->disp_feature->zte_lcd_hbm) {
			hbm_status = true;
			pr_info("MSM_LCD backlight donot set when HBM enabled, [lcd_hbm, hbm_layer] = [%d, %d]\n", panel->disp_feature->zte_lcd_hbm, panel->is_hbm_enabled);
			return rc;
		}

		if (panel->in_aod) {
			pr_info("MSM_LCD backlight do not set when panel in aod!");
			return rc;
		}

		if (bl_lvl == last_brightness && panel->disp_feature->zte_lcd_cur_fps != last_fps){
			pr_info("MSM_LCD backlight donot set when panel fps change\n");
			return rc;
		}
	}

	if (unlikely(panel->bl_config.lp_mode)) {
		mode_flags = dsi->mode_flags;
		dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	}

	if (panel->bl_config.bl_inverted_dbv)
		bl_lvl = (((bl_lvl & 0xff) << 8) | (bl_lvl >> 8));

	rc = mipi_dsi_dcs_set_display_brightness(dsi, bl_lvl);
	if (rc < 0)
		DSI_ERR("failed to update dcs backlight:%d\n", bl_lvl);

	if (unlikely(panel->bl_config.lp_mode))
		dsi->mode_flags = mode_flags;

    if (bl_lvl == 0)
	    need_dim = false;

	if (need_dim) {
        need_dim = false;
		frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);
		schedule_delayed_work(&panel->dim_work, usecs_to_jiffies(frame_time_us * 2));
	}

    if (panel->bl_config.bl_inverted_dbv)
		bl_lvl = (((bl_lvl & 0xff) << 8) | (bl_lvl >> 8));

	if ((last_brightness == 0 && bl_lvl != 0) || hbm_status)
	    need_dim = true;

	last_brightness = bl_lvl;
	last_fps = panel->disp_feature->zte_lcd_cur_fps;
	hbm_status = false;
	return rc;
}

void zte_lcd_gamespace_bl_limit(struct dsi_panel *panel, u32 bl_limit)
{
	u32 bl_bak_store;
	//u32 bl_bak = panel->bl_config.bl_level;
	u32 bl_bak, thermal_max_brightness;
	struct sde_connector *sde_conn;
	struct dsi_display *nubia_display = get_main_display();

    if (!nubia_display || !nubia_display->panel) {
		pr_err("msm_lcd Invalid display panel\n");
		return;
	}

	sde_conn = to_sde_connector(nubia_display->drm_conn);
	if(sde_conn && sde_conn->bl_device) {
		//backlight_update_status(sde_conn->bl_device);
		bl_bak = sde_conn->bl_device->props.brightness;
	} else {
		pr_err("msm_lcd Invalid sde_connector sde_conn\n");
		return;
    }

	thermal_max_brightness = sde_conn->thermal_max_brightness;

	if (bl_limit <= 0 || bl_limit > panel->bl_config.brightness_max_level) {
		panel->disp_feature->zte_lcd_bl_limit = panel->bl_config.brightness_max_level;
		pr_info("[MSM_LCD] gamespace_bl_limit value=%d error, do nothing\n", bl_limit);
		return;
	} else if (thermal_max_brightness > 0 && bl_limit > thermal_max_brightness) {
		panel->disp_feature->zte_lcd_bl_limit = bl_limit;
		pr_info("[MSM_LCD] gamespace_bl_limit=%d bigger thermal_max_brightness=%d do nothing\n", bl_limit, thermal_max_brightness);
		return;
	}

	if (panel->disp_feature->zte_lcd_bl_limit != bl_limit) {
		panel->disp_feature->zte_lcd_bl_limit = bl_limit;
		if (bl_bak > 0) {
			if (bl_bak > bl_limit) {
				//pr_info("[MSM_LCD] scale=%d,%d bl_bak %d to gamespace_bl_limit:%d\n", panel->bl_config.bl_scale, panel->bl_config.bl_scale_sv,
                //     bl_bak, panel->disp_feature->zte_lcd_bl_limit);
				bl_bak = bl_limit;
				bl_bak_store = mult_frac(bl_bak, panel->bl_config.bl_max_level, panel->bl_config.brightness_max_level);
                //scale backlight begin
                bl_bak_store = bl_bak_store * panel->bl_config.bl_scale / MAX_BL_SCALE_LEVEL;
                bl_bak_store = (u32)bl_bak_store * panel->bl_config.bl_scale_sv / MAX_SV_BL_SCALE_LEVEL;
                if (bl_bak_store > panel->bl_config.bl_max_level)
                    bl_bak_store = panel->bl_config.bl_max_level;
                if (bl_bak_store && (bl_bak_store < panel->bl_config.bl_min_level))
                    bl_bak_store = panel->bl_config.bl_min_level;
                //scale backlight end

				dsi_panel_set_backlight(panel, bl_bak_store);
                //dsi_display_set_backlight  //mutex_lock error
			} else {
				pr_info("[MSM_LCD] gamespace_bl_limit bl_scale_sv=%d,bl_bak:%d<=bl_limit:%d, do nothing\n", panel->bl_config.bl_scale_sv, bl_bak, bl_limit);
			}
		}
	} else {
		pr_info("[MSM_LCD] gamespace_bl_limit is same as old: %d, do nothing\n", bl_limit);
	}

	return;
}

