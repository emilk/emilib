#include "string_interning.hpp"

#include <cstdlib>
#include <mutex>

namespace emilib {

StringInterner::~StringInterner()
{
	for (auto& p : _map) {
		free(p.second);
	}
}

const char* StringInterner::intern(const std::string& str)
{
	auto& ptr = _map[str];
	if (!ptr) {
		ptr = strdup(str.c_str());
	}
	return ptr;
}

static std::mutex     s_mutex;
static StringInterner s_interner;

const char* intern_string(const std::string& str)
{
	std::lock_guard<std::mutex> lock(s_mutex);
	return s_interner.intern(str);
}

} // namespace emilib
