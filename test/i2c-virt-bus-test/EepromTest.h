/*
 * EepromTest.h
 *
 *  Created on: 24.04.2020
 *      Author: mnl
 */

#ifndef EEPROMTEST_H_
#define EEPROMTEST_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

class EepromTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(EepromTest);
	CPPUNIT_TEST(testSimpleRW);
	CPPUNIT_TEST(testMultipleRW);
	CPPUNIT_TEST_SUITE_END();

private:
	int eepromDev;

public:
	void setUp() {
		CPPUNIT_ASSERT_MESSAGE("I2C_BUS_NUM not set in environment",
				getenv("I2C_BUS_NUM") != nullptr);
		std::string i2cBus = "/dev/i2c-" + std::string(getenv("I2C_BUS_NUM"));
		eepromDev = open(i2cBus.c_str(), O_RDWR);
		std::ostringstream msg;
		msg << "Cannot open i2c bus " << i2cBus;
		CPPUNIT_ASSERT_MESSAGE(msg.str(), eepromDev >= 0);
		int res = ioctl(eepromDev, I2C_SLAVE, EEPROM_ADDR);
		CPPUNIT_ASSERT_MESSAGE(
				"Failed to acquire bus access and/or talk to slave", res >= 0);
	}

	void tearDown() {
		close(eepromDev);
	}

	void testSimpleRW() {
		char out[] = { 0, 0x42, 0x12, 0x34 };
		// write two bytes
		int res = write(eepromDev, out, 4);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 4);
		// write address for reading back
		res = write(eepromDev, out, 2);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 2);
		// read back
		char in[2];
		res = read(eepromDev, in, 2);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 2);
		CPPUNIT_ASSERT(in[0] == 0x12);
		CPPUNIT_ASSERT(in[1] == 0x34);
	}

	void testMultipleRW() {
	}

};

#endif /* EEPROMTEST_H_ */
