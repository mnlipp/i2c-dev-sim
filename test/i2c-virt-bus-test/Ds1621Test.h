/*
 * Ds1621Test.h
 *
 *  Created on: 01.05.2020
 *      Author: mnl
 */

#ifndef DS1621TEST_H_
#define DS1621TEST_H_

#include <string>
#include <cstdint>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

class Ds1621Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(Ds1621Test);
	CPPUNIT_TEST(testRwTh);
	CPPUNIT_TEST(testRwTl);
	CPPUNIT_TEST(testSingle);
	CPPUNIT_TEST(testLowPrecision);
	CPPUNIT_TEST(testHighPrecision);
	CPPUNIT_TEST_SUITE_END();

private:
	int ds1621Dev;
	const unsigned char accessTh = 0xa1;
	const unsigned char accessTl = 0xa2;
	const unsigned char readTemperature = 0xaa;
	const unsigned char readCounter = 0xa8;
	const unsigned char readSlope = 0xa9;
	const unsigned char accessAC = 0xac;
	const unsigned char startConvertT = 0xee;
	const unsigned char stopConvertT = 0x22;
	std::string sysFsDir;

	void testRw(unsigned char data[]) {
		// write two bytes
		int res = write(ds1621Dev, data, 3);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 3);
		// write address for reading back
		res = write(ds1621Dev, data, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		// read back
		char in[2];
		res = read(ds1621Dev, in, 2);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 2);
		CPPUNIT_ASSERT(in[0] == data[1]);
		CPPUNIT_ASSERT(in[1] ==data[2]);
	}

	void storeTemperature(float temperature) {
		std::ofstream temp(sysFsDir + "/temperature");
		temp << (int)(temperature * 1000);
		CPPUNIT_ASSERT_MESSAGE("Cannot store temperature", !temp.fail());
		temp.close();
	}

	float readTemperatureLowPrecision() {
		unsigned char out[] = { readTemperature };
		int res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		// read
		unsigned char in[2];
		res = read(ds1621Dev, in, 2);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 2);
		int16_t val = in[0] << 8 | in[1];
		val >>= 7;
		if (val & 0x100) {
			val |= 0xfe00;
		}
		return val / 2.0;
	}

	float readTemperatureHighPrecision() {
		unsigned char out[] = { readTemperature };
		// Get temperature
		int res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		unsigned char temp[2];
		res = read(ds1621Dev, temp, 2);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 2);

		// Get counter
		out[0] = readCounter;
		res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		unsigned char count_remain;
		res = read(ds1621Dev, &count_remain, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 1);

		// Get slope
		out[0] = readSlope;
		res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		unsigned char count_per_c;
		res = read(ds1621Dev, &count_per_c, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 1);

		float temperature = (char)temp[0] - 0.25
				+ (count_per_c - count_remain) / (float)count_per_c;
		return temperature;
	}

	bool cmpCents(float a, float b) {
		return (int)(a * 100 + 0.5) == (int)(b * 100 + 0.5);
	}

public:
	void setUp() {
		CPPUNIT_ASSERT_MESSAGE("I2C_BUS_NUM not set in environment",
				getenv("I2C_BUS_NUM") != nullptr);
		int busNum = stoi(std::string(getenv("I2C_BUS_NUM")));
		std::string i2cBus = "/dev/i2c-" + std::to_string(busNum);
		ds1621Dev = open(i2cBus.c_str(), O_RDWR);
		std::ostringstream msg;
		msg << "Cannot open i2c bus " << i2cBus;
		CPPUNIT_ASSERT_MESSAGE(msg.str(), ds1621Dev >= 0);
		int res = ioctl(ds1621Dev, I2C_SLAVE, DS1621_ADDR);
		CPPUNIT_ASSERT_MESSAGE(
				"Failed to acquire bus access and/or talk to slave", res >= 0);
		sysFsDir = "/sys/devices/i2c-"
				+ std::to_string(busNum - 1) + "/1-1048";
	}

	void tearDown() {
		close(ds1621Dev);
	}

	void testRwTh() {
		unsigned char out[] = { accessTh, 0x12, 0x34 };
		testRw(out);
	}

	void testRwTl() {
		unsigned char out[] = { accessTl, 0x56, 0x78 };
		testRw(out);
	}

	void testSingle() {
		storeTemperature(42.42);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() != 42);
		unsigned char out[] = { startConvertT };
		int res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 42.5);
	}

	void testLowPrecision() {
		// Start continuous conversion
		unsigned char out[] = { accessAC, 0x0, startConvertT };
		int res = write(ds1621Dev, out, 3);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 3);

		for (float givenTemp = -10; givenTemp <= 10; givenTemp += 0.5) {
			storeTemperature(givenTemp);
			CPPUNIT_ASSERT(readTemperatureLowPrecision() == givenTemp);
		}

		// Test rounding
		storeTemperature(-0.2);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 0);
		storeTemperature(-0.3);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == -0.5);
		storeTemperature(-0.7);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == -0.5);
		storeTemperature(-0.8);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == -1);
		storeTemperature(0.2);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 0);
		storeTemperature(0.3);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 0.5);
		storeTemperature(0.7);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 0.5);
		storeTemperature(0.8);
		CPPUNIT_ASSERT(readTemperatureLowPrecision() == 1);

		// Stop continuous conversion
		out[0] = { stopConvertT };
		res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
	}

	void testHighPrecision() {
		// Start continuous conversion
		unsigned char out[] = { accessAC, 0x0, startConvertT };
		int res = write(ds1621Dev, out, 3);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 3);

		for (float givenTemp = -3; givenTemp <= 3; givenTemp += 0.01) {
			storeTemperature(givenTemp);
			std::stringstream msg;
			msg << "Testing value " << givenTemp;
			CPPUNIT_ASSERT_MESSAGE(msg.str(),
					cmpCents(readTemperatureHighPrecision(), givenTemp));
		}

		// Stop continuous conversion
		out[0] = { stopConvertT };
		res = write(ds1621Dev, out, 1);
		CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
	}
};


#endif /* DS1621TEST_H_ */
