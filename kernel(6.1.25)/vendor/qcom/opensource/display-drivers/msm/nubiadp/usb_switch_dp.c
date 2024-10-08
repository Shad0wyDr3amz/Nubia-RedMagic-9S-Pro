// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2099, Nubia ltd. All rights reserved.
 */
#if IS_ENABLED(CONFIG_NUBIA_DP)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "../nubiadp/usb_switch_dp.h"

struct dp_switch_priv {
	struct platform_device *pdev;
	struct device *dev;
	bool is_enabled;
} *dp_switch = NULL;

#if IS_ENABLED(CONFIG_QCOM_FSA4480_I2C)
int dp_switch_event(struct device_node *node, enum fsa_function event)
{
	if (!dp_switch){
        pr_info("%s(),dp_switch is NULL\n", __func__);
		return -EINVAL;
    }
	switch (event) {
	case FSA_USBC_ORIENTATION_CC1:
	case FSA_USBC_ORIENTATION_CC2:
		dp_switch->is_enabled = 1;
		break;
	case FSA_USBC_DISPLAYPORT_DISCONNECTED:
		dp_switch->is_enabled = 0;
		break;
	default:
		break;
	}
    pr_info("%s(),event=%d,is_enabled=%d\n", __func__, event, dp_switch->is_enabled);
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_QCOM_WCD939X_I2C)
int dp_switch_event(struct device_node *node, enum wcd_usbss_cable_status status)
{
	if (!dp_switch){
        pr_info("%s(),dp_switch is NULL\n", __func__);
		return -EINVAL;
    }
	switch (status) {
	case WCD_USBSS_CABLE_CONNECT:
		dp_switch->is_enabled = 1;
		break;
	case WCD_USBSS_CABLE_DISCONNECT:
		dp_switch->is_enabled = 0;
		break;
	default:
		break;
	}
    pr_info("%s(),status=%d,is_enabled=%d\n", __func__, status, dp_switch->is_enabled);
	return 0;
}
#endif
EXPORT_SYMBOL(dp_switch_event);

static int parse_usb_dp_switch_dt(struct device_node *node)
{
	int rc = 0;
	return rc;
}

static ssize_t dp_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", dp_switch->is_enabled);
}

#if 0
static ssize_t dp_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;

	if (kstrtoint(buf, 0, &val))
		return -EINVAL;
	switch (val) {
	case 0:
	case 1:
		break;
	default:
		dev_err(dev, "Invalid argument\n");
		return -EINVAL;
	}
	return count;
}
#endif

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO, dp_enable_show, NULL);

static struct attribute *usb_switch_dp_attributes[] = {
		&dev_attr_enable.attr, NULL
};

static struct attribute_group usb_switch_dp_attribute_group = {
		.attrs = usb_switch_dp_attributes
};

static int usb_switch_dp_probe(struct platform_device *pdev)
{
	int rc = 0;
	int ret;
	struct device_node *node;

	pr_info("[msm-dp-info]%s: +\n", __func__);
	if (!pdev) {
		pr_err("[msm-dp-err]%s pdev is null\n", __func__);
		return -EPROBE_DEFER;
	}
	node = pdev->dev.of_node;
	dp_switch = kzalloc(sizeof(struct dp_switch_priv), GFP_KERNEL);
	if (!dp_switch) {
		dev_err(&pdev->dev, "cann't allocate device memory\n");
		return -ENOMEM;
	}
	dp_switch->dev = &pdev->dev;
	dp_switch->pdev = pdev;
	dp_switch->is_enabled = 0;
	rc = parse_usb_dp_switch_dt(node);
	if (rc < 0)
		return rc;
	ret = sysfs_create_group(&dp_switch->dev->kobj,
			&usb_switch_dp_attribute_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s error creating sysfs attr files\n",
				__func__);
	}
	pr_info("[msm-dp-info]%s: -\n", __func__);
	return 0;
}

static int usb_switch_dp_remove(struct platform_device *pdev)
{
	if (!dp_switch)
		return -EINVAL;
	sysfs_remove_group(&dp_switch->dev->kobj,
			&usb_switch_dp_attribute_group);
	return 0;
}

static const struct of_device_id of_match[] = {
	{ .compatible = "nubia,usb_switch_dp" },
	{}
};

static struct platform_driver usb_switch_dp_driver = {
	.probe = usb_switch_dp_probe,
	.remove = usb_switch_dp_remove,
	.driver = {
		.name = "nubia,usb_switch_dp",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_match),
	},
};

int __init nubia_usb_switch_dp_init(void)
{
	platform_driver_register(&usb_switch_dp_driver);
	return 0;
}

void __exit nubia_usb_switch_dp_exit(void)
{
	platform_driver_unregister(&usb_switch_dp_driver);
}

MODULE_AUTHOR("NUBIA");
MODULE_DESCRIPTION("Nubia DisplayPort");
MODULE_LICENSE("GPL");

#endif //CONFIG_NUBIA_DP
