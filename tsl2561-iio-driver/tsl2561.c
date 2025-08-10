// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

#define REG_CONTROL		0x00
#define REG_TIMING		0x01
#define REG_THRESH_LOW_LOW	0x02
#define REG_THRESH_LOW_HIGH	0x03
#define REG_THRESH_HIGH_LOW	0x04
#define REG_THRESH_HIGH_HIGH	0x05
#define REG_INTERRUPT		0x06
#define REG_CRC			0x07
#define REG_ID			0x0A
#define REG_DATA_0_LOW		0x0C
#define REG_DATA_0_HIGH		0x0D
#define REG_DATA_1_LOW		0x0E
#define REG_DATA_1_HIGH		0x0F

#define CMD_BIT			0x80
#define CLEAR_BIT		0x40
#define WORD_BIT		0x20
#define BLOCK_BIT		0x10

#define INTEG_TIME_MASK		0x03 // 0b00000011

/**
 * For the package type of the TSL2561
 *
 * If lux compensation is added, this will be used to determine which formula
 * to use
 */
enum tsl2561_package_type {
	PACKAGE_CS,
	PACKAGE_T_FN_CL
};

// For the POWER bits of the control register
enum tsl2561_power {
	POWER_ON = 0x03,
	POWER_OFF = 0x00
};

/*
 * For the GAIN bit of the timing register
 *
 * Used for switching between low/high gain modes
 */
enum tsl2561_gain {
	GAIN_1x = 0x00,
	GAIN_16x = 0x01
};

/*
 * For the MANUAL bit of the timing register
 *
 * Used for manually beginning/stopping integration cycles.
 * The INTEG register must be set to INTEG_TIME_MANUAL (0x03) to use this.
 */
enum tsl2561_manual {
	MANUAL_STOP = 0x00,
	MANUAL_START = 0x01
};

/*
 * For the INTEG bits of the timing register
 *
 * Used to choose nominal integration time for each conversion.
 * INTEG_TIME_MANUAL makes it manual with the usage of the MANUAL bit
 */
enum tsl2561_integ_time {
	INTEG_TIME_13MS = 0x00, // Actual 13.7ms
	INTEG_TIME_101MS = 0x01,
	INTEG_TIME_402MS = 0x02,
	INTEG_TIME_MANUAL = 0x03
};

/*
 * For the INTR bit of the interrupt control register
 *
 * Used to choose the mode for the interrupt logic.
 * TODO: I don't understand this at all - research
 */
enum tsl2561_intr {
	INTR_DISABLED = 0x00,
	INTR_LEVEL_INTERRUPT = 0x01,
	INTR_SMBALERT = 0x02,
	INTR_TEST = 0x03
};

/*
 * For the PERSIST bit field of the interrupt control register
 *
 * Determines when interrupts occur.
 * PERSIST_EVERY interrupts on every cycle.
 * PERSIST_N interrupts if the value remains outside the threshold window for
 * N integration cycles.
 */
enum tsl2561_persist {
	PERSIST_EVERY = 0x00,
	PERSIST_1 = 0x01,
	PERSIST_2 = 0x02,
	PERSIST_3 = 0x03,
	PERSIST_4 = 0x04,
	PERSIST_5 = 0x05,
	PERSIST_6 = 0x06,
	PERSIST_7 = 0x07,
	PERSIST_8 = 0x08,
	PERSIST_9 = 0x09,
	PERSIST_10 = 0x0A,
	PERSIST_11 = 0x0B,
	PERSIST_12 = 0x0C,
	PERSIST_13 = 0x0D,
	PERSIST_14 = 0x0E,
	PERSIST_15 = 0x0F
};

// Used to separate IIO channels
enum tsl2561_channel_id {
	CHANNEL_DATA0,
	CHANNEL_DATA1
};

struct tsl2561_data {
	struct iio_dev *indio_dev;
	struct i2c_client *client;
	struct mutex lock; // protects data state
	enum tsl2561_gain gain;
	enum tsl2561_integ_time integ_time;
	u16 ch0, ch1;
};

// Takes the first (lsb) byte and converts into a word
static inline u16 get_u16_le(const u8 *reg)
{
	return (reg[1] << 8) | reg[0];
}

static ssize_t in_illuminance_gain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct tsl2561_data *data = iio_priv(indio_dev);
	int ret;

	mutex_lock(&data->lock);
	// gain is an enum and is either 0/1 but represents 1/16
	// so this is a simple math formula to get f(0) = 1 and f(1) = 16
	// probably better and easier than a switchcase for each albeit less
	// clear
	ret = sysfs_emit(buf, "%d\n", data->gain * 15 + 1);
	mutex_unlock(&data->lock);

	return ret;
}

static ssize_t in_illuminance_gain_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct tsl2561_data *data = iio_priv(indio_dev);
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	switch (val) {
	case 1:
		mutex_lock(&data->lock);
		data->gain = GAIN_1x;
		mutex_unlock(&data->lock);
		return count;
	case 16:
		mutex_lock(&data->lock);
		data->gain = GAIN_16x;
		mutex_unlock(&data->lock);
		return count;
	default:
		return -EINVAL;
	}
}

static DEVICE_ATTR_RW(in_illuminance_gain);

static struct attribute *tsl2561_attributes[] = {
	&dev_attr_in_illuminance_gain.attr,
	NULL
};

static const struct attribute_group tsl2561_attr_group = {
	.name = NULL,
	.attrs = tsl2561_attributes
};

