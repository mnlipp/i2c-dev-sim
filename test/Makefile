TOPTARGETS := all clean

SUBDIRS := i2c-virt-bus-test

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@/Debug $(MAKECMDGOALS)

setup-test:
	@-rmmod i2c-slave-ds1621
	@insmod ../i2c-slave-ds1621/i2c-slave-ds1621.ko
	@-rmmod i2c-virt-bus
	@i=0; while [ -r /dev/i2c-$$i ]; do i=`expr $$i + 1`; done; \
	insmod ../i2c-virt-bus/i2c-virt-bus.ko; \
	echo slave-24c02 0x1050 > /sys/bus/i2c/devices/i2c-$$i/new_device; \
	echo slave-24c32 0x1051 > /sys/bus/i2c/devices/i2c-$$i/new_device; \
	echo slave-ds1621 0x1048 > /sys/bus/i2c/devices/i2c-$$i/new_device; \
	chmod 666 /sys/devices/i2c-$$i/1-1048/temperature; \
	i=`expr $$i + 1`; \
	chmod 666 /dev/i2c-$$i; \
	echo "Created master /dev/i2c-$$i"

.PHONY: $(TOPTARGETS) $(SUBDIRS) setup-test
