// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>

#define CLASS_NAME "bmp280"
#define REG_TEMP	0xFA
#define REG_PRESS	0xF7
#define REG_CONFIG	0xF5
#define REG_CTRL_MEAS	0xF4
#define REG_STATUS	0xF3
#define REG_RESET	0xE0
#define REG_ID		0xD0
#define REG_CALIB_START	0x88

#define REG_TEMP_LEN	3
#define REG_PRESS_LEN	3
#define REG_CALIB_LEN	24
#define RESET_VALUE	0xB6

enum BMP280_MODE {
	SLEEP = 0x00,
	FORCED = 0x01,
	NORMAL = 0x02
};

enum BMP280_OSRS {
	OSRS_SKIPPED = 0x00,
	OSRS_x1 = 0x01,
	OSRS_x2 = 0x02,
	OSRS_x4 = 0x03,
	OSRS_x8 = 0x04,
	OSRS_x16 = 0x05
};

enum BMP280_T_SB {
	T_SB_0_5 = 0x00,
	T_SB_62_5 = 0x01,
	T_SB_125 = 0x02,
	T_SB_250 = 0x03,
	T_SB_500 = 0x04,
	T_SB_1000 = 0x05,
	T_SB_2000 = 0x06,
	T_SB_4000 = 0x07
};

static struct class *bmp280_class;
static int device_count;

/**
 * dig (digital compensation parameters) data
 * Described in 3.11.2 in the datasheet.
 */
struct bmp280_calib_data {
	u16 t1;
	s16 t2, t3;
	u16 p1;
	s16 p2, p3, p4, p5, p6, p7, p8, p9;
};

/**
 * For the config register
 */
union bmp280_config {
	struct {
		u8 spi3w_en: 1;
		u8: 1; // Reserved
		u8 filter: 3;
		u8 t_sb: 3;
	} bits;
	u8 byte;
};

/**
 * For the ctrl_meas register
 */
union bmp280_ctrl_meas {
	struct {
		u8 mode: 2;
		u8 osrs_p: 3;
		u8 osrs_t: 3;
	} bits;
	u8 byte;
};

/**
 * For the status register
 */
union bmp280_status {
	struct {
		u8 im_update: 1;
		u8: 2; // Reserved
		u8 measuring: 1;
		u8: 4; // Reserved
	} bits;
	u8 byte;
};

/**
 * Data struct for bmp280_data.
 * Used with devm_kzalloc, dev_set_drvdata, and dev_get_drvdata for having
 * device data that is accessible in both probe and remove
 *
 * Temperature in millidegrees celsius
 * Pressure in pascals
 */
struct bmp280_data {
	struct i2c_client *client;
	struct device *device;
	struct bmp280_calib_data calib;
	struct hrtimer poll_timer;
	struct work_struct poll_work;
	union bmp280_config config;
	union bmp280_ctrl_meas ctrl_meas;
	union bmp280_status status;
	dev_t devt;
	atomic_t poll_interval, temperature, pressure;
	s32 t_fine;
};

/**
 * Compensate temperature function from 3.11.3 of the datasheet
 *
 * Returns temperature in millidegree celsius
 */
static s32 compensate_temperature(s32 adc_t, struct bmp280_data *data)
{
	struct bmp280_calib_data *calib = &data->calib;
	s32 var1, var2, T;

	var1 = ((((adc_t >> 3) - ((s32)calib->t1 << 1)))
		* ((s32)calib->t2)) >> 11;

	var2 = (((((adc_t >> 4) - ((s32)calib->t1))
		* ((adc_t >> 4) - ((s32)calib->t1))) >> 12)
		* ((s32)calib->t3)) >> 14;

	data->t_fine = var1 + var2;

	T = (data->t_fine * 5 + 128) >> 8;

	return T;
}

/**
 * Compensate pressure function from 3.11.3 of the datasheet
 *
 * Returns pressure in pascals, in a "Q24.8 format"
 */
