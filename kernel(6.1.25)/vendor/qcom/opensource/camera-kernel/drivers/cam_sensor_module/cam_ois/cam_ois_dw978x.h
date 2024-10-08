#ifndef _CAM_OIS_DW978X_H_
#define _CAM_OIS_DW978X_H_

#include <linux/cma.h>
#include "cam_ois_dev.h"

#define OIS_NAME_LEN 32

int cam_ois_update_time_dw9781c(struct i2c_settings_array *i2c_set);

void swap_low_high_byte(uint32_t *data);

int cam_ois_dw978x_reset(struct cam_ois_ctrl_t *o_ctrl);

uint32_t cam_ois_read_firmware_ver(struct cam_ois_ctrl_t *o_ctrl);

uint32_t cam_ois_dw978x_firmware_download(struct cam_ois_ctrl_t *o_ctrl);

bool cam_ois_dw978x_need_update_fw(struct cam_ois_ctrl_t *o_ctrl, struct CameraOisParams *param);

int cam_ois_dw978x_fw_download(struct cam_ois_ctrl_t *o_ctrl, struct CameraOisParams *param);

int cam_ois_dw978x_fw_download_init(struct cam_ois_ctrl_t *o_ctrl, uint16_t writesize, struct cam_sensor_i2c_reg_array *reg);

uint32_t cam_ois_dw978x_FirmWare_Download(struct cam_ois_ctrl_t *o_ctrl, uint16_t writesize, struct cam_sensor_i2c_reg_array *reg);

static struct CameraOisParams g_CameraOisParams[] =
{
    {
        "imx800_cerro",
        "ois_dw9784_cerro_imx800",
        0x603,  //  bit16-31 ver, bit0-15 date
        0x0628
    },
    {
        "ov50e40_cerro",
        "ois_dw9784_cerro_ov50e40",
        0x102,
        0x0802
    },
    {
        "ov64b40_cerro",
        "ois_dw9781_cerro_ov64b40",
        0x104,
        0x0116
    },
    {
        "s5kgn5sp03_tiro",
        "ois_dw9784_tiro",
        0x101,
        0x0725
    }
};

#endif
/* _CAM_OIS_DW978X_H_ */
