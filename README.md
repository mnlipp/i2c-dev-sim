# I2C device simulation

During the coronavirus pandemic the need for simulating I2C devices arose.
Students couldn't physically attend their laboratories and therefore I came
up with the idea of making simulated (or virtual) I2C devices available.
This would allow the students to still exercise the usage of the API
to interact with the (simulated) devices. Of course, there are no physical 
effects like blinking LEDs or alike, but ... better than nothing.

The kernel already provides something close to this as I2C slave devices,
notably the 
[i2c-eeprom-slave](https://github.com/torvalds/linux/blob/master/drivers/i2c/i2c-slave-eeprom.c). 
However, only a few I2C master device drivers support slaves and those
require hardware to work.

The first approach was to write a single I2C driver that would both 
have the slaves attached and be used to interact with them (i.e.
write and read data as I2C transactions). As it turned out, having
a slave device at a given address prevents the usage of this address
for the simple I2C ioctl/read/write operations (at least for the
i2c-eeprom-slave which I wanted to use as a sample device).

The i2c-virt-bus driver therefore creates two I2C busses. The first
("`/dev/i2c-<n>`") is a dummy that manages the slave devices. It exposes
no other capabilities. Although there is no such thing with I2C busses,
you can think of it as a transparent hub to which all slave devices are 
attached.

The second ("`/dev/i2c-<n+1>`") I2C bus created when loading the module
uses an I2C master driver that can be used to access the slave devices
as if they were attached to its bus (see the setup-test target 
[here](test/Makefile)). You can think of this bus as your connection
to the hub.

## Future development

No. I'm making these sources available as is because they may be helpful
in an educational context. I have no plans to clean them up
(I'm not a kernel developer and I'm sure some things are horrible, e.g.
I have no idea how much locking is done by the i2c-core layer and
what I have missed here by not doing any locking at all) or maintain
them beyond my needs during the pandemic.

## Eclipse settings for kernel modules

Of course, real kernel developers use vi or emacs. But I have grown
accustomed to using eclipse, its refactoring capabilities and the
ease of exploring unknown environments by hitting `F3`. I have no doubt
that something like this can be achieved with emacs (which I did use
as my main tool during the 80s), but I didn't have time to explore this. 
I achieved to setup eclipse for kernel module development, following 
[these](https://wiki.eclipse.org/HowTo_use_the_CDT_to_navigate_Linux_kernel_source)
hints. The setup is tied to the OS/compiler version I used when developing
these drivers (while `make all` should work anywhere). So you'll have
to go through the setting if you want to use this in a different environment.