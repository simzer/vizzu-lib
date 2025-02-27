#include "funcstring.h"

#include "smartstring.h"

using namespace Text;

FuncString::FuncString(std::string code, bool throwOnError)
{
	Text::SmartString::trim(code);

	if (code.empty()) return;

	auto parts = SmartString::split(code, '(');

	if (parts.size() != 2 || parts[1].empty()
	    || parts[1].back() != ')') {
		if (throwOnError)
			throw std::logic_error("invalid function format");
		else
			return;
	}

	Text::SmartString::rightTrim(parts[1],
	    [](int c) -> int
	    {
		    return c == ')';
	    });

	name = parts[0];
	params = SmartString::split(parts[1], ',');

	SmartString::trim(name);

	for (auto &param : params) { SmartString::trim(param); }
}