// Reads word data from reg_address and puts it into channel
static int read_data_word(const struct i2c_client *client, u8 reg_address,
			  u16 *channel)
{
	int ret, word;
	u8 reg_data[2];

	// word read function only returns first byte because this
	// device isn't built for SMBus
	ret = i2c_smbus_read_i2c_block_data(client, reg_address, 2,
					    reg_data);
	if (ret)
		return ret;

	word = get_u16_le(reg_data);

	*channel = le16_to_cpu(word);

	return 0;
}

static int read_data_registers(struct tsl2561_data *data,
			       struct iio_chan_spec const *chan)
{
	int ret;

	switch (chan->address) {
	case CHANNEL_DATA0:
		mutex_lock(&data->lock);
		ret = read_data_word(data->client, REG_DATA_0_LOW, &data->ch0);
		mutex_unlock(&data->lock);

		if (ret)
			goto err;

		return data->ch0;
	case CHANNEL_DATA1:
		mutex_lock(&data->lock);
		ret = read_data_word(data->client, REG_DATA_1_LOW, &data->ch1);
		mutex_unlock(&data->lock);

		if (ret)
			goto err;

		return data->ch1;
	default:
		goto err;
	}

err:
	return -EINVAL;
}

// Manual not supported
static int integ_time_int_to_enum(int val, enum tsl2561_integ_time *out)
{
	switch (val) {
	case 13:
		*out = INTEG_TIME_13MS;
		break;
	case 101:
		*out = INTEG_TIME_101MS;
		break;
	case 402:
		*out = INTEG_TIME_402MS;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int integ_time_enum_to_int(enum tsl2561_integ_time val)
{
	switch (val) {
	case INTEG_TIME_13MS:
		return 13;
	case INTEG_TIME_101MS:
		return 101;
	case INTEG_TIME_402MS:
		return 402;
	default:
		return -EINVAL;
	}
}

static int tsl2561_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	struct tsl2561_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		*val = read_data_registers(data, chan);
		if (*val < 0)
			return -EINVAL;

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_INT_TIME:
		*val = integ_time_enum_to_int(data->integ_time);
		if (*val < 0)
			return -EINVAL;

		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static int tsl2561_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct tsl2561_data *data = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_INT_TIME:
		u8 timing_reg;

		mutex_lock(&data->lock);

		// While also converting to enum, also validates it
		ret = integ_time_int_to_enum(val, &data->integ_time);
		if (ret)
			goto unlock_err;

		ret = i2c_smbus_read_byte_data(data->client,
					       REG_TIMING);
		if (ret < 0)
			goto unlock_err;

		// Bit logic. Takes the current timing register and changes the
		// integ_time bits
		timing_reg = (ret & ~INTEG_TIME_MASK) |
			     (data->integ_time & INTEG_TIME_MASK);

		ret = i2c_smbus_write_byte_data(data->client, REG_TIMING,
						timing_reg);
		if (ret < 0)
			goto unlock_err;

		mutex_unlock(&data->lock);

		return 0;
	default:
		goto err;
	}

unlock_err:
	mutex_unlock(&data->lock);
err:
	return ret;
}

static const struct iio_info tsl2561_info = {
	.read_raw = tsl2561_read_raw,
	.write_raw = tsl2561_write_raw
};

static const struct iio_chan_spec tsl2561_channels[] = {
	{
		.type = IIO_LIGHT,
		.modified = true,
		.channel = 0,
		.channel2 = IIO_MOD_LIGHT_BOTH,
		.address = CHANNEL_DATA0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
	},
	{
		.type = IIO_LIGHT,
		.modified = true,
		.channel = 1,
		.channel2 = IIO_MOD_LIGHT_IR,
		.address = CHANNEL_DATA1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
	},
	{
		.type = IIO_LIGHT,
		.channel = 2,
		.info_mask_separate = BIT(IIO_CHAN_INFO_INT_TIME)
	}
};

static int tsl2561_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct tsl2561_data *data;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	data->client = client;
	data->indio_dev = indio_dev;
	mutex_init(&data->lock);

	i2c_set_clientdata(client, data);

	indio_dev->dev.parent = &client->dev;
	indio_dev->name = "tsl2561";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &tsl2561_info;
	indio_dev->channels = tsl2561_channels;
	indio_dev->num_channels = ARRAY_SIZE(tsl2561_channels);

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret)
		return ret;

	ret = sysfs_create_group(&indio_dev->dev.kobj, &tsl2561_attr_group);
	if (ret)
		return ret;

	return ret;
}

static void tsl2561_remove(struct i2c_client *client)
{
	struct tsl2561_data *data = i2c_get_clientdata(client);
	struct iio_dev *indio_dev = data->indio_dev;

	sysfs_remove_group(&indio_dev->dev.kobj, &tsl2561_attr_group);
}

static const struct i2c_device_id tsl2561_id[] = {
	{ "tsl2561", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, tsl2561_id);

static struct i2c_driver tsl2561_driver = {
	.driver = {
		.name = "tsl2561"
	},
	.probe = tsl2561_probe,
	.remove = tsl2561_remove,
	.id_table = tsl2561_id
};

// This is awesome! Replaces the module_init() and module_exit() boilerplate
module_i2c_driver(tsl2561_driver);

MODULE_AUTHOR("Michael Harris <michaelharriscode@gmail.com>");
MODULE_DESCRIPTION("TSL2561 driver using iio subsystem");
MODULE_LICENSE("GPL");
