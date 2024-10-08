// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/firmware.h>

#include "cam_sensor_cmn_header.h"
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"

#include "zte_camera_ois_util.h"


#define INIT_WRITE_SIZE 16
#define RESET_WRITE_SIZE 3
#define DW9784_INIT_WRITE_SIZE 15

struct cam_sensor_i2c_reg_array g_ois_dw9784_init[DW9784_INIT_WRITE_SIZE] =
{
	/* step 1: MTP Erase and DSP Disable for firmware 0x8000 write */
	{0xd001, 0x0000, 0, 0xFFFF},

	/* release all protection */
	{0xFAFA, 0x98AC, 1, 0xFFFF},
	{0xF053, 0x70BD, 1, 0xFFFF},

	{0xDE01, 0x0000, 1, 0xFFFF},
	{0xFD00, 0x5252, 1, 0xFFFF},

	/* 4k Sector_0 */
	{0xde03, 0x0000, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_1 */
	{0xde03, 0x0008, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_2 */
	{0xde03, 0x0010, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_3 */
	{0xde03, 0x0018, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_4 */
	{0xde03, 0x0020, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF}
};


struct cam_sensor_i2c_reg_array g_ois_dw9781c_init[INIT_WRITE_SIZE] =
{
	/* step 1: MTP Erase and DSP Disable for firmware 0x8000 write */
	{0xd001, 0x0000, 0, 0xFFFF},

	/* release all protection */
	{0xFAFA, 0x98AC, 1, 0xFFFF},
	{0xF053, 0x70BD, 1, 0xFFFF},

	/* 12c level adjust */
	{0xd005, 0x0001, 0, 0xFFFF},
	{0xdd03, 0x0002, 0, 0xFFFF},
	{0xdd04, 0x0002, 0, 0xFFFF},

	/* 4k Sector_0 */
	{0xde03, 0x0000, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_1 */
	{0xde03, 0x0008, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_2 */
	{0xde03, 0x0010, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_3 */
	{0xde03, 0x0018, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF},
	/* 4k Sector_4 */
	{0xde03, 0x0020, 0, 0xFFFF},
	/* 4k Sector Erase */
	{0xde04, 0x0002, 10, 0xFFFF}
};

struct cam_sensor_i2c_reg_array g_ois_dw978x_reset[RESET_WRITE_SIZE] =
{
	/* printfc reset */
	{0xD002, 0x0001, 4, 0xFFFF},
	/* Active mode (DSP ON) */
	{0xD001, 0x0001, 25, 0xFFFF},
	/* User protection release */
	{0xEBF1, 0x56FA, 1, 0xFFFF}
};

#define EXTRA_FIRMWARE_SIZE 512
#define EXTRA_INIT_SIZE 3

struct cam_sensor_i2c_reg_array g_ois_dw9784_extra_init[EXTRA_INIT_SIZE] =
{
	{0xDE01, 0x1000, 1, 0xFFFF},
	{0xde03, 0x0000, 1, 0xFFFF},
	{0xde04, 0x0008, 10, 0xFFFF}
};

int cam_ois_update_time_dw9781c(struct i2c_settings_array *i2c_set)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0;
	uint32_t size = 0;
	uint32_t i = 0;
	uint64_t qtime_ns = 0;

	if (i2c_set == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	rc = cam_sensor_util_get_current_qtimer_ns(&qtime_ns);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
			"Failed to get current qtimer value: %d",
			rc);
		return rc;
	}

	list_for_each_entry(i2c_list,
		&(i2c_set->list_head), list) {
		if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_RANDOM) {
			size = i2c_list->i2c_settings.size;
			/* qtimer is 8 bytes so validate here*/
			/* dw9781c 4 words*/
			if (size * 2 < 8) {
				CAM_ERR(CAM_OIS, "Invalid write time settings size[%d] words", size);
				return -EINVAL;
			}

			i2c_list->i2c_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
			i2c_list->i2c_settings.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

			for (i = 0; i < size; i ++) {
				i2c_list->i2c_settings.reg_setting[i].reg_data =
					(qtime_ns & 0xFFFF);
				qtime_ns >>= 16;
			}
		}
	}

	return rc;
}


void swap_low_high_byte(uint32_t *data)
{
	uint32_t UpdateH, UpdateL = 0;

	UpdateH = (*data & 0xFF) << 8;
	UpdateL = (*data & 0xFF00) >> 8;
	*data = UpdateH | UpdateL;
}

/*****************************************************************/
/*these APIs are used for download firmware when open camera*/
/****************************************************************/
int cam_ois_dw978x_fw_download_init(
	struct cam_ois_ctrl_t *o_ctrl,
	uint32_t writesize,
	struct cam_sensor_i2c_reg_array *reg)
{
	uint16_t                           total_words = 0;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	void                              *vaddr = NULL;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	/*init for download firmware*/

	total_words = writesize;  //word
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

	i2c_reg_setting.size = total_words;
	i2c_reg_setting.delay = 0;
	fw_size = (sizeof(struct cam_sensor_i2c_reg_array) * total_words);
	vaddr = vmalloc(fw_size);
	if (!vaddr) {
		CAM_ERR(CAM_OIS,
			"Failed in allocating i2c_array: fw_size: %u", fw_size);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		vaddr);


	for (cnt = 0; cnt < i2c_reg_setting.size; cnt++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr = reg[cnt].reg_addr;
		i2c_reg_setting.reg_setting[cnt].reg_data = reg[cnt].reg_data;
		i2c_reg_setting.reg_setting[cnt].delay = reg[cnt].delay;
		i2c_reg_setting.reg_setting[cnt].data_mask = reg[cnt].data_mask;

		rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
			i2c_reg_setting.reg_setting[cnt].reg_addr,
			i2c_reg_setting.reg_setting[cnt].reg_data,
			i2c_reg_setting.addr_type,
			i2c_reg_setting.data_type);
		msleep(i2c_reg_setting.reg_setting[cnt].delay);

		if (rc < 0)
		{
			CAM_ERR(CAM_OIS, "%s i2c write failed %d", o_ctrl->ois_name, rc);
		}
	}

	vfree(vaddr);
	vaddr = NULL;
	fw_size = 0;

	return rc;
}

int cam_ois_dw978x_reset(struct cam_ois_ctrl_t *o_ctrl)
{
	uint16_t                           total_words = 0;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	void                              *vaddr = NULL;
	//uint32_t addr, value;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}


	/*reset ois*/
	total_words = RESET_WRITE_SIZE;   //word
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

	i2c_reg_setting.size = total_words;
	i2c_reg_setting.delay = 0;
	fw_size = (sizeof(struct cam_sensor_i2c_reg_array) * total_words);
	vaddr = vmalloc(fw_size);
	if (!vaddr) {
		CAM_ERR(CAM_OIS,
			"Failed in allocating i2c_array: fw_size: %u", fw_size);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		vaddr);


	for (cnt = 0; cnt < i2c_reg_setting.size; cnt++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr = g_ois_dw978x_reset[cnt].reg_addr;
		i2c_reg_setting.reg_setting[cnt].reg_data = g_ois_dw978x_reset[cnt].reg_data;
		i2c_reg_setting.reg_setting[cnt].delay = g_ois_dw978x_reset[cnt].delay;
		i2c_reg_setting.reg_setting[cnt].data_mask = g_ois_dw978x_reset[cnt].data_mask;

		rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
			i2c_reg_setting.reg_setting[cnt].reg_addr,
			i2c_reg_setting.reg_setting[cnt].reg_data,
			i2c_reg_setting.addr_type,
			i2c_reg_setting.data_type);
		msleep(i2c_reg_setting.reg_setting[cnt].delay);

		if (rc < 0)
		{
			CAM_ERR(CAM_OIS, "dw9781c i2c write failed", rc);
		}
	}

	vfree(vaddr);
	vaddr = NULL;
	fw_size = 0;

	return rc;
}

bool cam_ois_dw978x_need_update_fw(
	struct cam_ois_ctrl_t *o_ctrl,
	struct CameraOisParams *param)
{
	int32_t rc = 0;
	bool need_update_fw = true;
	uint32_t addr_ver, addr_date, ver, date;

	cam_ois_dw978x_reset(o_ctrl);

	addr_ver = 0x7001;
	ver = 0;
	rc = camera_io_dev_read(&(o_ctrl->io_master_info),
		addr_ver, &ver, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,false);
	CAM_INFO(CAM_OIS, "OIS read addr[0x%x] = 0x%x ", addr_ver, ver);

	addr_date = 0x7002;
	date = 0;
	rc = camera_io_dev_read(&(o_ctrl->io_master_info),
		addr_date, &date, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,false);
	CAM_INFO(CAM_OIS, "OIS read addr[0x%x] = 0x%x ", addr_date, date);

	if ((param->FWVersion == ver) && (param->FWDate == date))
		need_update_fw = false;

	return need_update_fw;
}

int cam_ois_dw978x_fw_download(
	struct cam_ois_ctrl_t *o_ctrl,
	struct CameraOisParams *param)
{
	uint16_t                           total_words = 0;
	uint16_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	//const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	//char                               name_coeff[32] = {0};
	struct device                     *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	void                              *vaddr = NULL;
	int32_t max_len = 128;
	int32_t current_len = max_len;
	uint32_t temp;
	uint32_t addr_offset = 0;

	CAM_INFO(CAM_OIS, "cam_ois_dw978x_fw_download %s Entry", o_ctrl->ois_name);

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if(!cam_ois_dw978x_need_update_fw(o_ctrl, param))
	{
		CAM_INFO(CAM_OIS, "%s firmware is latest version[0x%x, 0x%x] no need to update!",
			param->OISName, param->FWVersion, param->FWDate);
		return rc;
	}

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;
	//fw_name_coeff = name_coeff;

	if(!strncmp(o_ctrl->ois_name, "ois_dw9781", sizeof("ois_dw9781") - 1))
	{
		cam_ois_dw978x_fw_download_init(o_ctrl, INIT_WRITE_SIZE, g_ois_dw9781c_init);
	}
	else if(!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
	{
		cam_ois_dw978x_fw_download_init(o_ctrl, DW9784_INIT_WRITE_SIZE, g_ois_dw9784_init);
	}

	/* Load FW prog*/
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	CAM_DBG(CAM_OIS, "fw->size %d", fw->size);
	if(!strncmp(o_ctrl->ois_name, "ois_dw9781", sizeof("ois_dw9781") - 1))
	{
		total_words = fw->size;
	}
	else if(!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
	{
		total_words = fw->size - EXTRA_FIRMWARE_SIZE;
	}

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	total_words = total_words / 2;   //word

	i2c_reg_setting.size = total_words;
	i2c_reg_setting.delay = 0;
	fw_size = (sizeof(struct cam_sensor_i2c_reg_array) * total_words);
	vaddr = vmalloc(fw_size);
	if (!vaddr) {
		CAM_ERR(CAM_OIS,
			"Failed in allocating i2c_array: fw_size: %u", fw_size);
		release_firmware(fw);
		return -ENOMEM;
	}

	CAM_DBG(CAM_OIS, "FW prog size(word):%d. o_ctrl->opcode.prog[0x%x]", total_words, o_ctrl->opcode.prog);

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		vaddr);

	ptr = (uint16_t *)fw->data;
	do {
		current_len = (total_words > max_len) ? max_len : total_words;
		for (cnt = 0; cnt < current_len; cnt++, ptr++) {
			i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog + addr_offset;
			temp = *ptr;
			swap_low_high_byte(&temp);
				i2c_reg_setting.reg_setting[cnt].reg_data = temp;
			i2c_reg_setting.reg_setting[cnt].delay = 0;
			i2c_reg_setting.reg_setting[cnt].data_mask = 0;
		}
		i2c_reg_setting.size = cnt;
		addr_offset = addr_offset + cnt;
		rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
			&i2c_reg_setting, 1);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		}
		total_words -= cnt;
	} while (total_words > 0);

	if(!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
	{
		// extra init
		for (cnt = 0; cnt < EXTRA_INIT_SIZE; cnt++) {
			i2c_reg_setting.reg_setting[cnt].reg_addr = g_ois_dw9784_extra_init[cnt].reg_addr;
			i2c_reg_setting.reg_setting[cnt].reg_data = g_ois_dw9784_extra_init[cnt].reg_data;
			i2c_reg_setting.reg_setting[cnt].delay = g_ois_dw9784_extra_init[cnt].delay;
			i2c_reg_setting.reg_setting[cnt].data_mask = g_ois_dw9784_extra_init[cnt].data_mask;

			rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
				i2c_reg_setting.reg_setting[cnt].reg_addr,
				i2c_reg_setting.reg_setting[cnt].reg_data,
				i2c_reg_setting.addr_type,
				i2c_reg_setting.data_type);
			msleep(i2c_reg_setting.reg_setting[cnt].delay);

			if (rc < 0)
			{
				CAM_ERR(CAM_OIS, "dw9784 i2c write failed", rc);
			}
		}

		// extra firmware download
			total_words = EXTRA_FIRMWARE_SIZE;
			total_words = total_words / 2;   //word
			addr_offset = 0;

			do {
			current_len = (total_words > max_len) ? max_len : total_words;
			for (cnt = 0; cnt < current_len; cnt++, ptr++) {
				i2c_reg_setting.reg_setting[cnt].reg_addr =
				o_ctrl->opcode.prog + addr_offset;      //  0x8000
				temp = *ptr;
				swap_low_high_byte(&temp);
				i2c_reg_setting.reg_setting[cnt].reg_data = temp;
				i2c_reg_setting.reg_setting[cnt].delay = 0;
				i2c_reg_setting.reg_setting[cnt].data_mask = 0;
			}
			i2c_reg_setting.size = cnt;
			addr_offset = addr_offset + cnt;
			rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
				&i2c_reg_setting, 1);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
			}
			total_words -= cnt;
		} while (total_words > 0);
	}

	cam_ois_dw978x_reset(o_ctrl);

	CAM_INFO(CAM_OIS, "cam_ois_dw9781c_fw_download Exit");

	vfree(vaddr);
	vaddr = NULL;
	fw_size = 0;
	release_firmware(fw);
	return rc;
}

