#include "../dsi/dsi_display.h"
#include "zte_panel_feature.h"
#include "zte_panel_backlight.h"
#include "zte_panel_work.h"

int zte_hbm_ctrl_display(struct dsi_panel *panel, u32 setHbm, bool from_node){
    int err = 0;
    struct dsi_display_mode_priv_info *priv_info;
    struct dsi_cmd_desc *cmds = NULL;
    struct dsi_cmd_desc *aod_cmds = NULL;
    enum dsi_cmd_set_type type, aod_type;
    u8 *tx_buf;
    u8 *aod_tx_buf;
    u32 count, aod_count;

    if (panel->cur_mode)
		priv_info = panel->cur_mode->priv_info;
	else
		priv_info = NULL;

    if (setHbm == 0) {
        if (atomic_read(&panel->pm_aod))
            type = DSI_CMD_SET_ZTE_HBM_OFF_AOD_ON;
        else
            type = DSI_CMD_SET_ZTE_HBM_OFF;

        if (type == DSI_CMD_SET_ZTE_HBM_OFF) {
            if (panel->saved_backlight != 0x0 && priv_info){
                pr_info("[MSM_LCD] set HBM Brightness : %d\n",panel->saved_backlight);
                count = priv_info->cmd_sets[type].count;
				cmds = priv_info->cmd_sets[type].cmds;
                if (cmds && count >= 1) {
                    tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                    if (tx_buf && tx_buf[0] == 0x51) {
                        if (panel->in_aod) {
                            tx_buf[1] = 0;
                            tx_buf[2] = 0;
                        } else {
                            tx_buf[1] = panel->saved_backlight >> 8;
                            tx_buf[2] = panel->saved_backlight & 0xff;
                        }
                        pr_info("[MSM_LCD] HBM DSI_CMD_SET_ZTE_HBM_OFF 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
                    }
                }
            }
        } else {
            count = priv_info->cmd_sets[type].count;
			cmds = priv_info->cmd_sets[type].cmds;
            if (cmds && count >= 1) {
                tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                if (tx_buf && tx_buf[0] == 0x51) {
                    aod_type = DSI_CMD_SET_ZTE_AOD_LOW;
                    if (panel->disp_feature->zte_lcd_aod_bl == AOD_BL_MID) {
                       aod_type = DSI_CMD_SET_ZTE_AOD_MID;
                    } else if (panel->disp_feature->zte_lcd_aod_bl == AOD_BL_HIGH) {
                       aod_type = DSI_CMD_SET_ZTE_AOD_HIGH;
                    }
                    aod_count = priv_info->cmd_sets[aod_type].count;
                    aod_cmds = priv_info->cmd_sets[aod_type].cmds;
                    aod_tx_buf = (u8 *)aod_cmds[aod_count-1].msg.tx_buf;
                    tx_buf[1] = aod_tx_buf[1];
                    tx_buf[2] = aod_tx_buf[2];
                }
            }
        }
        err = zte_dsi_panel_tx_cmd_set(panel, type);
        if (err < 0) {
            pr_err("[MSM_LCD] HBM OFF: failed to send %d cmd, rc=%d\n", type, err);
            return err;
        } else {
            pr_info("[MSM_LCD] HBM OFF: success to send %d cmd\n ", type);
        }
    } else {
        if (atomic_read(&panel->pm_aod))
            type = DSI_CMD_SET_ZTE_AOD_OFF_HBM_ON;
        else
            type = DSI_CMD_SET_ZTE_HBM_ON;

        err = zte_dsi_panel_tx_cmd_set(panel, type);
        if (err < 0) {
			pr_err(" MSM_LCD HBM ON: failed to send %d cmd, rc=%d\n", type, err);
            return err;
		} else {
			pr_info("MSM_LCD HBM ON: success to send %d cmd.", type);
		}

        if (atomic_read(&panel->pm_aod))
            atomic_dec(&panel->pm_aod);
    }
    return err;
}


void zte_hdr_ctrl_display(struct dsi_panel *panel, u32 setHdr)
{
	u32 bl_lvl_bak = panel->bl_config.bl_level;
    char cal_value = panel->disp_feature->zte_lcd_hdr;

    switch (setHdr) {
    case HDR_VIDEO_OFF:
        cal_value &= 0xfe;/* clear bit(0) */
        panel->disp_feature->hbm_config->hdr_video_enable = false;
        break;
    case HDR_VIDEO_ON:
        cal_value |= 0x01;/* set bit(0) */
        panel->disp_feature->hbm_config->hdr_video_enable = true;
        break;
    case LBM:
    case MBM:
    case HBM: //enter sunlight mode
        cal_value &= 0xf;
        cal_value |= setHdr << 4;/* set bit(1) */
        panel->disp_feature->hbm_config->hdr_sunlight_lvl = setHdr;
        break;
    case SUNLIGHT_OFF :
        cal_value &= 0xf;/* clear bit(1) */
        panel->disp_feature->hbm_config->hdr_sunlight_lvl = 0;
        break;
    default:
        pr_err("[MSM_LCD] HDR invalid flag: %d\n", setHdr);
    break;
    }
    // modify by zte for sunlight status bar display sun icon begin
    if (cal_value == 0x20 || cal_value == 0x40 || cal_value == 0x60) {
      zte_panel_hdr_send_uevent(1);
	} else {
      zte_panel_hdr_send_uevent(0);
	}
    // modify by zte for sunlight status bar display sun icon end

	if (panel->disp_feature->zte_lcd_hdr != cal_value) {
		panel->disp_feature->zte_lcd_hdr = cal_value; //0x00,0x01,0x20,0x21,0x30,0x31,0x40,0x41,0x60,0x61
		if (bl_lvl_bak != 0) {
			dsi_panel_set_backlight(panel, bl_lvl_bak);
		}
		pr_info("[MSM_LCD] HDR flag new value:0x%x\n", panel->disp_feature->zte_lcd_hdr);
	} else {
		pr_info("[MSM_LCD] HDR flag is same as old:0x%x, do nothing\n", panel->disp_feature->zte_lcd_hdr);
	}
}

int zte_set_disp_parameter(struct dsi_panel *panel, u32 feature, u32 feature_mode, bool from_node) {
    int err = 0;
	struct dsi_display_mode_priv_info *priv_info;
	struct dsi_cmd_desc *cmds = NULL;
	u8 *tx_buf;
	u32 count;

    if (!panel) {
		pr_info("[MSM_LCD] No panel device\n");
		return -EINVAL;
	}

    if (panel->cur_mode)
		priv_info = panel->cur_mode->priv_info;
	else
		priv_info = NULL;

    mutex_lock(&panel->panel_lock);

    if (!panel->panel_initialized) {
		err = -EPERM;
		pr_err("[MSM_LCD] panel is off or not initialized feature=%d feature_mode=%d\n", feature, feature_mode);
		if (feature == ZTE_LCD_AOD_BL && feature_mode < AOD_BL_OFF) {
			panel->disp_feature->zte_lcd_aod_bl = feature_mode;
			pr_info("[MSM_LCD] save aod bl =%d\n", panel->disp_feature->zte_lcd_aod_bl);
		}
		goto error;
	}

    pr_info("[MSM_LCD] Send cmds [feature,feature_mode] = [%s,%d]\n", feature_name[feature], feature_mode);

    switch (feature) {
        case ZTE_LCD_HBM_CTRL:
			err = zte_hbm_ctrl_display(panel,feature_mode,from_node);
			if(from_node) {
				panel->disp_feature->zte_lcd_hbm = feature_mode;
            	pr_info("[MSM_LCD] HBM MODE is %d\n", panel->disp_feature->zte_lcd_hbm);
			}
            break;
        case ZTE_LCD_HDR_CTRL:
            zte_hdr_ctrl_display(panel,feature_mode);
            break;
        case ZTE_LCD_COLOR_GAMUT_CTRL:
            if (panel->disp_feature->zte_lcd_color_gamut != feature_mode) {
                switch(feature_mode) {
                    case COLOR_GAMUT_ORIGINAL:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_ORIGINAL);
                        break;
                    case COLOR_GAMUT_SRGB:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_SRGB);
                        break;
                    case COLOR_GAMUT_P3:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_P3);
                        break;
                    default:
                        pr_err("[MSM_LCD] Color gamut index %d not supported\n", feature_mode);
                        break;
                }
                panel->disp_feature->zte_lcd_color_gamut = feature_mode;
                pr_info("[MSM_LCD] COLOR GAMUT is %d\n", panel->disp_feature->zte_lcd_color_gamut);
            } else {
                pr_err("[MSM_LCD] COLOR GAMUT Repeat settings!");
            }
            break;
        case ZTE_LCD_ACL_CTRL:
            if (panel->disp_feature->zte_lcd_acl != feature_mode) {
                switch(feature_mode){
                    case ACL_OFF:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_OFF);
                        break;
                    case ACL_LOW:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_LOW);
                        break;
                    case ACL_MID:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_MID);
                        break;
                    case ACL_HIGH:
                        err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_HIGH);
                        break;
                    default:
                        pr_err("[MSM_LCD] ACL %d not supported\n", feature_mode);
                        break;
                }
                panel->disp_feature->zte_lcd_acl = feature_mode;
                pr_info("[MSM_LCD] ACL level is %d\n", panel->disp_feature->zte_lcd_acl);
            } else {
                pr_err("[MSM_LCD] ACL Repeat settings!");
            }
            break;
        case ZTE_LCD_AOD_BL:
            if (!panel->in_aod && atomic_read(&panel->pm_aod) == 0) {
               pr_info("[MSM_LCD] skip setting aod bl when panel not in aod!");
               break;
            }

            if (panel->disp_feature->zte_lcd_hbm == 1 || panel->is_hbm_enabled) {
                pr_info("[MSM_LCD] skip setting aod bl when panel in hbm!");
                break;
            }

            if (feature_mode < AOD_BL_OFF)
                panel->disp_feature->zte_lcd_aod_bl = feature_mode;

            switch(feature_mode) {
                case AOD_BL_LOW:
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW);
                    break;
                case AOD_BL_MID:
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_MID);
                    break;
                case AOD_BL_HIGH:
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_HIGH);
                    break;
                default:
                    pr_info("[MSM_LCD] AOD: please check AOD Brightness level\n");
                    break;
            }
            pr_info("[MSM_LCD] AOD Brightness is %d\n", panel->disp_feature->zte_lcd_aod_bl);
            break;
            case ZTE_LCD_RECOVERY_BL:
                if (priv_info && feature_mode != 0) {
                    if (panel->disp_feature->zte_lcd_hbm == 1 || panel->is_hbm_enabled){
                        panel->saved_backlight = feature_mode;
                        pr_info("[MSM_LCD] skip restore Brightness when hbm enabled!");
                        break;
                    }
                    pr_info("[MSM_LCD] restore Brightness : %d\n",feature_mode);

                    count = priv_info->cmd_sets[DSI_CMD_SET_ZTE_RESTORE_BL].count;
                    cmds = priv_info->cmd_sets[DSI_CMD_SET_ZTE_RESTORE_BL].cmds;
                    if (cmds && count > 1) {
                        tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                        if (tx_buf && tx_buf[0] == 0x51) {
                            tx_buf[1] = feature_mode >> 8;
                            tx_buf[2] = feature_mode & 0xff;
                            pr_info("[MSM_LCD] DSI_CMD_SET_ZTE_RESTORE_BL 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
                        }
                    }
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_RESTORE_BL);
                }
                break;
        case ZTE_LCD_LSPOT:
            if (feature_mode != panel->disp_feature->zte_lcd_lspot) {
                panel->disp_feature->zte_lcd_lspot = feature_mode;
                if (feature_mode)
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_FOD_ON);
                else
                    err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_FOD_OFF);
            } else {
                pr_info("[MSM_LCD] lspot setting is Repeated and don't work\n");
            }
			break;
        case ZTE_DIM_ON:
            err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_DIM_ON);
            break;
        case ZTE_LCD_BL_LIMIT:
            zte_lcd_gamespace_bl_limit(panel,feature_mode);
            break; 
        default:
            pr_info("[MSM_LCD] Unsupported index=%d\n", feature_mode);
            break;
	}
	mutex_unlock(&panel->panel_lock);

    if (err < 0) {
        pr_err("MSM_LCD Feature %d mode %d tx send fail\n", feature,feature_mode);
    } else {
        if (feature == ZTE_LCD_HBM_CTRL) {
            zte_panel_hbm_send_uevent(feature_mode, err);
        }
    }

	return err;
error:
	mutex_unlock(&panel->panel_lock);
	return err;
}