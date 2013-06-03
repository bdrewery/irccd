#include <cppunit/TextTestRunner.h>

#include "TestParser.h"

ParserTest::ParserTest()
{
}

ParserTest::~ParserTest()
{
}

void ParserTest::openCorrect()
{
	CPPUNIT_ASSERT(true);
}

int main()
{
	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(ParserTest::suite());
	runnerText.run();
}
