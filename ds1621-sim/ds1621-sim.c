// SPDX-License-Identifier: GPL-2.0-or-later
/*
    ds1621-simulator.c - I2C/SMBus chip emulator

    Copyright (C) 2020-2020 Michael Lipp <mnl@mnl.de>

*/

#define DEBUG 1
#define pr_fmt(fmt) "ds1621-sim: " fmt

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

static unsigned short chip_addr = 0;
module_param(chip_addr, ushort, S_IRUGO);
MODULE_PARM_DESC(chip_addr, "Chip address");

/*
 * Handle single transfer. Return negative errno on error.
 */
static int i2c_xfer(struct i2c_adapter *adap, int idx, struct i2c_msg* msg) {
	if (msg->addr != chip_addr) {
		return -ENODEV;
	}

	dev_dbg(&adap->dev, "  %d: %s (flags %d) %d bytes %s 0x%02x\n",
				idx, msg->flags & I2C_M_RD ? "read" : "write",
				msg->flags, msg->len, msg->flags & I2C_M_RD ? "from" : "to", msg->addr);

	if ((msg->flags & I2C_M_RD) == 0) {
		// Write data
	} else {
		// Read data
	}

	return 0;
}

/*
 * Handle transfers one by one. Return negative errno on error.
 */
static int ds1621_xfer(struct i2c_adapter *adap, struct i2c_msg* msgs, int num) {
	int i;

	dev_dbg(&adap->dev, "ds1621-sim xfer %d messages:\n", num);

	for (i = 0; i < num; i++) {
		int ret = i2c_xfer(adap, i, &msgs[i]);
		if (ret < 0) {
			return ret;
		}
	}
	return num;
}

/**
 * This implements a I2C controller, the emulation layer
 * converts SMBus commands into I2C transfers.
 */
static u32 ds1621_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm i2c_algorithm = {
	.functionality	= ds1621_func,
	.master_xfer = ds1621_xfer,
};

static struct i2c_adapter ds1621_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.algo		= &i2c_algorithm,
	.name		= "DS1621 simulator driver",
};

static void ds1621_sim_free(void) {
//	kfree(stub_chips);
}

static int __init ds1621_sim_init(void) {
	int ret;

	if (!chip_addr) {
		pr_err("Please specify a chip address\n");
		return -ENODEV;
	}

	if (chip_addr < 0b1001000 || chip_addr > 0b1001111) {
		pr_err("Invalid chip address 0x%02x\n", chip_addr);
		return -EINVAL;
	}

	pr_info("Virtual chip at 0x%02x\n", chip_addr);

	/* Allocate memory for all chips at once */
//	stub_chips = kcalloc(1, sizeof(struct stub_chip),
//			     GFP_KERNEL);
//	if (!stub_chips) {
//		return -ENOMEM;
//	}

	ret = i2c_add_adapter(&ds1621_adapter);
	if (ret) {
		goto fail_free;
	}

	return 0;

 fail_free:
	ds1621_sim_free();
	return ret;
}

static void __exit ds1621_sim_exit(void)
{
	i2c_del_adapter(&ds1621_adapter);
	ds1621_sim_free();
}

MODULE_AUTHOR("Michael N. Lipp <mnl@mnl.de>");
MODULE_DESCRIPTION("DS1621 Simulator");
MODULE_LICENSE("GPL");

module_init(ds1621_sim_init);
module_exit(ds1621_sim_exit);
