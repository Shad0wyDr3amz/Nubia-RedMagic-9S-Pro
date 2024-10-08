#include "zte_display_panel.h"
#include "zte_lcd_reg_debug.h"

LCD_PROC_FILE_DEFINE(zte_lcd_info, ZTE_LCD_INFO_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_hbm, ZTE_LCD_HBM_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_hdr, ZTE_LCD_HDR_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_aod_bl, ZTE_LCD_AOD_BL)
LCD_PROC_FILE_DEFINE(zte_lcd_color_gamut, ZTE_LCD_COLOR_GAMUT_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_acl, ZTE_LCD_ACL_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_cur_fps, ZTE_LCD_FPS_CTRL)
LCD_PROC_FILE_DEFINE(zte_panel_state, ZTE_LCD_STATE_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_lspot, ZTE_LCD_LSPOT)
LCD_PROC_FILE_DEFINE(zte_lcd_bl_limit, ZTE_LCD_BL_LIMIT)


static int zte_disp_hbm_config(struct dsi_panel *panel)
{
	int rc = 0;
    u32 val = 0;
	struct dsi_parser_utils *utils = &panel->utils;
    int len = 0;

    panel->disp_feature->hbm_config = kzalloc(sizeof(struct disp_hbm_config), GFP_KERNEL);

    panel->disp_feature->hbm_config->global_hbm = utils->read_bool(utils->data, "zte,hbm_enabled");

    rc = utils->read_u32(utils->data, "zte,hdr_video_brightness", &val);
	if (!rc) {
		panel->disp_feature->hbm_config->hdr_video_brightness = val;
	}

    pr_info("MSM_LCD finish hbm config! global_hbm = %d, hdr_video_brightness = %d\n",
                                    panel->disp_feature->hbm_config->global_hbm,
                                    panel->disp_feature->hbm_config->hdr_video_brightness);

    len = utils->count_u32_elems(utils->data, "zte,hdr_threshold_list");
    if (!len) {
        pr_err("MSM_LCD no hdr threshold");
    } else {
        panel->disp_feature->hbm_config->hdr_threshold_list_len = len;
        panel->disp_feature->hbm_config->hdr_threshold_list = kcalloc(len, sizeof(u32),GFP_KERNEL);
        rc = utils->read_u32_array(utils->data,
  			    "zte,hdr_threshold_list",
  			    panel->disp_feature->hbm_config->hdr_threshold_list,
  			    panel->disp_feature->hbm_config->hdr_threshold_list_len);
        if (rc) {
            pr_err("MSM_LCD hdr threshold list parse failed!");
        } else {
            pr_info("MSM_LCD hdr threshold list parse success!");
            pr_info("MSM_LCD min_hdr_threshold = %d, max_hdr_threshold = %d\n",
                                   panel->disp_feature->hbm_config->hdr_threshold_list[0],
                                   panel->disp_feature->hbm_config->hdr_threshold_list[len-1]);
        }
    }
	return rc;
}

void zte_disp_feature_check(struct dsi_panel *panel) {
    bool feature_enabled;
	struct dsi_parser_utils *utils = &panel->utils;

    zte_lcd_info_init(panel);
    panel->disp_feature->zte_lcd_info = panel->name;
    pr_info("MSM_LCD panel name = %s\n", panel->disp_feature->zte_lcd_info);

    zte_panel_state_init(panel);

    feature_enabled = utils->read_bool(utils->data, "zte,hbm_enabled");
    if (feature_enabled) {
        zte_lcd_hbm_init(panel);
        panel->disp_feature->zte_lcd_hbm = 0;
        pr_info("MSM_LCD create HBM node\n");
        zte_disp_hbm_config(panel);
    }

    feature_enabled = utils->read_bool(utils->data, "zte,hdr_enabled");
    if (feature_enabled) {
        zte_lcd_hdr_init(panel);
        panel->disp_feature->zte_lcd_hdr = 0;
        pr_info("MSM_LCD create HDR node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,aod_enabled");
    if (feature_enabled) {
        zte_lcd_aod_bl_init(panel);
        panel->disp_feature->zte_lcd_aod_bl = AOD_BL_MID;
        pr_info("MSM_LCD create AOD node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,fps_enabled");
    if (feature_enabled) {
        zte_lcd_cur_fps_init(panel);
        panel->disp_feature->zte_lcd_cur_fps = 60;
        pr_info("MSM_LCD create FPS node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,color_space_enabled");
    if (feature_enabled) {
        zte_lcd_color_gamut_init(panel);
        panel->disp_feature->zte_lcd_color_gamut = COLOR_GAMUT_P3;
        pr_info("MSM_LCD create COLOR node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,acl_enabled");
    if (feature_enabled) {
        zte_lcd_acl_init(panel);
        panel->disp_feature->zte_lcd_acl = 0;
        pr_info("MSM_LCD create ACL node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,lspot_enabled");
    if (feature_enabled) {
        zte_lcd_lspot_init(panel);
        panel->disp_feature->zte_lcd_lspot = 0;
        pr_info("MSM_LCD create LSPOT node\n");
    }

    feature_enabled = utils->read_bool(utils->data, "zte,bl_limit_enabled");
    if (feature_enabled) {
        zte_lcd_bl_limit_init(panel);
        panel->disp_feature->zte_lcd_bl_limit = panel->bl_config.brightness_max_level;
		pr_info("MSM_LCD create bl_limit node\n");
    } else {
        panel->disp_feature->zte_lcd_bl_limit = 0;
    }
}

void zte_disp_config(struct dsi_panel *panel) {
    int rc = 0;
    struct dsi_parser_utils *utils = &panel->utils;

    rc = utils->read_u32(utils->data, "zte,te-high-width", &panel->vsync_width);
    if (rc)
		panel->vsync_width = 0;

	pr_info("MSM_LCD panel vsync width %d\n", panel->vsync_width);

    panel->bypass_gamut = utils->read_bool(utils->data, "zte,bypass_gamut");
    pr_info("MSM_LCD bypass_gamut =%d\n",  panel->bypass_gamut);
}

extern void zte_lcd_reg_debug_func(struct dsi_panel *panel);
void zte_disp_common_func(struct dsi_panel *panel) {

    if(!panel) {
        pr_info("MSM_LCD No panel device\n");
		return;
    }

    panel->disp_feature = kzalloc(sizeof(struct zte_disp_feature), GFP_KERNEL);

    if (!panel->disp_feature) {
  		pr_err("%s: %d MSM_LCD kzalloc memory failed\n", __func__, __LINE__);
  		return;
  	}

    zte_disp_feature_check(panel);

    zte_disp_config(panel);

    zte_lcd_reg_debug_func(panel);

    INIT_DELAYED_WORK(&panel->dim_work, dimming_enable_work_handler);
    INIT_DELAYED_WORK(&panel->enter_aod_work, aod_enter_handler);
    INIT_DELAYED_WORK(&panel->exit_aod_work, aod_exit_handler);

    panel->fod_uiready_wq = create_singlethread_workqueue("fod_uiready_event0");
    INIT_DELAYED_WORK(&panel->fod_uiready_delayed_work, zte_fod_uiready_event_work_handler);

    atomic_set(&panel->pm_aod, 0);
}