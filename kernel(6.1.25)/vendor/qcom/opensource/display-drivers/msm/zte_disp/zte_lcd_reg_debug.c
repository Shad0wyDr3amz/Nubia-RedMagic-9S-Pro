#include "zte_lcd_reg_debug.h"
#include "../dsi/dsi_display.h"
struct zte_lcd_reg_debug zte_lcd_reg_debug;
/*
*echo ff988100 > gwrite (0x13,0x29) or echo 51ff > dwrite (0x15,0x39)
*echo 5401 > dread(0x14,0x24), then cat dread
*dread (0x06) sometimes read nothing,return error
*file path: sys/reg_debug
*/
ssize_t zte_dsi_reg_dread(struct dsi_panel *panel, struct zte_lcd_reg_debug *reg_debug, bool hs)
{
	int rc = 0;
	int i = 0;
	u8 *tx_buf;
	struct dsi_read_config ld_read_config;

	while(!panel->cur_mode || !panel->cur_mode->priv_info || !panel->panel_initialized) {
		pr_debug("[%s][%s] waitting for panel priv_info initialized!\n", __func__, panel->name);
		msleep_interruptible(1000);
	}

	mutex_lock(&panel->panel_lock);
	if (panel->cur_mode->priv_info->cmd_sets[DSI_CMD_SET_REG_READ].cmds) {
		ld_read_config.is_read = 1;
		ld_read_config.cmds_rlen = reg_debug->wbuf[1];
		ld_read_config.read_cmd.cmds = panel->cur_mode->priv_info->cmd_sets[DSI_CMD_SET_REG_READ].cmds;
		ld_read_config.read_cmd.count = 1;

        if(hs)
          ld_read_config.read_cmd.state = DSI_CMD_SET_STATE_HS;
		else
          ld_read_config.read_cmd.state = DSI_CMD_SET_STATE_LP;

        tx_buf = (u8 *)ld_read_config.read_cmd.cmds[0].msg.tx_buf;
		tx_buf[0] = reg_debug->wbuf[0];
		pr_info("MSM_LCD read reg addr 0x%02x, length = %d", tx_buf[0], ld_read_config.cmds_rlen);
		rc = dsi_panel_read_cmd_set(panel, &ld_read_config);
		if (rc <= 0) {
			pr_err("[%s][%s] failed to read cmds, rc=%d\n", __func__, panel->name, rc);
			rc = -EIO;
			goto done;
		}

		for(i = 0; i < reg_debug->length; i++) {
			pr_info("[%s][%d]0x%02x", __func__, __LINE__, ld_read_config.rbuf[i]);
			reg_debug->rbuf[i] = ld_read_config.rbuf[i];
		}
	}
done:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

static void zte_lcd_reg_rw_func(struct dsi_panel *ctrl, struct zte_lcd_reg_debug *reg_debug)
{

	int i;
	struct mipi_dsi_device *dsi;
    unsigned long mode_flags = 0;

	if ((!reg_debug) || (!ctrl))
		return;

	dsi = &ctrl->mipi_device;

	/*if debug this func,define ZTE_LCD_REG_DEBUG 1*/
	for (i = 0; i < reg_debug->length; i++)
		pr_info("wbuf[%d]= %x\n", i, reg_debug->wbuf[i]);

	if (!ctrl->panel_initialized) {
		pr_err("MSM_LCD panel is off or not initialized reg read write\n");
		return;
	}

	switch (reg_debug->is_read_mode) {
	case REG_READ_MODE:
        zte_dsi_reg_dread(ctrl, reg_debug, true);
		break;
	case REG_WRITE_MODE:
		mipi_dsi_dcs_write(dsi, reg_debug->wbuf[0], &reg_debug->wbuf[1], reg_debug->length - 1);
		break;
    case REG_READ_MODE_LP:
		zte_dsi_reg_dread(ctrl, reg_debug, false);
		break;
	case REG_WRITE_MODE_LP:
		mode_flags = dsi->mode_flags;
		dsi->mode_flags |= MIPI_DSI_MODE_LPM;

		mipi_dsi_dcs_write(dsi, reg_debug->wbuf[0], &reg_debug->wbuf[1], reg_debug->length - 1);

		dsi->mode_flags = mode_flags;
		break;
	default:
		pr_err("%s:rw error\n", __func__);
		break;
	}
}

static void get_user_sapce_data(const char *buf, size_t count)
{
	int i = 0, length = 0;
	char lcd_status[ZTE_REG_LEN*2] = { 0 };

	if (count >= sizeof(lcd_status)) {
		pr_info("count=%zu,sizeof(lcd_status)=%zu\n", count, sizeof(lcd_status));
		return;
	}

	strlcpy(lcd_status, buf, count);
	memset(zte_lcd_reg_debug.wbuf, 0, ZTE_REG_LEN);
	memset(zte_lcd_reg_debug.rbuf, 0, ZTE_REG_LEN);

	/*if debug this func,define ZTE_LCD_REG_DEBUG 1*/
	#ifdef ZTE_LCD_REG_DEBUG
	for (i = 0; i < count; i++)
		pr_info("lcd_status[%d]=%c  %d\n", i, lcd_status[i], lcd_status[i]);
	#endif
	for (i = 0; i < count; i++) {
		if (isdigit(lcd_status[i]))
			lcd_status[i] -= '0';
		else if (isalpha(lcd_status[i]))
			lcd_status[i] -= (isupper(lcd_status[i]) ? 'A' - 10 : 'a' - 10);
	}
	for (i = 0, length = 0; i < (count-1); i = i+2, length++) {
		zte_lcd_reg_debug.wbuf[length] = lcd_status[i]*16 + lcd_status[1+i];
	}

	zte_lcd_reg_debug.length = length; /*length is use space write data number*/
}

static void zte_panel_reg_handle(struct dsi_panel *panel, const char *buf, size_t count, int type) {
	int i = 0, length = 0;

	get_user_sapce_data(buf, count);

	switch(type) {
		case REG_READ_MODE:
		case REG_READ_MODE_LP:
			length = zte_lcd_reg_debug.wbuf[1];
			if (length < 1) {
				pr_err("%s:read length is 0\n", __func__);
				return;
			}
			zte_lcd_reg_debug.is_read_mode = type;
			pr_info("read cmd = %x length = %x\n", zte_lcd_reg_debug.wbuf[0], length);
			zte_lcd_reg_rw_func(panel, &zte_lcd_reg_debug);

			zte_lcd_reg_debug.length = length;
			for (i = 0; i < length; i++)
				pr_info("read zte_lcd_reg_debug.rbuf[%d]=0x%02x\n", i, zte_lcd_reg_debug.rbuf[i]);
			break;
		case REG_WRITE_MODE:
		case REG_WRITE_MODE_LP:
			length = zte_lcd_reg_debug.length;
			zte_lcd_reg_debug.is_read_mode = type;
			zte_lcd_reg_rw_func(panel, &zte_lcd_reg_debug);
			pr_info("write cmd = 0x%02x,length = 0x%02x\n", zte_lcd_reg_debug.wbuf[0], length);
			break;
	}
}

static ssize_t sysfs_show_read(struct kobject *kobj,
		 struct kobj_attribute *attr, char *buf)
{
	int i = 0, len = 0, count = 0;
	char *s = NULL;
	char *data_buf = NULL;

	data_buf = kzalloc(ZTE_REG_LEN * REG_MAX_LEN, GFP_KERNEL);
	if (!data_buf)
		return -ENOMEM;

	s = data_buf;
	for (i = 0; i < zte_lcd_reg_debug.length; i++) {
		len = snprintf(s, 20, "rbuf[%02d]=%02x ", i, zte_lcd_reg_debug.rbuf[i]);
		s += len;
		if ((i+1)%8 == 0) {
			len = snprintf(s, 20, "\n");
			s += len;
		}
	}

	count = snprintf(buf, PAGE_SIZE, "read back:\n%s\n", data_buf);
	kfree(data_buf);
	return count;
}

static ssize_t sysfs_store_dread(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_main_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_READ_MODE);

	return count;
}

