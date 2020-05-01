// SPDX-License-Identifier: GPL-2.0-or-later
/*
    i2c-virt-master.c - A driver that forwards everything to slaves

    Copyright (C) 2020-2020 Michael Lipp <mnl@mnl.de>

*/

#define DEBUG 1
#define pr_fmt(fmt) "i2c-virt-master: " fmt

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>

/*
 * Handle single transfer. Return negative errno on error.
 */
static int i2c_xfer(struct i2c_adapter *adap, struct i2c_client *client,
		int idx, struct i2c_msg* msg) {
	u8 value;
	int i;

	dev_dbg(&adap->dev, "  %d: %s (flags %d) %d bytes %s 0x%02x\n",
				idx, msg->flags & I2C_M_RD ? "read" : "write",
				msg->flags, msg->len, msg->flags & I2C_M_RD ? "from" : "to", msg->addr);

	if (msg->flags & I2C_M_RD) {
		// Read data
		i2c_slave_event(client, I2C_SLAVE_READ_REQUESTED, &value);
		msg->buf[0] = value;
		for (i = 1; i < msg->len; i++) {
			i2c_slave_event(client, I2C_SLAVE_READ_PROCESSED, &value);
			msg->buf[i] = value;
		}
	} else {
		// Write data
		i2c_slave_event(client, I2C_SLAVE_WRITE_REQUESTED, &value);
		for (i = 0; i < msg->len; i++) {
			value = msg->buf[i];
			i2c_slave_event(client, I2C_SLAVE_WRITE_RECEIVED, &value);
		}
	}
	i2c_slave_event(client, I2C_SLAVE_STOP, &value);

	return 0;
}

/*
 * Handle transfers one by one. Return negative errno on error.
 */
static int virt_master_xfer(
		struct i2c_adapter *adap, struct i2c_msg* msgs, int num) {
	struct i2c_adapter* hub = i2c_get_adapdata(adap);
	int i;
	struct i2c_client *client = NULL;
	int found = FALSE;
	int ret;

	dev_dbg(&adap->dev, "I2C virt bus xfer %d messages:\n", num);

	// Process all messages
	for (i = 0; i < num; i++) {
		// No or different client?
		if (!client || client->addr != msgs[i].addr) {
			// Find registered client
			list_for_each_entry(client , &hub->userspace_clients, detected) {
				if (msgs[i].addr == client->addr) {
					found = TRUE;
					break;
				}
			}
			if (!found) {
				return -ENODEV;
			}
		}

		// Transfer current message
		ret = i2c_xfer(adap, client, i, &msgs[i]);
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
static u32 virt_master_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm virt_master_algorithm = {
	.functionality	= virt_master_func,
	.master_xfer = virt_master_xfer,
};

static struct i2c_adapter virt_master_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.algo		= &virt_master_algorithm,
	.name		= "I2C virt master driver",
};

static void virt_bus_free(void) {
//	kfree(stub_chips);
}

extern int virt_hub_init(struct i2c_adapter**);

static int __init virt_bus_init(void) {
	int ret;
	struct i2c_adapter* hub;

	pr_info("Initializing new I2C bus\n");

	ret = virt_hub_init(&hub);
	if (ret) {
		return ret;
	}

	/* Allocate memory for all chips at once */
//	stub_chips = kcalloc(1, sizeof(struct stub_chip),
//			     GFP_KERNEL);
//	if (!stub_chips) {
//		return -ENOMEM;
//	}
	i2c_set_adapdata(&virt_master_adapter, hub);

	ret = i2c_add_adapter(&virt_master_adapter);
	if (ret) {
		goto fail_free;
	}

	return 0;

 fail_free:
	virt_bus_free();
	return ret;
}

extern void virt_hub_exit(void);

static void __exit virt_bus_exit(void)
{
	pr_info("Deleting I2C bus\n");

	i2c_del_adapter(&virt_master_adapter);
	virt_bus_free();

	virt_hub_exit();
}

module_init(virt_bus_init); // @suppress("Unused function declaration")
module_exit(virt_bus_exit); // @suppress("Unused function declaration")

MODULE_AUTHOR("Michael N. Lipp <mnl@mnl.de>");
MODULE_DESCRIPTION("I2C virtual bus");
MODULE_LICENSE("GPL");

