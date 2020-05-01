#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestCaller.h>

#include "EepromTest.h"
#include "Ds1621Test.h"

int main(int argc, char **argv) {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(EepromTest::suite());
	runner.addTest(Ds1621Test::suite());
	runner.run();
	return 0;
}
