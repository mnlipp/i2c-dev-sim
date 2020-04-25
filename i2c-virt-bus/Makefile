obj-m := i2c-virt-bus.o
 
i2c-virt-bus-objs := i2c-virt-master.o i2c-virt-hub.o 

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

.PHONY: setup 