static u32 compensate_pressure(s32 adc_p, struct bmp280_data *data)
{
	struct bmp280_calib_data *calib = &data->calib;
	s64 var1, var2, p;

	var1 = ((s64)data->t_fine) - 128000;
	var2 = var1 * var1 * (s64)calib->p6;
	var2 = var2 + ((var1 * (s64)calib->p5) << 17);
	var2 = var2 + (((s64)calib->p4) << 35);
	var1 = ((var1 * var1 * (s64)calib->p3) >> 8)
		+ ((var1 * (s64)calib->p2) << 12);
	var1 = (((((s64)1) << 47) + var1)) * ((s64)calib->p1) >> 33;

	if (var1 == 0)
		return 0; // Avoid division by zero

	p = 1048576 - adc_p;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((s64)calib->p9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((s64)calib->p8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((s64)calib->p7) << 4);

	return (u32)p;
}

// Takes the first (lsb) byte and converts into a word
static inline u16 get_u16_le(const u8 *reg)
{
	return (reg[1] << 8) | reg[0];
}

// Takes the first (lsb) byte and converts into a word
static inline s16 get_s16_le(const u8 *reg)
{
	return (s16)((reg[1] << 8) | reg[0]);
}

// Takes the register and combines the next three bytes into a u32 (uses 20bits)
// used for getting adc_t or adc_p variable from register values
static inline u32 reg_to_adc(const u8 *reg)
{
	return (reg[0] << 12) | (reg[1] << 4) | (reg[2] >> 4);
}

// Reads data from registers into data variables
static void full_read(struct bmp280_data *data)
{
	u8 temp_buf[REG_TEMP_LEN];
	u8 press_buf[REG_TEMP_LEN];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(data->client, REG_TEMP,
					    REG_TEMP_LEN, temp_buf);
	if (ret < 0) {
		pr_err("bmp280: i2c read temp failure\n");
		return;
	}

	atomic_set(&data->temperature,
		   compensate_temperature(reg_to_adc(temp_buf), data));

	ret = i2c_smbus_read_i2c_block_data(data->client, REG_PRESS,
					    REG_PRESS_LEN, press_buf);
	if (ret < 0) {
		pr_err("bmp280: i2c read pres failure\n");
		return;
	}

	atomic_set(&data->pressure,
		   compensate_pressure(reg_to_adc(temp_buf), data));

	ret = i2c_smbus_read_byte_data(data->client, REG_STATUS);
	if (ret < 0) {
		pr_err("bmp280: i2c read status failure\n");
		return;
	}

	data->status.byte = ret;
}

// Writes data from data variables into registers
static void full_write(struct bmp280_data *data)
{
	int ret;

	ret = i2c_smbus_write_byte_data(data->client, REG_CONFIG,
					data->config.byte);
	if (ret < 0) {
		pr_err("bmp280: i2c write to config failure\n");
		return;
	}

	ret = i2c_smbus_write_byte_data(data->client, REG_CTRL_MEAS,
					data->ctrl_meas.byte);
	if (ret < 0) {
		pr_err("bmp280: i2c write to ctrl_meas failure\n");
		return;
	}
}

static ssize_t temperature_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct bmp280_data *data = dev_get_drvdata(dev);

	if (!data)
		return -ENODEV;

	return sprintf(buf, "%d\n", atomic_read(&data->temperature));
}

static DEVICE_ATTR_RO(temperature);

static ssize_t pressure_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct bmp280_data *data = dev_get_drvdata(dev);

	if (!data)
		return -ENODEV;

	return sprintf(buf, "%d\n", atomic_read(&data->pressure));
}

static DEVICE_ATTR_RO(pressure);

static ssize_t poll_interval_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct bmp280_data *data = dev_get_drvdata(dev);

	if (!data)
		return -ENODEV;

	return sprintf(buf, "%d\n", atomic_read(&data->poll_interval));
}

static ssize_t poll_interval_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct bmp280_data *data = dev_get_drvdata(dev);

	if (!data)
		return -ENODEV;

	int ret, new_poll_interval;

	ret = kstrtoint(buf, 10, &new_poll_interval);
	if (ret < 0)
		return ret;

	// Ensures number isn't negative and is between 50ms and 10s.
	// 50ms is bottom boundary due to maximum measure time for ultra high
	// resolution sampling - whichis 43.2ms.
	if (new_poll_interval < 0) {
		return -EINVAL;
	} else if (new_poll_interval > 10000) {
		pr_err("bmp280: tried to assign poll_interval greater than 10s\n");
		return -EINVAL;
	} else if (new_poll_interval < 50) {
		pr_err("bmp280: tried to assign poll_interval less than 50ms\n");
		return -EINVAL;
	}

	atomic_set(&data->poll_interval, new_poll_interval);

	// run once and restart timer with new interval
	schedule_work(&data->poll_work);
	hrtimer_cancel(&data->poll_timer);
	hrtimer_start(&data->poll_timer, ms_to_ktime(new_poll_interval),
		      HRTIMER_MODE_REL);

	return count;
}

static DEVICE_ATTR_RW(poll_interval);

static int create_dev_files(struct device *dev)
{
	int ret = 0;

	ret = device_create_file(dev, &dev_attr_temperature);
	if (ret)
		return ret;

	ret = device_create_file(dev, &dev_attr_pressure);
	if (ret)
		goto del_temperature;

	ret = device_create_file(dev, &dev_attr_poll_interval);
	if (ret)
		goto del_pressure;

	return 0;

del_pressure:
	device_remove_file(dev, &dev_attr_pressure);
del_temperature:
	device_remove_file(dev, &dev_attr_temperature);
	return ret;
}

static void remove_dev_files(struct device *dev)
{
	device_remove_file(dev, &dev_attr_poll_interval);
	device_remove_file(dev, &dev_attr_pressure);
	device_remove_file(dev, &dev_attr_temperature);
}

static void poll_work(struct work_struct *workqueue)
{
	struct bmp280_data *data = container_of(workqueue, struct bmp280_data,
						poll_work);

	// Used to write data to force a measurement
	full_write(data);
	full_read(data);
}

