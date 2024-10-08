#ifndef _ZTE_LCD_COMMON_H_
#define _ZTE_LCD_COMMON_H_
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include "../dsi/dsi_panel.h"
#include "../dsi/dsi_display.h"
#include "zte_panel_work.h"
#include "zte_panel_feature.h"

#define SKIP_CREATE_NODE "no need to create node"
static const char *zte_node_string[ZTE_LCD_MAX_CTRL] = {
	"driver/lcd_id",
	"driver/lcd_hbm",
	"driver/lcd_hdr",
	"driver/lcd_color_gamut",
	"driver/lcd_fps",
	"driver/lcd_acl",
	"driver/lcd_aod_bl",
	"driver/lcd_lspot",
	"driver/lcd_od",
	SKIP_CREATE_NODE,
	SKIP_CREATE_NODE,
	SKIP_CREATE_NODE,
	"driver/lcd_state",
	"driver/lcd_bl_limit",
	"driver/lcd_second_irq_fps",
	"driver/lcd_spr"
};

static const char *zte_node_string_sec[ZTE_LCD_MAX_CTRL] = {
	"driver_sec/lcd_id",
	"driver_sec/lcd_hbm",
	"driver_sec/lcd_hdr",
	"driver_sec/lcd_color_gamut",
	"driver_sec/lcd_fps",
	"driver_sec/lcd_acl",
	"driver_sec/lcd_aod_bl",
	"driver_sec/lcd_lspot",
	"driver_sec/lcd_od",
	SKIP_CREATE_NODE,
	SKIP_CREATE_NODE,
	SKIP_CREATE_NODE,
	"driver_sec/lcd_state",
	"driver_sec/lcd_bl_limit",
	"driver_sec/lcd_second_irq_fps",
	"driver_sec/lcd_spr"
};

int zte_set_disp_parameter(struct dsi_panel *panel, u32 feature, u32 feature_mode, bool from_node);
void zte_disp_common_func(struct dsi_panel *panel);
#define LCD_PROC_FILE_DEFINE(name, nodeid) \
static ssize_t name##_proc_write(struct file *file, \
		const char __user *buffer, size_t count, loff_t *ppos) \
{ \
	char *tmp = kzalloc((count+1), GFP_KERNEL); \
	u32 mode; \
	struct dsi_display *display = NULL;\
	struct dsi_panel *panel = NULL;\
	if (!tmp) \
		return -ENOMEM; \
	if (copy_from_user(tmp, buffer, count)) { \
		kfree(tmp); \
		return -EFAULT; \
	} \
    if (nodeid == ZTE_LCD_BL_LIMIT) { \
		sscanf(tmp, "%d", &mode); \
	} else { \
		mode = *tmp - '0'; \
	} \
	display = get_main_display();\
	if (!display) {\
		DSI_ERR("MSM_LCD no primary display device\n");\
		return -ENODEV;\
	}\
	panel = display->panel;\
	if (!panel) {\
		DSI_ERR("MSM_LCD no primary panel device\n");\
		return -ENODEV;\
	}\
	zte_set_disp_parameter(panel, nodeid, mode, true); \
	kfree(tmp); \
	return count; \
} \
static ssize_t name##_proc_write_sec(struct file *file, \
		const char __user *buffer, size_t count, loff_t *ppos) \
{ \
	char *tmp = kzalloc((count+1), GFP_KERNEL); \
	u32 mode; \
	struct dsi_display *display = NULL;\
	struct dsi_panel *panel = NULL;\
	if (!tmp) \
		return -ENOMEM; \
	if (copy_from_user(tmp, buffer, count)) { \
		kfree(tmp); \
		return -EFAULT; \
	} \
    if (nodeid == ZTE_LCD_BL_LIMIT) { \
		sscanf(tmp, "%d", &mode); \
	} else { \
		mode = *tmp - '0'; \
	} \
	display = get_sec_display();\
	if (!display) {\
		DSI_ERR("MSM_LCD no sec display device\n");\
		return -ENODEV;\
	}\
	panel = display->panel;\
	if (!panel) {\
		DSI_ERR("MSM_LCD no sec panel device\n");\
		return -ENODEV;\
	}\
	zte_set_disp_parameter(panel, nodeid, mode, true); \
	kfree(tmp); \
	return count; \
} \
static int name##_show(struct seq_file *m, void *v) \
{ \
	struct dsi_display *display = NULL;\
	struct dsi_panel *panel = NULL;\
	display = get_main_display();\
	if (!display) {\
		DSI_ERR("No display device\n");\
		return -ENODEV;\
	}\
	panel = display->panel;\
	if (!panel) {\
		DSI_ERR("No panel device\n");\
		return -ENODEV;\
	}\
    if (nodeid == ZTE_LCD_INFO_CTRL) { \
		seq_printf(m, "panel_name=%s\n", panel->disp_feature->name); \
		pr_info("MSM_LCD node name: %s\n", panel->disp_feature->name); \
	} else { \
		seq_printf(m, "%d\n", panel->disp_feature->name); \
		pr_info("MSM_LCD node value: %d\n", panel->disp_feature->name); \
	} \
	return 0; \
} \
static int name##_show_sec(struct seq_file *m, void *v) \
{ \
	struct dsi_display *display = NULL;\
	struct dsi_panel *panel = NULL;\
	display = get_sec_display();\
	if (!display) {\
		DSI_ERR("No display device\n");\
		return -ENODEV;\
	}\
	panel = display->panel;\
	if (!panel) {\
		DSI_ERR("No panel device\n");\
		return -ENODEV;\
	}\
    if (nodeid == ZTE_LCD_INFO_CTRL) { \
		seq_printf(m, "panel_name=%s\n", panel->disp_feature->name); \
		pr_info("MSM_LCD node name: %s\n", panel->disp_feature->name); \
	} else { \
		seq_printf(m, "%d\n", panel->disp_feature->name); \
		pr_info("MSM_LCD node value: %d\n", panel->disp_feature->name); \
	} \
	return 0; \
} \
static int name##_proc_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, name##_show, NULL); \
} \
static int name##_proc_open_sec(struct inode *inode, struct file *file) \
{ \
	return single_open(file, name##_show_sec, NULL); \
} \
static const struct proc_ops name##_proc_fops = { \
  	.proc_open	= name##_proc_open, \
  	.proc_read	= seq_read, \
  	.proc_lseek	= seq_lseek, \
 	.proc_release	= single_release, \
	.proc_write	= name##_proc_write, \
}; \
static const struct proc_ops name##_proc_fops_sec = { \
  	.proc_open	= name##_proc_open_sec, \
  	.proc_read	= seq_read, \
  	.proc_lseek	= seq_lseek, \
 	.proc_release	= single_release, \
	.proc_write	= name##_proc_write_sec, \
}; \
static void name##_init(struct dsi_panel *panel) \
{ \
	if (!strcmp(panel->type, "primary")) {\
		proc_create(zte_node_string[nodeid], 0664, NULL, & name##_proc_fops); \
	} else {\
		proc_create(zte_node_string_sec[nodeid], 0664, NULL, & name##_proc_fops_sec); \
	}\
	return; \
}
#endif /* _ZTE_LCD_COMMON_H_ */

