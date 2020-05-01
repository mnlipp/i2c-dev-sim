// SPDX-License-Identifier: GPL-2.0-only
/*
 * I2C slave mode DS1621 simulator
 *
 * Copyright (C) 2020 by Michael N. Lipp
 */

#define DEBUG 1
#define pr_fmt(fmt) "i2c-sim-ds1621: " fmt

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

#define AC_THF (1 << 6)
#define AC_TLF (1 << 5)
#define AC_1SHOT (1 << 0)

struct ds1621_data {
	int stored_temperature;
	int measured_temperature;
	s16 TL;
	s16 TH;
	u8 AC;
	u8 tout;
	u8 read_counter;
	u8 read_slope;
	void* write_target;
	u16 buffer;
	u8 pending;
	u8 converting_continuously;
	struct device_attribute temperature_ac;
	// spinlock_t buffer_lock;
};

static int leftAlignedToInt(s16 value) {
	value >>= 7;
	if (value & 0x100) {
		value |= 0xfe00;
	}
	return value * 500;
}

static void setTemperature(struct ds1621_data *ds1621, int value) {
	ds1621->measured_temperature = value;
	if (value <= leftAlignedToInt(ds1621->TL)) {
		ds1621->AC |= AC_TLF;
	}
	if (value >= leftAlignedToInt(ds1621->TH)) {
		ds1621->AC |= AC_THF;
	}
}

static void handle_command(struct ds1621_data *ds1621, u8 cmd) {
	int fracDelta;
	ds1621->pending = 1;
	switch (cmd) {
	case 0xa1: // Access TH
		ds1621->pending = 2;
		ds1621->buffer = ds1621->TH;
		ds1621->write_target = &ds1621->TH;
		break;
	case 0xa2: // Access TL
		ds1621->pending = 2;
		ds1621->buffer = ds1621->TL;
		ds1621->write_target = &ds1621->TL;
		break;
	case 0xac: // Access Config
		ds1621->buffer = ds1621->AC | 0x8;
		ds1621->write_target = &ds1621->AC;
		break;
	case 0xa8: // Read Counter
		ds1621->buffer = ds1621->read_counter;
		break;
	case 0xa9: // Read Slope
		ds1621->buffer = ds1621->read_slope;
		break;
	case 0xaa: // Read Temperature
		ds1621->pending = 2;
		ds1621->buffer = (ds1621->measured_temperature <= 0 ? -1 : 1)
				* ((abs(ds1621->measured_temperature) + 250) / 500) << 7;
		fracDelta = ds1621->measured_temperature
				- (char)(ds1621->buffer >> 8) * 1000;
		ds1621->read_slope = 255;
		ds1621->read_counter = (750 - fracDelta) * ds1621->read_slope / 1000;
		break;
	case 0xee: // Start Convert T
		setTemperature(ds1621, ds1621->stored_temperature);
		if (!(ds1621->AC & AC_1SHOT)) {
			ds1621->converting_continuously = TRUE;
		}
		break;
	case 0x22: // Stop Convert T
		ds1621->converting_continuously = FALSE;
		break;
	default:
		ds1621->pending = 0;
		break;
	}
}

static int i2c_slave_ds1621_slave_cb(struct i2c_client *client,
				     enum i2c_slave_event event, u8 *val) {
	struct ds1621_data *ds1621 = i2c_get_clientdata(client);

	switch (event) {
	case I2C_SLAVE_WRITE_RECEIVED:
		if (ds1621->pending == 0) {
			dev_dbg(&client->dev, "Command %02x\n", *val);
			handle_command(ds1621, *val);
		} else {
			ds1621->buffer = (ds1621->buffer << 8) | *val;
			if (--ds1621->pending == 0) {
				dev_dbg(&client->dev, "  writing value %x\n", ds1621->buffer);
				if (ds1621->write_target == &ds1621->TH
						|| ds1621->write_target == &ds1621->TL) {
					*((u16*)(ds1621->write_target)) = ds1621->buffer;
				} else {
					*((u8*)(ds1621->write_target)) = ds1621->buffer;
				}
			}
		}
		break;

	case I2C_SLAVE_READ_REQUESTED:
		if (ds1621->pending > 0) {
			dev_dbg(&client->dev, "  returning value %x\n", ds1621->buffer);
		}
		/* fall through */
		/* no break */
	case I2C_SLAVE_READ_PROCESSED:
		/* The previous byte made it to the bus, get next one */
		if (ds1621->pending > 0) {
			*val = ds1621->buffer >> (--ds1621->pending * 8);
		}
		break;

	case I2C_SLAVE_WRITE_REQUESTED:
		ds1621->pending = 0;
		break;

	default:
		break;
	}

	return 0;
}