static enum hrtimer_restart bmp280_poll_timer_callback(struct hrtimer *timer)
{
	struct bmp280_data *data = container_of(timer, struct bmp280_data,
						poll_timer);

	schedule_work(&data->poll_work);

	hrtimer_forward_now(timer,
			    ms_to_ktime(atomic_read(&data->poll_interval)));
	return HRTIMER_RESTART;
}

static int bmp280_poll_timer_init(struct hrtimer *timer, int poll_interval)
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = bmp280_poll_timer_callback;
	hrtimer_start(timer, ms_to_ktime(poll_interval),
		      HRTIMER_MODE_REL);

	return 0;
}

static void bmp280_poll_timer_exit(struct hrtimer *timer)
{
	hrtimer_cancel(timer);
}

// TODO: reorganize structure of this
static int bmp280_probe(struct i2c_client *client)
{
	struct bmp280_data *data;
	u8 calib_buf[REG_CALIB_LEN];
	int ret;

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	atomic_set(&data->poll_interval, 1000);
	data->client = client;

	// Creates a dev_t num and a device
	alloc_chrdev_region(&data->devt, 0, 1, CLASS_NAME);
	data->device = device_create(bmp280_class, NULL, data->devt, NULL,
				     "bmp280%d", device_count++);

	dev_set_drvdata(&client->dev, data); // For accessing in .remove
	dev_set_drvdata(data->device, data); // For accessing in sysfs attr

	create_dev_files(data->device);

	ret = i2c_smbus_read_i2c_block_data(client, REG_CALIB_START,
					    REG_CALIB_LEN, calib_buf);
	if (ret < 0) {
		pr_err("bmp280: i2c calibration value read failure\n");
		return ret;
	}

	// Parsing registers into correct calibration variables
	data->calib.t1 = get_u16_le(&calib_buf[0]);
	data->calib.t2 = get_s16_le(&calib_buf[2]);
	data->calib.t3 = get_s16_le(&calib_buf[4]);
	data->calib.p1 = get_u16_le(&calib_buf[6]);
	data->calib.p2 = get_s16_le(&calib_buf[8]);
	data->calib.p3 = get_s16_le(&calib_buf[10]);
	data->calib.p4 = get_s16_le(&calib_buf[12]);
	data->calib.p5 = get_s16_le(&calib_buf[14]);
	data->calib.p6 = get_s16_le(&calib_buf[16]);
	data->calib.p7 = get_s16_le(&calib_buf[18]);
	data->calib.p8 = get_s16_le(&calib_buf[20]);
	data->calib.p9 = get_s16_le(&calib_buf[22]);

	full_read(data);

	data->ctrl_meas.bits.osrs_t = OSRS_x16;
	data->ctrl_meas.bits.osrs_p = OSRS_x16;
	data->ctrl_meas.bits.mode = FORCED;
	data->config.bits.t_sb = T_SB_0_5;

	full_write(data);

	INIT_WORK(&data->poll_work, poll_work);

	bmp280_poll_timer_init(&data->poll_timer,
			       atomic_read(&data->poll_interval));

	pr_info("bmp280: device probed");

	return 0;
}

static void bmp280_remove(struct i2c_client *client)
{
	// no error handling since return void???
	struct bmp280_data *data = dev_get_drvdata(&client->dev);

	bmp280_poll_timer_exit(&data->poll_timer);
	remove_dev_files(data->device);
	device_destroy(bmp280_class, data->devt);
	unregister_chrdev_region(data->devt, 1);
	flush_work(&data->poll_work);
	cancel_work_sync(&data->poll_work);

	pr_info("bmp280: device removed");
}

static const struct i2c_device_id bmp280_id_table[] = {
	{ "bmp280", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bmp280_id_table);

static const struct of_device_id bmp280_dt_ids[] = {
	{ .compatible = "bosch, bmp280" },
	{ }
};
MODULE_DEVICE_TABLE(of, bmp280_dt_ids);

static struct i2c_driver bmp280_driver = {
	.driver = {
		.name = "bmp280",
		.of_match_table = bmp280_dt_ids,
	},
	.probe = bmp280_probe,
	.remove = bmp280_remove,
	.id_table = bmp280_id_table,
};

static int __init bmp280_init(void)
{
	bmp280_class = class_create(CLASS_NAME);
	if (IS_ERR(bmp280_class)) {
		pr_err("bmp280: failed to create class\n");
		return PTR_ERR(bmp280_class);
	}

	pr_info("bmp280: driver initialized\n");

	return i2c_add_driver(&bmp280_driver);
}

static void __exit bmp280_exit(void)
{
	i2c_del_driver(&bmp280_driver);
	class_destroy(bmp280_class);
	pr_info("bmp280: driver exited\n");
}

module_init(bmp280_init);
module_exit(bmp280_exit);

MODULE_AUTHOR("Michael Harris <michaelharriscode@gmail.com>");
MODULE_DESCRIPTION("Simple BMP280 driver");
MODULE_LICENSE("GPL");
