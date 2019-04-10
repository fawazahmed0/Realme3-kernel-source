/*
* Copyright(C)2014 MediaTek Inc.
* Modification based on code covered by the below mentioned copyright
* and/or permission notice(S).
*/

/* maghub.c - maghub compass driver
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <hwmsensor.h>
#include "maghub.h"
#include "mag.h"
#include <SCP_sensorHub.h>
#include "SCP_power_monitor.h"

#define MAGHUB_DEV_NAME         "mag_hub"
#define DRIVER_VERSION          "1.0.1"
#define MAGHUB_DEBUG		1

#if MAGHUB_DEBUG
#define MAGN_TAG                  "[Msensor] "
#define MAGN_FUN(f)               pr_debug(MAGN_TAG"%s\n", __func__)
#define MAGN_PR_ERR(fmt, args...)    pr_err(MAGN_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define MAGN_LOG(fmt, args...)    pr_debug(MAGN_TAG fmt, ##args)
#else
#define MAGN_TAG
#define MAGN_FUN(f)               do {} while (0)
#define MAGN_PR_ERR(fmt, args...)    do {} while (0)
#define MAGN_LOG(fmt, args...)    do {} while (0)
#endif


struct maghub_ipi_data *mag_ipi_data;
static struct mag_init_info maghub_init_info;

static int maghub_init_flag = -1;
static DEFINE_SPINLOCK(calibration_lock);

typedef enum {
	MAG_FUN_DEBUG = 0x01,
	MAG_MDATA_DEBUG = 0X02,
	MAG_ODATA_DEBUG = 0X04,
	MAG_CTR_DEBUG = 0X08,
	MAG_IPI_DEBUG = 0x10,
} MAG_TRC;
struct maghub_ipi_data {
	int		direction;
	int32_t dynamic_cali[MAGHUB_AXES_NUM];
	int32_t parameter_cali[6];
	atomic_t	trace;
	atomic_t	suspend;
	atomic_t	scp_init_done;
	atomic_t first_ready_after_boot;
	struct work_struct init_done_work;
	struct data_unit_t m_data_t;
	bool factory_enable;
	bool android_enable;
	struct mag_dev_info_t mag_dev_info;
};
static int maghub_m_setPowerMode(bool enable)
{
	int res = 0;

	MAGN_LOG("magnetic enable value = %d\n", enable);
	res = sensor_enable_to_hub(ID_MAGNETIC, enable);
	if (res < 0)
		MAGN_PR_ERR("maghub_m_setPowerMode is failed!!\n");

	return res;
}

static int maghub_GetMData(char *buf, int size)
{
	struct maghub_ipi_data *obj = mag_ipi_data;
	struct data_unit_t data;
	uint64_t time_stamp = 0;
	int mag_m[MAGHUB_AXES_NUM];
	int err = 0;
	unsigned int status = 0;

	if (atomic_read(&obj->suspend))
		return -3;

	if (buf == NULL)
		return -1;
	err = sensor_get_data_from_hub(ID_MAGNETIC, &data);
	if (err < 0) {
		MAGN_PR_ERR("sensor_get_data_from_hub fail!\n");
		return err;
	}

	time_stamp				= data.time_stamp;
	mag_m[MAGHUB_AXIS_X]	= data.magnetic_t.x;
	mag_m[MAGHUB_AXIS_Y]	= data.magnetic_t.y;
	mag_m[MAGHUB_AXIS_Z]	= data.magnetic_t.z;
	status					= data.magnetic_t.status;

	MAGN_LOG("recv ipi: timestamp: %lld, x: %d, y: %d, z: %d!\n", time_stamp,
		mag_m[MAGHUB_AXIS_X], mag_m[MAGHUB_AXIS_Y], mag_m[MAGHUB_AXIS_Z]);


	sprintf(buf, "%04x %04x %04x %04x", mag_m[MAGHUB_AXIS_X], mag_m[MAGHUB_AXIS_Y], mag_m[MAGHUB_AXIS_Z], status);

	if (atomic_read(&obj->trace) & MAG_MDATA_DEBUG)
		MAGN_LOG("RAW DATA: %s!\n", buf);


	return 0;
}

static int maghub_ReadChipInfo(char *buf, int bufsize)
{
	if ((!buf) || (bufsize <= MAGHUB_BUFSIZE - 1))
		return -1;

	sprintf(buf, "maghub Chip");
	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[MAGHUB_BUFSIZE];

	maghub_ReadChipInfo(strbuf, MAGHUB_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[MAGHUB_BUFSIZE];

	maghub_m_setPowerMode(true);
	msleep(20);
	maghub_GetMData(strbuf, MAGHUB_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;

	if (obj == NULL) {
		MAGN_PR_ERR("maghub_ipi_data is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct maghub_ipi_data *obj = mag_ipi_data;
	int trace = 0;
	int res = 0;

	if (obj == NULL) {
		MAGN_PR_ERR("maghub_ipi_data is null!!\n");
		return 0;
	}
	if (sscanf(buf, "0x%x", &trace) == 1) {
		atomic_set(&obj->trace, trace);
		res = sensor_set_cmd_to_hub(ID_MAGNETIC, CUST_ACTION_SET_TRACE, &trace);
		if (res < 0) {
			MAGN_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
				ID_MAGNETIC, CUST_ACTION_SET_TRACE);
			return 0;
		}
	} else {
		MAGN_PR_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *buf)
{
	ssize_t _tLength = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;

	MAGN_LOG("[%s] default direction: %d\n", __func__, obj->direction);

	_tLength = snprintf(buf, PAGE_SIZE, "default direction = %d\n", obj->direction);

	return _tLength;
}

static ssize_t store_chip_orientation(struct device_driver *ddri, const char *buf, size_t tCount)
{
	int _nDirection = 0, err = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;
	int res = 0;

	if (obj == NULL)
		return 0;
	err = kstrtoint(buf, 10, &_nDirection);
	if (err == 0) {
		obj->direction = _nDirection;
		res = sensor_set_cmd_to_hub(ID_MAGNETIC, CUST_ACTION_SET_DIRECTION, &_nDirection);
		if (res < 0) {
			MAGN_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
				ID_MAGNETIC, CUST_ACTION_SET_DIRECTION);
			return 0;
		}
	}

	MAGN_LOG("[%s] set direction: %d\n", __func__, _nDirection);

	return tCount;
}

static ssize_t show_regiter_map(struct device_driver *ddri, char *buf)
{

	ssize_t _tLength = 0;

	return _tLength;
}

#ifdef VENDOR_EDIT
/*Fei.Mo@EXP.BSP.Sensor, 2017/06/29, Add for msensor auto test */
static int selftest_result = 0;
static ssize_t show_test_id(struct device_driver *ddri, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", 1);
}
static ssize_t show_magnet_close(struct device_driver *ddri, char *buf)
{
	int result = 1;

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}
static ssize_t show_magnet_leave(struct device_driver *ddri, char *buf)
{
	ssize_t _tLength = 0;
	int res;
	res = sensor_set_cmd_to_hub(ID_MAGNETIC, CUST_ACTION_SELFTEST, &selftest_result);
	MAGN_LOG("selftest_result = %d\n",selftest_result);

	_tLength = snprintf(buf, PAGE_SIZE, "%d\n", selftest_result);
	return _tLength;
}