static ssize_t sysfs_store_dwrite(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_main_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_WRITE_MODE);
	return count;
}

static ssize_t sysfs_store_dreadlp(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_main_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_READ_MODE_LP);
	return count;
}

static ssize_t sysfs_store_dwritelp(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_main_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_WRITE_MODE_LP);
	return count;
}

static ssize_t sysfs_store_sec_dread(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_sec_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_READ_MODE);

	return count;
}

static ssize_t sysfs_store_sec_dwrite(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_sec_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_WRITE_MODE);
	return count;
}

static ssize_t sysfs_store_sec_dreadlp(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_sec_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_READ_MODE_LP);
	return count;
}

static ssize_t sysfs_store_sec_dwritelp(struct kobject *kobj,
		 struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct dsi_display *display;
	struct dsi_panel *panel;

	display = get_sec_display();

	if (!display) {
		DSI_ERR("No display device\n");
		return -ENODEV;
	}

	panel = display->panel;

	if (!panel) {
		DSI_ERR("No panel device\n");
		return -ENODEV;
	}

	zte_panel_reg_handle(panel, buf, count, REG_WRITE_MODE_LP);
	return count;
}

struct kobj_attribute primary_debug_attrs[] = {
    __ATTR(dread, 0664, sysfs_show_read, sysfs_store_dread),
    __ATTR(dwrite, 0664, NULL, sysfs_store_dwrite),
    __ATTR(dreadlp, 0664, sysfs_show_read, sysfs_store_dreadlp),
    __ATTR(dwritelp, 0664, NULL, sysfs_store_dwritelp),
};

struct kobj_attribute sec_debug_attrs[] = {
    __ATTR(dread, 0664, sysfs_show_read, sysfs_store_sec_dread),
    __ATTR(dwrite, 0664, NULL, sysfs_store_sec_dwrite),
    __ATTR(dreadlp, 0664, sysfs_show_read, sysfs_store_sec_dreadlp),
    __ATTR(dwritelp, 0664, NULL, sysfs_store_sec_dwritelp),
};

void create_sys_file(int id) {
	int ret = -1;
	int attr_count;
	struct kobject *vkey_obj = NULL;

	if(id)
		vkey_obj = kobject_create_and_add(SEC_REG, NULL);
	else
		vkey_obj = kobject_create_and_add(PRI_REG, NULL);

	if (!vkey_obj) {
		pr_err("%s:unable to create kobject\n", __func__);
		return;
	}

	if (id) {
		for (attr_count = 0; attr_count < ARRAY_SIZE(sec_debug_attrs); attr_count++) {
			ret = sysfs_create_file(vkey_obj, &sec_debug_attrs[attr_count].attr);
			if (ret < 0) {
				pr_err("failed to create sysfs attributes\n");
			}
		}
	} else {
		for (attr_count = 0; attr_count < ARRAY_SIZE(primary_debug_attrs); attr_count++) {
			ret = sysfs_create_file(vkey_obj, &primary_debug_attrs[attr_count].attr);
			if (ret < 0) {
				pr_err("failed to create sysfs attributes\n");
			}
		}
	}
}

void zte_lcd_reg_debug_func(struct dsi_panel *panel)
{

	if (!strcmp(panel->type, "primary")) {
		create_sys_file(PRI_FILE);
	} else {
		create_sys_file(SEC_FILE);
	}
}
