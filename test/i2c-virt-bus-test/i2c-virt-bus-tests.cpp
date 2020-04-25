#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestCaller.h>

#include "EepromTest.h"

int main(int argc, char **argv) {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(EepromTest::suite());
	runner.run();
	return 0;
}
