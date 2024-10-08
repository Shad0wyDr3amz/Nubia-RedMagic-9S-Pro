/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>

#include "cam_ois_dev.h"

#include "cam_sensor_io.h"
#include "cam_sensor_i2c.h"
#include "zte_camera_ois_util.h"
#include "ois_dw9781c.h"

#define CONFIG_ZTE_OIS_UTIL_DEBUG

#undef CDBG
#ifdef CONFIG_ZTE_OIS_UTIL_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define     GYRO_OFFSET_CAL_OK              0x00
#define     GYRO_CAL_TIME_OVER              0xFF
#define     X_GYRO_OFFSET_SPEC_OVER_NG      0x0001
#define     X_GYRO_RAW_DATA_CHECK           0x0010
#define     Y_GYRO_OFFSET_SPEC_OVER_NG      0x0002
#define     Y_GYRO_RAW_DATA_CHECK           0x0020
#define     GYRO_OFST_CAL_OVERCNT           10
#define     ADJ_OK                          0

int32_t RamWrite16A_ois(void *data, int addr, int value)
{
	msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *)data;
	int32_t rc = 0;

	rc = zte_cam_cci_i2c_write(&(ptr->s_ctrl->io_master_info),
			addr, value,
			CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		pr_err("%s:%d: i2c write 0x%x=0x%x failed", __func__, __LINE__, addr, value);
		return rc;
	}

	return rc;
}

int32_t RamRead16A_ois(void *data, int addr, uint32_t *value)
{
	msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *)data;
	int32_t rc = 0;

	rc = camera_io_dev_read(&(ptr->s_ctrl->io_master_info),
		addr, value, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,false);
	if (rc < 0) {
		pr_err("%s:%d: i2c read addr[%d] = 0x%x failed",
			__func__, __LINE__, addr, *value);
	}

	return rc;
}

void ois_reset_ois(void *data)
{
    pr_info("%s:%d: [dw9781c_ois_reset_ois] ois reset", __func__, __LINE__);

	RamWrite16A_ois(data, 0xD002, 0x0001); /* printfc reset */
	mdelay(4);
	RamWrite16A_ois(data, 0xD001, 0x0001); /* Active mode (DSP ON) */
	mdelay(25); /* ST gyro - over wait 25ms, default Servo On */
	RamWrite16A_ois(data, 0xEBF1, 0x56FA); /* User protection release */
}

void calibration_save_ois(void *data)
{
	pr_info("%s:%d: [dw9781c_calibration_save_ois] calibration save starting", __func__, __LINE__);
	RamWrite16A_ois(data, 0x7011, 0x4000); /* calibration mode */
	mdelay(1);
	RamWrite16A_ois(data, 0x7011, 0x00AA); /* select mode */
	mdelay(10);
	RamWrite16A_ois(data, 0x7010, 0x8000); /* start mode */
	mdelay(100);
	ois_reset_ois(data);
	pr_info("%s:%d: [dw9781c_calibration_save_ois] calibration save finish", __func__, __LINE__);
}

int ois_dw9781c_debugfs_cal_s(void *data)
{
    msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *) data;
	uint32_t Addr, status;
	uint32_t xOffset, yOffset;
	uint32_t OverCnt;
	int msg;
    int i;

	pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] gyro_offset_calibrtion starting", __func__, __LINE__);

    ptr->cal_info.cal_success = 0;
	for(i = 1; i < 3; i++)
	{
		/* Gyro offset */
		RamWrite16A_ois(data, 0x7011, 0x4015);
		RamWrite16A_ois(data, 0x7010, 0x8000);
		mdelay(100);
		msg = 0;
		OverCnt = 0;

		while (1)
		{
			Addr = 0x7036; status = 0;
			RamRead16A_ois(data, Addr, &status);
			if (status & 0x8000)
			{
				break;
			} /* it done! */
			else
			{
				mdelay(50); /* 100msec waiting */
			}
			if (OverCnt++ > GYRO_OFST_CAL_OVERCNT)
			{ /* when it take over 10sec .. break */
				msg = GYRO_CAL_TIME_OVER;
				break;
			}
		}
		if (msg == 0)
		{
			if (status & 0x8000)
			{
				if ( status == 0x8000 )
				{
					msg = GYRO_OFFSET_CAL_OK;
					pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] GYRO_OFFSET_CAL_OK", __func__, __LINE__);
					break;
				}
				else
				{
					if (status & 0x1)
					{
						msg += X_GYRO_OFFSET_SPEC_OVER_NG;
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] X_GYRO_OFFSET_SPEC_OVER_NG", __func__, __LINE__);
					}
					if (status & 0x2)
					{
						msg += Y_GYRO_OFFSET_SPEC_OVER_NG;
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] Y_GYRO_OFFSET_SPEC_OVER_NG", __func__, __LINE__);
					}
					if (status & 0x10)
					{
						msg += X_GYRO_RAW_DATA_CHECK;
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] X_GYRO_RAW_DATA_CHECK", __func__, __LINE__);
					}
					if (status & 0x20)
					{
						msg += Y_GYRO_RAW_DATA_CHECK;
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] Y_GYRO_RAW_DATA_CHECK", __func__, __LINE__);
					}
					if (i >= 2)
					{
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] gyro offset calibration-retry NG (%d times)", __func__, __LINE__, i);
					} else {
						pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] gyro offset calibration-retry NG (%d times)", __func__, __LINE__, i);
						msg = 0;
					}
				}
			}
		}
	}

	RamRead16A_ois(data, 0x70F8, &xOffset);
	RamRead16A_ois(data, 0x70F9, &yOffset);
	pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] msg : %d", __func__, __LINE__, msg);
	pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] x_gyro_offset: 0x%04X, y_gyro_offset : 0x%04X", __func__, __LINE__, xOffset, yOffset);
	pr_info("%s:%d:[dw9781c_gyro_offset_calibrtion] gyro_offset_calibrtion finished...Status = 0x%04X", __func__, __LINE__, status);

	if(msg == ADJ_OK)
	{
		calibration_save_ois(data);
        ptr->cal_info.cal_success = 1;
	}

	return msg;
}

int ois_dw9781c_debugfs_cal_g(void *data, u64 *val)
{
	msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *) data;

	ois_dw9781c_debugfs_cal_s(data);

	if (ptr->cal_info.cal_success == 1) {
		*val |= 0x100000000;
	} else {
		*val = 0;
	}
	pr_info("%s:%d: cal_success = %d  0x%llx",
		__func__, __LINE__, ptr->cal_info.cal_success, *val);
	return 0;
}

