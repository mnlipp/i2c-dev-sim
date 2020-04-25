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
		eepromDev = open(STR(I2C_BUS), O_RDWR);
		CPPUNIT_ASSERT_MESSAGE("Cannot open i2c bus " STR(I2C_BUS),
				eepromDev >= 0);
		int res = ioctl(eepromDev, I2C_SLAVE, EEPROM_ADDR);
		CPPUNIT_ASSERT_MESSAGE(
				"Failed to acquire bus access and/or talk to slave", res >= 0);
	}

	void tearDown() {
		close(eepromDev);
	}

	void testSimpleRW() {
		char out[] = { 0, 0x42, 0x12, 0x34 };
		int res = write(eepromDev, out, 4);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res != 4);
	}

	void testMultipleRW() {
		int i = 0;
	}

};

#endif /* EEPROMTEST_H_ */