/*****************************************************************/
/*these APIs are used for download firmware when sensor probe*/
/****************************************************************/

uint32_t cam_ois_read_firmware_ver(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t rc = 0;
	int32_t cnt = 0;
	struct cam_sensor_cci_client local_cci_client;
	uint32_t addr_ver, addr_date, ver, date;
	uint32_t data = 0;
	uint16_t slave_addr = 0xE4;
	enum camera_sensor_i2c_type addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	enum camera_sensor_i2c_type data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

	o_ctrl->io_master_info.cci_client->sid = slave_addr >> 1;
	o_ctrl->io_master_info.cci_client->i2c_freq_mode = I2C_STANDARD_MODE;
	memcpy(&local_cci_client, o_ctrl->io_master_info.cci_client, sizeof(struct cam_sensor_cci_client));

	for (cnt = 0; cnt < RESET_WRITE_SIZE; cnt++) {
		rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
			g_ois_dw978x_reset[cnt].reg_addr,
			g_ois_dw978x_reset[cnt].reg_data,
			addr_type,
			data_type);
		msleep(g_ois_dw978x_reset[cnt].delay);

		if (rc < 0)
		{
			CAM_ERR(CAM_OIS, "cam_ois_read_firmware_ver %d", rc);
		}
	}

	addr_ver = 0x7001;
	ver = 0;
	rc = cam_cci_i2c_read(&local_cci_client, addr_ver, &ver, addr_type, data_type, TRUE);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "%s :%d: read vers id fail\n", __func__, __LINE__);
	}

	addr_date = 0x7002;
	date = 0;
	rc = cam_cci_i2c_read(&local_cci_client, addr_date, &date, addr_type, data_type, TRUE);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "%s :%d: read date  fail\n", __func__, __LINE__);
	}

	data = ver << 16 | date;
	CAM_ERR(CAM_OIS, "%s ver:0x%x, date:0x%x data:0x%x\n", __func__, ver, date, data);

	return data;
}

