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



#ifndef ZTE_CAMERA_OIS_dw9784_hi847_UTIL_H

#define ZTE_CAMERA_OIS_dw9784_hi847_UTIL_H

#define     LOOP_A                          200
#define     LOOP_B                          LOOP_A-1
#define     FUNC_PASS                       0
#define     FUNC_FAIL                       -1

/* gyro offset calibration */
#define     GYRO_OFS_CAL_DONE_FAIL          0xFF
#define     X_AXIS_GYRO_OFS_PASS            0x1
#define     X_AXIS_GYRO_OFS_FAIL            0x1
#define     Y_AXIS_GYRO_OFS_PASS            0x2
#define     Y_AXIS_GYRO_OFS_FAIL            0x2
#define     X_AXIS_GYRO_OFS_OVER_MAX_LIMIT  0x10
#define     Y_AXIS_GYRO_OFS_OVER_MAX_LIMIT  0x20
#define     XY_AXIS_CHECK_GYRO_RAW_DATA     0x800
#define     EOK                             0

int ois_dw9784_hi847_debugfs_cal_g(void *data, u64 *val);

#endif



