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
#include "ois_dw9784_hi847.h"

#define CONFIG_ZTE_OIS_UTIL_DEBUG

#undef CDBG
#ifdef CONFIG_ZTE_OIS_UTIL_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

int32_t RamWrite16A_ois_dw9784_hi847(void *data, int addr, int value)
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

int32_t RamRead16A_ois_dw9784_hi847(void *data, int addr, uint32_t *value)
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

int dw9784_hi847_wait_check_register(void *data, int addr, uint32_t value)
{
	/*
	reg : read target register
	ref : compare reference data
	*/
	uint32_t r_data;
	int i = 0;

	for (i = 0; i < LOOP_A; i++) {
		RamRead16A_ois_dw9784_hi847(data, addr, &r_data);  //  Read status
		if (r_data == value) {
			break;
		}
		else {
			if (i >= LOOP_B) {
				pr_err("%s:%d: [dw9784_hi847_wait_check_register]fail: 0x%04X",
					__func__, __LINE__, r_data);
				return FUNC_FAIL;
			}
		}
		mdelay(100);
	}
	return FUNC_PASS;
}

void dw9784_hi847_code_pt_off(void *data)
{
	pr_info("%s:%d: [dw9784_hi847_code_pt_off] start", __func__, __LINE__);
	/* release all protection */
	RamWrite16A_ois_dw9784_hi847(data, 0xFD00, 0x5252);
	mdelay(1);
	pr_info("%s:%d: [dw9784_hi847_code_pt_off] finish", __func__, __LINE__);
}

void ois_reset_ois_dw9784_hi847(void *data)
{
	pr_info("%s:%d: [dw9784_hi847_ois_reset_ois_dw9784_hi847] ois reset", __func__, __LINE__);

	RamWrite16A_ois_dw9784_hi847(data, 0xD002, 0x0001);  /* printfc reset */
	mdelay(4);
	RamWrite16A_ois_dw9784_hi847(data, 0xD001, 0x0001);  /* Active mode (DSP ON) */
	mdelay(25);  /* ST gyro - over wait 25ms, default Servo On */
	RamWrite16A_ois_dw9784_hi847(data, 0xEBF1, 0x56FA);  /* User protection release */
}

int calibration_save_ois_dw9784_hi847(void *data)
{
	pr_info("%s:%d: [dw9784_hi847_calibration_save_ois_dw9784_hi847] calibration save starting", __func__, __LINE__);
	RamWrite16A_ois_dw9784_hi847(data, 0x7012, 0x000A);  // Set store mode

	//When store is done, status changes to 0x6001
	if (dw9784_hi847_wait_check_register(data, 0x7010, 0x6001) == FUNC_PASS) {
		pr_info("[dw9784_hi847_set_cal_store] successful entry into store mode");
	}
	else {
		pr_err("[dw9784_hi847_set_cal_store] failed to enter store mode");
		return FUNC_FAIL;
	}

	dw9784_hi847_code_pt_off(data); /* code protection off */
	RamWrite16A_ois_dw9784_hi847(data, 0x700F, 0x5959);  // Set protect code
	mdelay(1);
	RamWrite16A_ois_dw9784_hi847(data, 0x7011, 0x0001);  // Execute store
	mdelay(40);

	//  When store is done, status changes to 0xA001
	if (dw9784_hi847_wait_check_register(data, 0x7010, 0x6001) == FUNC_PASS) {
		ois_reset_ois_dw9784_hi847(data);
		pr_info("%s:%d: [dw9784_hi847_set_cal_store] finish", __func__, __LINE__);
	}
	else {
		pr_err("%s:%d: [dw9784_hi847_set_cal_store] store function fail", __func__, __LINE__);
		return FUNC_FAIL;
	}
	return FUNC_PASS;
}