static ssize_t show_chip_selftest(struct device_driver *ddri, char *buf)
{
	ssize_t _tLength = 0;
	int res;
	res = sensor_set_cmd_to_hub(ID_MAGNETIC, CUST_ACTION_SELFTEST, &selftest_result);

	_tLength = snprintf(buf, PAGE_SIZE, "%d\n", selftest_result);
	return _tLength;
}
#endif /* VENDOR_EDIT */
static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(trace, S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);
static DRIVER_ATTR(regmap, S_IRUGO, show_regiter_map, NULL);
#ifdef VENDOR_EDIT
/*Fei.Mo@EXP.BSP.Sensor, 2017/06/29, Add for msensor auto test */
static DRIVER_ATTR(magnet_close, S_IRUGO, show_magnet_close, NULL);
static DRIVER_ATTR(magnet_leave, S_IRUGO, show_magnet_leave, NULL);
static DRIVER_ATTR(test_id, S_IRUGO, show_test_id, NULL);
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO, show_chip_selftest, NULL);
#endif /* VENDOR_EDIT */
static struct driver_attribute *maghub_attr_list[] = {
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_trace,
	&driver_attr_orientation,
	&driver_attr_regmap,
#ifdef VENDOR_EDIT
/*Fei.Mo@EXP.BSP.Sensor, 2017/06/29, Add for msensor auto test */
	&driver_attr_magnet_close,
	&driver_attr_magnet_leave,
	&driver_attr_test_id,
	&driver_attr_selftest,
#endif /* VENDOR_EDIT */

};
static int maghub_create_attr(struct device_driver *driver)
{
	int idx = 0, err = 0;
	int num = (int)(ARRAY_SIZE(maghub_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, maghub_attr_list[idx]);
		if (err) {
			MAGN_PR_ERR("driver_create_file (%s) = %d\n", maghub_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
static int maghub_delete_attr(struct device_driver *driver)
{
	int idx = 0, err = 0;
	int num = (int)(ARRAY_SIZE(maghub_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, maghub_attr_list[idx]);

	return err;
}

static void scp_init_work_done(struct work_struct *work)
{
	int32_t cfg_data[9] = {0};
	struct maghub_ipi_data *obj = mag_ipi_data;
	int err = 0;
	struct mag_libinfo_t mag_libinfo;

	if (atomic_read(&obj->scp_init_done) == 0) {
		MAGN_PR_ERR("scp is not ready to send cmd\n");
		return;
	}
	if (atomic_xchg(&obj->first_ready_after_boot, 1) == 0) {

		err = sensor_set_cmd_to_hub(ID_MAGNETIC,
			CUST_ACTION_GET_SENSOR_INFO, &obj->mag_dev_info);
		if (err < 0) {
			pr_err_ratelimited("set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
				ID_MAGNETIC, CUST_ACTION_GET_SENSOR_INFO);
		}
		strlcpy(mag_libinfo.libname,
			obj->mag_dev_info.libname,
			sizeof(mag_libinfo.libname));
		mag_libinfo.layout = obj->mag_dev_info.layout;
		mag_libinfo.deviceid = obj->mag_dev_info.deviceid;

		err = mag_info_record(&mag_libinfo);
		return;
	}

	spin_lock(&calibration_lock);
	cfg_data[0] = obj->dynamic_cali[0];
	cfg_data[1] = obj->dynamic_cali[1];
	cfg_data[2] = obj->dynamic_cali[2];

	cfg_data[3] = obj->parameter_cali[0];
	cfg_data[4] = obj->parameter_cali[1];
	cfg_data[5] = obj->parameter_cali[2];
	cfg_data[6] = obj->parameter_cali[3];
	cfg_data[7] = obj->parameter_cali[4];
	cfg_data[8] = obj->parameter_cali[5];
	spin_unlock(&calibration_lock);
	err = sensor_cfg_to_hub(ID_MAGNETIC, (uint8_t *)cfg_data, sizeof(cfg_data));
			if (err < 0)
				MAGN_PR_ERR("sensor_cfg_to_hub fail\n");
}
static int mag_recv_data(struct data_unit_t *event, void *reserved)
{
	int err = 0;
	struct mag_data data;
	struct maghub_ipi_data *obj = mag_ipi_data;

		data.x = event->magnetic_t.x;
		data.y = event->magnetic_t.y;
		data.z = event->magnetic_t.z;
		data.status = event->magnetic_t.status;
		data.timestamp = (int64_t)event->time_stamp;
		data.reserved[0] = event->reserve[0];

	if (event->flush_action == DATA_ACTION)
		err = mag_data_report(&data);
	else if (event->flush_action == FLUSH_ACTION)
		err = mag_flush_report();
	else if (event->flush_action == BIAS_ACTION) {
		data.x = event->magnetic_t.x_bias;
		data.y = event->magnetic_t.y_bias;
		data.z = event->magnetic_t.z_bias;
		err = mag_bias_report(&data);
		spin_lock(&calibration_lock);
		obj->dynamic_cali[MAGHUB_AXIS_X] = event->magnetic_t.x_bias;
		obj->dynamic_cali[MAGHUB_AXIS_Y] = event->magnetic_t.y_bias;
		obj->dynamic_cali[MAGHUB_AXIS_Z] = event->magnetic_t.z_bias;
		spin_unlock(&calibration_lock);
	} else if (event->flush_action == CALI_ACTION) {
		err = mag_cali_report(event->data);
		spin_lock(&calibration_lock);
		obj->parameter_cali[0] = event->data[0];
		obj->parameter_cali[1] = event->data[1];
		obj->parameter_cali[2] = event->data[2];
		obj->parameter_cali[3] = event->data[3];
		obj->parameter_cali[4] = event->data[4];
		obj->parameter_cali[5] = event->data[5];
		spin_unlock(&calibration_lock);
	}
	return err;
}
static int maghub_enable(int en)
{
	int res = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;

	if (en == true)
		WRITE_ONCE(obj->android_enable, true);
	else
		WRITE_ONCE(obj->android_enable, false);

	res = maghub_m_setPowerMode(en);
	if (res)
		MAGN_PR_ERR("maghub_m_setPowerMode is failed!!\n");
	return res;
}

static int maghub_set_delay(u64 ns)
{
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	int delayms = 0, err = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;

	delayms = (int)ns / 1000 / 1000;
	err = sensor_set_delay_to_hub(ID_MAGNETIC, delayms);
	if (err < 0) {
		MAGN_PR_ERR("maghub_m_set_delay fail!\n");
		return err;
	}

	MAGN_LOG("maghub_m_set_delay (%d)\n", delayms);
	return err;
#elif defined CONFIG_NANOHUB
	return 0;
#else
	return 0;
#endif
}
static int maghub_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	maghub_set_delay(samplingPeriodNs);
#endif
	return sensor_batch_to_hub(ID_MAGNETIC, flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int maghub_flush(void)
{
	return sensor_flush_to_hub(ID_MAGNETIC);
}

static int maghub_set_cali(uint8_t *data, uint8_t count)
{
	int32_t *buf = (int32_t *)data;
	struct maghub_ipi_data *obj = mag_ipi_data;

	spin_lock(&calibration_lock);
	obj->dynamic_cali[0] = buf[0];
	obj->dynamic_cali[1] = buf[1];
	obj->dynamic_cali[2] = buf[2];

	obj->parameter_cali[0] = buf[3];
	obj->parameter_cali[1] = buf[4];
	obj->parameter_cali[2] = buf[5];
	obj->parameter_cali[3] = buf[6];
	obj->parameter_cali[4] = buf[7];
	obj->parameter_cali[5] = buf[8];
	spin_unlock(&calibration_lock);

	return sensor_cfg_to_hub(ID_MAGNETIC, data, count);
}

static int maghub_open_report_data(int open)
{
	return 0;
}

static int maghub_get_data(int *x, int *y, int *z, int *status)
{
	char buff[MAGHUB_BUFSIZE];

	maghub_GetMData(buff, MAGHUB_BUFSIZE);

	if (sscanf(buff, "%x %x %x %x", x, y, z, status) != 4)
		MAGN_PR_ERR("maghub_m_get_data sscanf fail!!\n");
	return 0;
}
static int scp_ready_event(uint8_t event, void *ptr)
{
	struct maghub_ipi_data *obj = mag_ipi_data;

	switch (event) {
	case SENSOR_POWER_UP:
	    atomic_set(&obj->scp_init_done, 1);
		schedule_work(&obj->init_done_work);
	    break;
	case SENSOR_POWER_DOWN:
	    atomic_set(&obj->scp_init_done, 0);
	    break;
	}
	return 0;
}
static struct scp_power_monitor scp_ready_notifier = {
	.name = "mag",
	.notifier_call = scp_ready_event,
};
static int maghub_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err = 0;
	struct maghub_ipi_data *obj = mag_ipi_data;

	if (enabledisable == true)
		WRITE_ONCE(obj->factory_enable, true);
	else
		WRITE_ONCE(obj->factory_enable, false);

	if (enabledisable == 1) {
		err = sensor_set_delay_to_hub(ID_MAGNETIC, sample_periods_ms);
		if (err < 0) {
			MAGN_PR_ERR("sensor_set_delay_to_hub fail!\r\n");
			return -1;
		}
	}
	err = sensor_enable_to_hub(ID_MAGNETIC, enabledisable == true ? 1 : 0);
	if (err < 0) {
		MAGN_PR_ERR("sensor_enable_to_hub fail!\r\n");
		return -1;
	}
	return 0;
}
static int maghub_factory_get_data(int32_t data[3], int *status)
{
	int err = 0;

	/* get raw data */
	err = maghub_get_data(&data[0], &data[1], &data[2], status);
	data[0] = data[0] / CONVERT_M_DIV;
	data[1] = data[1] / CONVERT_M_DIV;
	data[2] = data[2] / CONVERT_M_DIV;

	return err;
}
static int maghub_factory_get_raw_data(int32_t data[3])
{
	MAGN_LOG("do not support maghub_factory_get_raw_data!\n");
	return 0;
}
static int maghub_factory_enable_calibration(void)
{
	return 0;
}
static int maghub_factory_clear_cali(void)
{
	return 0;
}
static int maghub_factory_set_cali(int32_t data[3])
{
	return 0;
}
static int maghub_factory_get_cali(int32_t data[3])
{
	return 0;
}
static int maghub_factory_do_self_test(void)
{
	return 0;
}

static struct mag_factory_fops maghub_factory_fops = {
	.enable_sensor = maghub_factory_enable_sensor,
	.get_data = maghub_factory_get_data,
	.get_raw_data = maghub_factory_get_raw_data,
	.enable_calibration = maghub_factory_enable_calibration,
	.clear_cali = maghub_factory_clear_cali,
	.set_cali = maghub_factory_set_cali,
	.get_cali = maghub_factory_get_cali,
	.do_self_test = maghub_factory_do_self_test,
};

static struct mag_factory_public maghub_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &maghub_factory_fops,
};
static int maghub_probe(struct platform_device *pdev)
{
	int err = 0;
	struct maghub_ipi_data *data;
	struct mag_control_path ctl = { 0 };
	struct mag_data_path mag_data = { 0 };

	MAGN_FUN();
	data = kzalloc(sizeof(struct maghub_ipi_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	mag_ipi_data = data;
	atomic_set(&data->trace, 0);
	WRITE_ONCE(data->factory_enable, false);
	WRITE_ONCE(data->android_enable, false);

	platform_set_drvdata(pdev, data);

	err = scp_sensorHub_data_registration(ID_MAGNETIC, mag_recv_data);
	if (err < 0) {
		MAGN_PR_ERR("scp_sensorHub_data_registration failed\n");
		goto exit_kfree;
	}
	err = mag_factory_device_register(&maghub_factory_device);
	if (err) {
		MAGN_PR_ERR("mag_factory_device_register register failed\n");
		goto exit_kfree;
	}
	/* Register sysfs attribute */
	err = maghub_create_attr(&(maghub_init_info.platform_diver_addr->driver));
	if (err) {
		MAGN_PR_ERR("create attribute err = %d\n", err);
		goto exit_misc_device_register_failed;
	}
	ctl.is_use_common_factory = false;
	ctl.enable = maghub_enable;
	ctl.set_delay = maghub_set_delay;
	ctl.batch = maghub_batch;
	ctl.flush = maghub_flush;
	ctl.set_cali = maghub_set_cali;
	ctl.open_report_data = maghub_open_report_data;
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	ctl.is_report_input_direct = true;
	ctl.is_support_batch = false;
#elif defined CONFIG_NANOHUB
	ctl.is_report_input_direct = true;
	ctl.is_support_batch = true;
#else
#endif

	err = mag_register_control_path(&ctl);
	if (err) {
		MAGN_PR_ERR("register mag control path err\n");
		goto create_attr_failed;
	}

	mag_data.div = CONVERT_M_DIV;
	mag_data.get_data = maghub_get_data;

	err = mag_register_data_path(&mag_data);
	if (err) {
		MAGN_PR_ERR("register data control path err\n");
		goto create_attr_failed;
	}
	MAGN_LOG("%s: OK\n", __func__);
	maghub_init_flag = 1;
	/*Mointor scp ready notify,
	 *need monitor at the end of probe for two function:
	 * 1.read mag_dev_info from sensorhub,
	 * write to mag context
	 * 2.set cali to sensorhub
	 */
	INIT_WORK(&data->init_done_work, scp_init_work_done);
	scp_power_monitor_register(&scp_ready_notifier);
	return 0;

create_attr_failed:
	maghub_delete_attr(&(maghub_init_info.platform_diver_addr->driver));
exit_misc_device_register_failed:
	mag_factory_device_deregister(&maghub_factory_device);
exit_kfree:
	kfree(data);
exit:
	MAGN_PR_ERR("%s: err = %d\n", __func__, err);
	maghub_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int maghub_remove(struct platform_device *pdev)
{
	int err = 0;

	err = maghub_delete_attr(&(maghub_init_info.platform_diver_addr->driver));
	if (err)
		MAGN_PR_ERR("maghub_delete_attr fail: %d\n", err);

	kfree(platform_get_drvdata(pdev));
	mag_factory_device_deregister(&maghub_factory_device);
	return 0;
}

static int maghub_suspend(struct platform_device *pdev, pm_message_t msg)
{
	return 0;
}

static int maghub_resume(struct platform_device *pdev)
{
	return 0;
}
static void maghub_shutdown(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		MAGN_PR_ERR("%s::i=%d\n", __func__, i);
		maghub_enable(0);
	}
	maghub_enable(0);
}

static struct platform_device maghub_device = {
	.name = MAGHUB_DEV_NAME,
	.id = -1,
};

static struct platform_driver maghub_driver = {
	.driver = {
		   .name = MAGHUB_DEV_NAME,
	},
	.probe = maghub_probe,
	.remove = maghub_remove,
	.suspend = maghub_suspend,
	.resume = maghub_resume,
	.shutdown = maghub_shutdown,
};

static int maghub_local_remove(void)
{
	platform_driver_unregister(&maghub_driver);
	return 0;
}

static int maghub_local_init(void)
{
	if (platform_driver_register(&maghub_driver)) {
		MAGN_PR_ERR("add_driver error\n");
		return -1;
	}
	if (-1 == maghub_init_flag)
		return -1;
	return 0;
}
static struct mag_init_info maghub_init_info = {
	.name = "maghub",
	.init = maghub_local_init,
	.uninit = maghub_local_remove,
};

static int __init maghub_init(void)
{
	if (platform_device_register(&maghub_device)) {
		MAGN_PR_ERR("platform device error\n");
		return -1;
	}
	mag_driver_add(&maghub_init_info);
	return 0;
}

static void __exit maghub_exit(void)
{
	MAGN_FUN();
}

module_init(maghub_init);
module_exit(maghub_exit);

MODULE_AUTHOR("hongxu.zhao@mediatek.com");
MODULE_DESCRIPTION("MAGHUB compass driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
