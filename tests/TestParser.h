#ifndef _TEST_PARSER_H_
#define _TEST_PARSER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

class ParserTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ParserTest);
	CPPUNIT_TEST(openCorrect);
	CPPUNIT_TEST_SUITE_END();

public:
	ParserTest();
	~ParserTest();

	void openCorrect();
};

#endif // !_TEST_PARSER_H_
