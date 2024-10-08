// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2099, Nubia ltd. All rights reserved.
 */

#ifndef USB_SWITCH_DP_H
#define USB_SWITCH_DP_H

#if IS_ENABLED(CONFIG_NUBIA_DP)

#include <linux/of.h>
#include <linux/notifier.h>

#if IS_ENABLED(CONFIG_QCOM_FSA4480_I2C)
#include <linux/soc/qcom/fsa4480-i2c.h>
#elif IS_ENABLED(CONFIG_QCOM_WCD939X_I2C)
#include <linux/soc/qcom/wcd939x-i2c.h>
#endif

#include "dp_aux.h"
#include "dp_hpd.h"
#include "dp_debug.h"


#if IS_ENABLED(CONFIG_QCOM_FSA4480_I2C)
int dp_switch_event(struct device_node *node, enum fsa_function event);
#endif

#if IS_ENABLED(CONFIG_QCOM_WCD939X_I2C)
int dp_switch_event(struct device_node *node, enum wcd_usbss_cable_status status);
#endif

#endif //CONFIG_NUBIA_DP

#endif