uint32_t cam_ois_dw978x_FirmWare_Download(
	struct cam_ois_ctrl_t *o_ctrl,
	uint32_t writesize,
	struct cam_sensor_i2c_reg_array *reg)
{
	uint32_t data = 0;

	uint16_t                           total_words = 0;
	uint16_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	//const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	//char                               name_coeff[32] = {0};
	struct device                     *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	void                              *vaddr = NULL;
	int32_t max_len = 128;
	int32_t current_len = max_len;
	uint32_t temp;
	uint32_t addr_offset = 0;
	uint16_t slave_addr = 0xE4;
	enum camera_sensor_i2c_type addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	enum camera_sensor_i2c_type data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

	o_ctrl->io_master_info.cci_client->sid = slave_addr >> 1;
	o_ctrl->io_master_info.cci_client->i2c_freq_mode = I2C_STANDARD_MODE;

	CAM_INFO(CAM_OIS, "cam_ois_dw9784_firmware_download %s Entry", o_ctrl->ois_name);

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;

	// init
	for (cnt = 0; cnt < writesize; cnt++) {
		rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
			reg[cnt].reg_addr,
			reg[cnt].reg_data,
			addr_type,
			data_type);
		msleep(reg[cnt].delay);

		if (rc < 0)
		{
			CAM_ERR(CAM_OIS, "%s i2c write failed %d", o_ctrl->ois_name, rc);
		}
	}

	/* Load FW prog*/
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	CAM_DBG(CAM_OIS, "fw->size %d", fw->size);
	if(!strncmp(o_ctrl->ois_name, "ois_dw9781", sizeof("ois_dw9781") - 1))
	{
		total_words = fw->size;
	}
	else if(!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
	{
		total_words = fw->size - EXTRA_FIRMWARE_SIZE;
	}

	i2c_reg_setting.addr_type = addr_type;
	i2c_reg_setting.data_type = data_type;
	total_words = total_words / 2;   //word

	i2c_reg_setting.size = total_words;
	i2c_reg_setting.delay = 0;
	fw_size = (sizeof(struct cam_sensor_i2c_reg_array) * total_words);
	vaddr = vmalloc(fw_size);
	if (!vaddr) {
		CAM_ERR(CAM_OIS,
			"Failed in allocating i2c_array: fw_size: %u", fw_size);
		release_firmware(fw);
		return -ENOMEM;
	}

	o_ctrl->opcode.prog = 0x8000;
	CAM_ERR(CAM_OIS, "FW prog size(word):%d. o_ctrl->opcode.prog[0x%x]", total_words, o_ctrl->opcode.prog);

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		vaddr);

	ptr = (uint16_t *)fw->data;
	do {
		current_len = (total_words > max_len) ? max_len : total_words;
		for (cnt = 0; cnt < current_len; cnt++, ptr++) {
			i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog + addr_offset;
			temp = *ptr;
			swap_low_high_byte(&temp);
			i2c_reg_setting.reg_setting[cnt].reg_data = temp;
			i2c_reg_setting.reg_setting[cnt].delay = 0;
			i2c_reg_setting.reg_setting[cnt].data_mask = 0;
		}
		i2c_reg_setting.size = cnt;
		addr_offset = addr_offset + cnt;
		rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
			&i2c_reg_setting, 1);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		}
		total_words -= cnt;
	} while (total_words > 0);

	if(!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
	{
		// extra init
		for (cnt = 0; cnt < EXTRA_INIT_SIZE; cnt++) {
			i2c_reg_setting.reg_setting[cnt].reg_addr = g_ois_dw9784_extra_init[cnt].reg_addr;
			i2c_reg_setting.reg_setting[cnt].reg_data = g_ois_dw9784_extra_init[cnt].reg_data;
			i2c_reg_setting.reg_setting[cnt].delay = g_ois_dw9784_extra_init[cnt].delay;
			i2c_reg_setting.reg_setting[cnt].data_mask = g_ois_dw9784_extra_init[cnt].data_mask;

			rc = zte_cam_cci_i2c_write(&(o_ctrl->io_master_info),
				g_ois_dw9784_extra_init[cnt].reg_addr,
				g_ois_dw9784_extra_init[cnt].reg_data,
				addr_type,
				data_type);
			msleep(g_ois_dw9784_extra_init[cnt].delay);

			if (rc < 0)
			{
				CAM_ERR(CAM_OIS, "dw9784 i2c write failed", rc);
			}
		}

		// extra firmware download
		total_words = EXTRA_FIRMWARE_SIZE;
		total_words = total_words / 2;   //word
		addr_offset = 0;

		do {
			current_len = (total_words > max_len) ? max_len : total_words;
			for (cnt = 0; cnt < current_len; cnt++, ptr++) {
				i2c_reg_setting.reg_setting[cnt].reg_addr =
				o_ctrl->opcode.prog + addr_offset;      //  0x8000
				temp = *ptr;
				swap_low_high_byte(&temp);
				i2c_reg_setting.reg_setting[cnt].reg_data = temp;
				i2c_reg_setting.reg_setting[cnt].delay = 0;
				i2c_reg_setting.reg_setting[cnt].data_mask = 0;
			}
			i2c_reg_setting.size = cnt;
			addr_offset = addr_offset + cnt;
			rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
				&i2c_reg_setting, 1);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
			}
			total_words -= cnt;
		} while (total_words > 0);
	}

	data = cam_ois_read_firmware_ver(o_ctrl);

	vfree(vaddr);
	vaddr = NULL;
	fw_size = 0;
	release_firmware(fw);

	CAM_ERR(CAM_OIS, "%s Exit\n", __func__);

	return data;
}

uint32_t cam_ois_dw978x_firmware_download(struct cam_ois_ctrl_t *o_ctrl)
{
	uint32_t data = 0;

	if(!strncmp(o_ctrl->ois_name, "ois_dw9781", sizeof("ois_dw9781") - 1))
		data = cam_ois_dw978x_FirmWare_Download(o_ctrl, INIT_WRITE_SIZE, g_ois_dw9781c_init);
	else if (!strncmp(o_ctrl->ois_name, "ois_dw9784", sizeof("ois_dw9784") - 1))
		data = cam_ois_dw978x_FirmWare_Download(o_ctrl, DW9784_INIT_WRITE_SIZE, g_ois_dw9784_init);
	else
		CAM_ERR(CAM_OIS, "%s unknown ois name %s Exit\n", __func__, o_ctrl->ois_name);

	return data;
}