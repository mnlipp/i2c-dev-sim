# DS1621 virtual slave device

This driver simulates DS1621 chip. For the general idea of I2C slave
devices see 
[the kernel documentation](https://www.kernel.org/doc/html/latest/i2c/slave-interface.html).

The device behaves as specified by the data sheet with the following
exceptions:

* There is no non-volatile storage. All DS1621 registers are set to 0
  when the salve is registered.
  
* There is no simulation of delays. The NVB flag in the AC register
  is never set.

The "sensor temperature", i.e. the value measured if measurement
is activated continuously or as one shot measurement, can be set
and read back using the "temperature" file in the driver's sysfs
directory.

The state of the Tout pin can be obtained by reading the "tout"
file in the driver's sysfs directory.