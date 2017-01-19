// By Emil Ernerfeldt 2015-2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

namespace emilib {

/// Based on Andrei Alexandrescu's talk "Systematic Error Handling in C++" at C++ and Beyond 2012.
/// Slides: https://onedrive.live.com/view.aspx?resid=F1B8FF18A2AEC5C5!1158&app=WordPdf&authkey=!APo6bfP5sJ8EmH4
template<class Fun>
class ScopeGuard
{
public:
	explicit ScopeGuard(Fun f) : _fun(std::move(f)), _active(true) {}

	~ScopeGuard() noexcept(false)
	{
		if (_active) {
			_fun();
		}
	}

	void dismiss() { _active = false; }
	ScopeGuard() = delete;
	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;

	ScopeGuard(ScopeGuard && rhs) : _fun(std::move(rhs._fun)), _active(rhs._active)
	{
		rhs.dismiss();
	}

private:
	Fun  _fun;
	bool _active;
};

template<class Fun>
ScopeGuard<Fun> make_scope_guard(Fun f)
{
	return ScopeGuard<Fun>(std::move(f));
}

namespace detail {
	enum class ScopeGuardOnExit { };

	template<typename Fun>
	ScopeGuard<Fun> operator+(ScopeGuardOnExit, Fun&& fn)
	{
		return ScopeGuard<Fun>(std::forward<Fun>(fn));
	}
} // namespace detail

/**
 * @brief Usage: SCOPE_EXIT{ fclose(file); };
 * @details SCOPE_EXIT is very useful when dealing with code that is not RAII-wrapped.
 * This includes closing of FILE:s and pipes.
 *
 * Example:
 *
 *     void foo()
 *     {
 *         File* file = fopen(...);
 *         SCOPE_EXIT{ fclose(file); };
 *
 *         if (bar) { return; }
 *         if (baz) { exit(); }
 *         might_throw_an_exception(file);
 *
 *         // fclose will be called automagically here.
 *     }
 */
#define SCOPE_EXIT \
 auto LOGURU_ANONYMOUS_VARIABLE(scope_guard_) = ::emilib::detail::ScopeGuardOnExit() + [&]()

} // namespace emilib
