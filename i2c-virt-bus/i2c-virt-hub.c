// SPDX-License-Identifier: GPL-2.0-or-later
/*
    i2c-virt-hub.c - A driver that is used to register slaves

    Copyright (C) 2020-2020 Michael Lipp <mnl@mnl.de>

*/

#define DEBUG 1
#define pr_fmt(fmt) "i2c-virt-hub: " fmt

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>

static int reg_slave(struct i2c_client *slave) {
	struct i2c_adapter *adap = slave->adapter;
	struct i2c_driver* i2c_driver = to_i2c_driver(adap->dev.driver);

	dev_dbg(&adap->dev, "Register slave %s\n", slave->name);


	// void *priv = i2c_get_adapdata(slave->adapter);

//	i2c_driver->clients

	return 0;
}

static int unreg_slave(struct i2c_client *slave) {
	void *priv = i2c_get_adapdata(slave->adapter);

	return 0;
}

/*
 * Handle transfers one by one. Return negative errno on error.
 */
static int virt_hub_xfer(struct i2c_adapter *adap, struct i2c_msg* msgs, int num) {
	return num;
}

/**
 * The driver should only manage slaves.
 */
static u32 virt_hub_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_SLAVE;
}

static const struct i2c_algorithm virt_hub_algorithm = {
	.functionality	= virt_hub_func,
	.master_xfer = virt_hub_xfer,
	.reg_slave	= reg_slave,
	.unreg_slave = unreg_slave,
};

static struct i2c_adapter virt_bus_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.algo		= &virt_hub_algorithm,
	.name		= "I2C virt hub driver",
};

static void virt_hub_free(void) {
//	kfree(stub_chips);
}

int __init virt_hub_init(struct i2c_adapter** adap) {
	int ret;

	pr_info("Initializing new I2C hub\n");

	/* Allocate memory for all chips at once */
//	stub_chips = kcalloc(1, sizeof(struct stub_chip),
//			     GFP_KERNEL);
//	if (!stub_chips) {
//		return -ENOMEM;
//	}

	ret = i2c_add_adapter(&virt_bus_adapter);
	if (ret) {
		goto fail_free;
	}
	*adap = &virt_bus_adapter;

	return 0;

 fail_free:
	virt_hub_free();
	return ret;
}

void __exit virt_hub_exit(void)
{
	pr_info("Deleting I2C hub\n");

	i2c_del_adapter(&virt_bus_adapter);
	virt_hub_free();
}

