#ifndef _IRCCD_TEST_JS_UTIL_H_
#define _IRCCD_TEST_JS_UTIL_H_

#include <iostream>
#include <string>
#include <sstream>

#include <gtest/gtest.h>

#include <Js.h>

namespace irccd {

class LibtestUtil : public testing::Test {
protected:
	JsDuktape m_ctx;

public:
	LibtestUtil(const std::string &ret, const std::string &modname)
	{
		std::ostringstream oss;
		std::string str;

		oss << ret << " = require(\"" << modname << "\");";
		str = oss.str();

		duk_push_c_function(m_ctx, [] (duk_context *ctx) -> duk_ret_t {
			std::cerr << "failure from script: " << duk_require_string(ctx, 0) << std::endl;

			return 0;
		}, 1);
		duk_put_global_string(m_ctx, "fail");

		duk_eval_string_noresult(m_ctx, str.c_str());
		duk_push_string(m_ctx, BINARY);
		duk_put_global_string(m_ctx, "BINARY");
	}

	void checkSymbol(const std::string &name, const std::string &type)
	{
		std::ostringstream oss;
		std::string cmd;

		oss << "typeof (" << name << ") === \"" << type << "\"";
		cmd = oss.str();

		if (duk_peval_string(m_ctx, cmd.c_str())) {
			std::string msg = duk_safe_to_string(m_ctx, -1);

			duk_pop(m_ctx);

			FAIL() << "Error in command: " << msg;
		} else {
			bool result = duk_to_boolean(m_ctx, -1);
			duk_pop(m_ctx);

			if (!result) {
				FAIL() << "Missing symbol: " << name;
			}
		}
	}

	void execute(const std::string &cmd)
	{
		if (duk_peval_string(m_ctx, cmd.c_str())) {
			std::string msg = duk_safe_to_string(m_ctx, -1);

			duk_pop(m_ctx);

			FAIL() << "Error in command: " << msg;
		}
	}
};

} // !irccd

#endif // !_IRCCD_TEST_JS_UTIL_H_