ssize_t temperature_show(struct device *dev, struct device_attribute *attr,
			char *buf) {
	struct ds1621_data *ds1621
		= (struct ds1621_data*)i2c_get_clientdata(to_i2c_client(dev));
    return scnprintf(buf, PAGE_SIZE, "%d\n", ds1621->stored_temperature);
}

ssize_t temperature_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count) {
	int res;
	struct ds1621_data *ds1621
		= (struct ds1621_data*)i2c_get_clientdata(to_i2c_client(dev));

	dev_dbg(dev, "Store temperature %s\n", buf);
	res = kstrtoint(buf, 10, &ds1621->stored_temperature);
	if (res < 0) {
		return res;
	}
	if (ds1621->converting_continuously) {
		setTemperature(ds1621, ds1621->stored_temperature);
	}
	return count;
}


static int i2c_slave_ds1621_probe(struct i2c_client *client,
		const struct i2c_device_id *id) {
	struct ds1621_data *ds1621;
	int ret;

	// Check address, must be in range
	if ((client->addr >> 3) != 9) {
		return -ENXIO;
	}

	// Allocate private (device) data
	ds1621 = devm_kzalloc(&client->dev, sizeof(struct ds1621_data), GFP_KERNEL);
	if (!ds1621) {
		return -ENOMEM;
	}

	// Initialize private data
	ds1621->stored_temperature = 21000;
	ds1621->measured_temperature = 0;
	ds1621->TL = 0;
	ds1621->TH = 0;
	ds1621->AC = 0;
	ds1621->converting_continuously = 0;
	ds1621->tout = 0;
	ds1621->pending = 0;
	// spin_lock_init(&ds1621->buffer_lock);
	i2c_set_clientdata(client, ds1621);

	// Prepare sysfs
	sysfs_attr_init(ds1621->temperature_ac.attr);
	ds1621->temperature_ac.attr.name = "temperature";
	ds1621->temperature_ac.attr.mode = S_IRUSR | S_IWUSR;
	ds1621->temperature_ac.show = temperature_show;
	ds1621->temperature_ac.store = temperature_store;

	ret = sysfs_create_file(&client->dev.kobj, &ds1621->temperature_ac.attr);
	if (ret)
		return ret;

	// Register as slave
	ret = i2c_slave_register(client, i2c_slave_ds1621_slave_cb);
	if (ret) {
		sysfs_remove_file(&client->dev.kobj, &ds1621->temperature_ac.attr);
		return ret;
	}

	return 0;
};

static int i2c_slave_ds1621_remove(struct i2c_client *client) {
	struct ds1621_data *ds1621 = i2c_get_clientdata(client);

	i2c_slave_unregister(client);
	sysfs_remove_file(&client->dev.kobj, &ds1621->temperature_ac.attr);

	return 0;
}

static const struct i2c_device_id i2c_slave_ds1621_id[] = {
	{ "slave-ds1621", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, i2c_slave_ds1621_id);

static struct i2c_driver i2c_slave_ds1621_driver = {
	.driver = {
		.name = "i2c-slave-ds1621",
	},
	.probe = i2c_slave_ds1621_probe,
	.remove = i2c_slave_ds1621_remove,
	.id_table = i2c_slave_ds1621_id,
};
module_i2c_driver(i2c_slave_ds1621_driver); // @suppress("Unused function declaration")

MODULE_AUTHOR("Michael N. Lipp <mnl@mnl.de>");
MODULE_DESCRIPTION("I2C slave mode DS1621 simulator");
MODULE_LICENSE("GPL v2");
