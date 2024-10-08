/* FS-Cache statistics viewing interface
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static unsigned int smartpa_t_cnt = 0;
static unsigned int smartpa_t_stat = 0;

/*
 * add the smartpa's state
 */
void smartpa_count_add_t(void)
{
	smartpa_t_cnt++;
}
EXPORT_SYMBOL(smartpa_count_add_t);

/*
 * update the smartpa's state
 */
void smartpa_stat_update_t(void)
{
	smartpa_t_stat++;
}
EXPORT_SYMBOL(smartpa_stat_update_t);

/*
 * show the smart's state
 */
static int smartpa_stat_t_show(struct seq_file *m, void *v)
{
	pr_info("%s, smartpa_t_cnt = %d smartpa_t_stat = %d", __func__,
                smartpa_t_cnt, smartpa_t_stat);
	if (smartpa_t_cnt > 0) {
		seq_printf(m, "%d\n", (smartpa_t_cnt == smartpa_t_stat));
	} else {
		seq_printf(m, "%d\n", 0);
	}

	return 0;
}

static int smartpa_stat_t_open(struct inode *inode, struct file *file)
{
	return single_open(file, smartpa_stat_t_show, NULL);
}

const struct  proc_ops smartpa_stat_fops = {
	.proc_open		= smartpa_stat_t_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release   = single_release,
};

/*
 * initialise the /proc/driver/smartpa
 */
static int __init smartpa_stat_t_init(void)
{
	pr_info("%s enter", __func__);

	if (!proc_create_data("driver/smarttest", S_IRUGO, NULL, &smartpa_stat_fops, NULL)) {
		pr_err("%s: Failed to create /proc/driver/smarttest", __func__);
		return -ENOMEM;
	}

	pr_info("%s exit", __func__);
	return 0;
}

/*
 * clean up the /proc/driver/smarttest
 */
static void smartpa_stat_t_cleanup(void)
{
	remove_proc_entry("driver/smarttest", NULL);
}

module_init(smartpa_stat_t_init);
module_exit(smartpa_stat_t_cleanup);

MODULE_DESCRIPTION("ZTE smartpa board test");
MODULE_AUTHOR("weiguohua <wei.guohua@zte.com.cn>");
MODULE_LICENSE("GPL");