int ois_dw9784_hi847_debugfs_cal_s(void *data)
{
	msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *) data;
	uint32_t status;
	uint32_t xOffset, yOffset;
	int msg = EOK;

	pr_info("%s:%d:[dw9784_hi847_gyro_offset_calibrtion] gyro_offset_calibrtion starting", __func__, __LINE__);

	ptr->cal_info.cal_success = 0;

	RamWrite16A_ois_dw9784_hi847(data, 0x7012, 0x0006);  // gyro offset calibration
	mdelay(1);

	if (dw9784_hi847_wait_check_register(data, 0x7010, 0x6000) == FUNC_PASS) {
		RamWrite16A_ois_dw9784_hi847(data, 0x7011, 0x0001);		// gyro ofs calibration execute command
		mdelay(1);
	}
	else {
		pr_err("[dw9784_hi847_gyro_ofs_calibration] FUNC_FAIL");
		return FUNC_FAIL;
	}

	//  when calibration is done, Status changes to 0x6001
	if (dw9784_hi847_wait_check_register(data, 0x7010, 0x6001) == FUNC_PASS) {
		pr_info("[dw9784_hi847_gyro_ofs_calibration]calibration function finish");
	}
	else {
		pr_err("[dw9784_hi847_gyro_ofs_calibration]calibration function error");
		return FUNC_FAIL;
	}

	RamRead16A_ois_dw9784_hi847(data, 0x7180, &xOffset);  /* x gyro offset */
	RamRead16A_ois_dw9784_hi847(data, 0x7181, &yOffset);  /* y gyro offset */
	RamRead16A_ois_dw9784_hi847(data, 0x7195, &status);  /* gyro offset status */
	pr_info("%s:%d:[dw9784_gyro_offset_calibrtion] xOffset: 0x%04X, yOffset : 0x%04X Status = 0x%04X",
		__func__, __LINE__, xOffset, yOffset, status);

	/* Read Gyro offset cailbration result status */
	if ((status & 0x8000)== 0x8000) {
		if ((status & 0x1) == X_AXIS_GYRO_OFS_PASS) {
			msg = EOK;
			pr_info("[dw9784_hi847_gyro_ofs_calibration] x gyro ofs cal pass");
		}
		else
		{
			msg += X_AXIS_GYRO_OFS_FAIL;
			pr_err("[dw9784_hi847_gyro_ofs_calibration] x gyro ofs cal fail");
		}

		if ((status & 0x10) == X_AXIS_GYRO_OFS_OVER_MAX_LIMIT) {
			msg += X_AXIS_GYRO_OFS_OVER_MAX_LIMIT;
			pr_info("[dw9784_hi847_gyro_ofs_calibration] x gyro ofs over the max. limit");
		}

		if ((status & 0x2) == Y_AXIS_GYRO_OFS_PASS) {
			msg += EOK;
			pr_info("[dw9784_hi847_gyro_ofs_calibration] y gyro ofs cal pass");
		}else
		{
			msg += Y_AXIS_GYRO_OFS_FAIL;
			pr_err("[dw9784_hi847_gyro_ofs_calibration] y gyro ofs cal fail");
		}

		if ((status & 0x20) == Y_AXIS_GYRO_OFS_OVER_MAX_LIMIT) {
			msg += Y_AXIS_GYRO_OFS_OVER_MAX_LIMIT;
			pr_info("[dw9784_hi847_gyro_ofs_calibration] y gyro ofs over the max. limit");
		}

		if ((status & 0x800) == XY_AXIS_CHECK_GYRO_RAW_DATA) {
			msg += XY_AXIS_CHECK_GYRO_RAW_DATA;
			pr_info("[dw9784_hi847_gyro_ofs_calibration] check the x/y gyro raw data");
		}
		pr_info("[dw9784_hi847_gyro_ofs_calibration] x/y gyro ofs calibration finish");

		if(msg == EOK)
		{
			msg = calibration_save_ois_dw9784_hi847(data);
			ptr->cal_info.cal_success = 1;
		}
	}
	else {
		pr_err("[dw9784_hi847_gyro_ofs_calibration] x/y gyro ofs calibration done fail");
		return GYRO_OFS_CAL_DONE_FAIL;
	}

	pr_info("%s:%d:[dw9784_hi847_gyro_offset_calibrtion] msg : %d", __func__, __LINE__, msg);
	pr_info("%s:%d:[dw9784_hi847_gyro_offset_calibrtion] x_gyro_offset: 0x%04X, y_gyro_offset : 0x%04X", __func__, __LINE__, xOffset, yOffset);
	pr_info("%s:%d:[dw9784_hi847_gyro_offset_calibrtion] gyro_offset_calibrtion finished...Status = 0x%04X", __func__, __LINE__, status);

	return msg;
}

int ois_dw9784_hi847_debugfs_cal_g(void *data, u64 *val)
{
	msm_ois_debug_info_t *ptr = (msm_ois_debug_info_t *) data;

	ois_dw9784_hi847_debugfs_cal_s(data);

	if (ptr->cal_info.cal_success == 1) {
		*val |= 0x100000000;
	} else {
		*val = 0;
	}
	pr_info("%s:%d: cal_success = %d  0x%llx",
		__func__, __LINE__, ptr->cal_info.cal_success, *val);
	return 0;
}

