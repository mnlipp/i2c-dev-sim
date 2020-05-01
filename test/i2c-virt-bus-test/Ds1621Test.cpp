/*
 * Ds1621Test.cpp
 *
 *  Created on: 01.05.2020
 *      Author: mnl
 */

#include "Ds1621Test.h"

void Ds1621Test::testRw(unsigned char data[]) {
	// write three bytes
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
	CPPUNIT_ASSERT(in[1] == data[2]);
}

void Ds1621Test::storeTemperature(float temperature) {
	std::ofstream temp(sysFsDir + "/temperature");
	temp << (int)(temperature * 1000);
	CPPUNIT_ASSERT_MESSAGE("Cannot store temperature", !temp.fail());
	temp.close();
}

float Ds1621Test::readTemperatureLowPrecision() {
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

float Ds1621Test::readTemperatureHighPrecision() {
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

bool Ds1621Test::cmpCents(float a, float b) {
	return (int)(a * 100 + 0.5) == (int)(b * 100 + 0.5);
}

void Ds1621Test::writeAc(uint8_t value) {
	unsigned char out[] = { accessAC, value };
	int res = write(ds1621Dev, out, 2);
	CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 2);
}

uint8_t Ds1621Test::Ds1621Test::readAc() {
	unsigned char out[] = { accessAC };
	int res = write(ds1621Dev, out, 1);
	CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
	// read
	unsigned char ac;
	res = read(ds1621Dev, &ac, 1);
	CPPUNIT_ASSERT_MESSAGE("Failed to read data", res == 1);
	return ac;
}

void Ds1621Test::startContinuousConversion() {
	// Start continuous conversion
	unsigned char out[] = { startConvertT };
	int res = write(ds1621Dev, out, 1);
	CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
}

void Ds1621Test::stopContinuousConversion() {
	// Stop continuous conversion
	unsigned char out[] = { stopConvertT };
	int res = write(ds1621Dev, out, 1);
	CPPUNIT_ASSERT_MESSAGE("Failed to write data", res == 1);
}
