// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "coroutine.hpp"

#include <algorithm>
#include <thread>

#define LOGURU_WITH_STREAMS 1
#include <loguru.hpp>

namespace emilib {
namespace cr {

// This is thrown when the outer thread stop():s the coroutine.
struct AbortException {};

static std::atomic<unsigned> s_cr_counter { 0 };

// ----------------------------------------------------------------------------

// Trick for allowing forward-declaration of std::thread.
class Coroutine::Thread : public std::thread
{
public:
	using std::thread::thread;
};

// ----------------------------------------------------------------------------

Coroutine::Coroutine(const char* debug_name_base, std::function<void(InnerControl& ic)> fun)
{
	_debug_name = std::string(debug_name_base) + " " + std::to_string(s_cr_counter++);
	DLOG_F(1, "%s: Coroutine starting", _debug_name.c_str());

	_mutex.lock();
	_inner = std::make_unique<InnerControl>(*this);

	_thread = std::make_unique<Thread>([this, fun]{
		loguru::set_thread_name(_debug_name.c_str());
		ERROR_CONTEXT("Coroutine", _debug_name.c_str());
		DLOG_F(1, "%s: Coroutine thread starting up", _debug_name.c_str());

		_mutex.lock();
		CHECK_F(!_control_is_outer);

		try {
			fun(*_inner);
		} catch (AbortException&) {
			DLOG_F(1, "%s: AbortException caught", _debug_name.c_str());
		} catch (std::exception& e) {
			LOG_F(ERROR, "%s: Exception caught from Coroutine: %s", _debug_name.c_str(), e.what());
		} catch (...) {
			LOG_F(ERROR, "%s: Unknown exception caught from Coroutine", _debug_name.c_str());
		}
		_is_done = true;
		_control_is_outer = true;
		_mutex.unlock();
		_cond.notify_one();

		DLOG_F(1, "%s: Coroutine thread shutting down", _debug_name.c_str());
	});

	CHECK_NOTNULL_F(_inner);
}

Coroutine::~Coroutine()
{
	stop();
	CHECK_F(!_thread);
	DLOG_F(1, "%s: Coroutine destroyed", _debug_name.c_str());
}

void Coroutine::stop()
{
	if (_thread) {
		if (!_is_done) {
			LOG_SCOPE_F(1, "Aborting coroutine '%s'...", _debug_name.c_str());
			_abort = true;
			while (!_is_done) {
				poll(0);
			}
		}
		_thread->join();
		CHECK_F(_is_done);
		CHECK_F(_control_is_outer);
		_mutex.unlock(); // We need to unlock before destroying it.
		_thread = nullptr;
	}
}

// Returns 'true' on done
void Coroutine::poll(double dt)
{
	CHECK_NOTNULL_F(_inner);
	if (_is_done) { return; }

	CHECK_EQ_F(_control_is_outer.load(), true);
	_control_is_outer = false;
	_mutex.unlock();
	_cond.notify_one();

	// Let the inner thread do it's business. Wait for it to return to us:

	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [&]{ return !!_control_is_outer; });
	lock.release(); // Keep the _mutex locked.
	CHECK_EQ_F(_control_is_outer.load(), true);

	_inner->_time += dt;
}

// ----------------------------------------------------------------------------

void InnerControl::wait_sec(double s)
{
	auto target_time = _time + s;
	wait_for( [=](){ return target_time <= _time; } );
}

void InnerControl::yield()
{
	CHECK_EQ_F(_cr._control_is_outer.load(), false);
	_cr._control_is_outer = true;
	_cr._mutex.unlock();
	_cr._cond.notify_one();

	// Let the outer thread do it's business. Wait for it to return to us:

	std::unique_lock<std::mutex> lock(_cr._mutex);
	_cr._cond.wait(lock, [=]{ return !_cr._control_is_outer; });
	lock.release(); // Keep the _mutex locked.
	CHECK_EQ_F(_cr._control_is_outer.load(), false);

	if (_cr._abort) {
		DLOG_F(1, "%s: throwing AbortException", _cr._debug_name.c_str());
		throw AbortException();
	}
}

// ----------------------------------------------------------------------------

void CoroutineSet::clear()
{
	for (auto& cr_ptr : _list) {
		cr_ptr.reset();
	}
}

std::shared_ptr<Coroutine> CoroutineSet::start(const char* debug_name, std::function<void(InnerControl& ic)> fun)
{
	auto cr_ptr = std::make_shared<Coroutine>(debug_name, std::move(fun));
	_list.push_back(cr_ptr);
	return cr_ptr;
}

bool CoroutineSet::erase(const std::shared_ptr<Coroutine>& cr_ptr)
{
	auto it = std::find(begin(_list), end(_list), cr_ptr);
	if (it != _list.end()) {
		// Do not modify _list - we might be currently iterating over it!
		// Instead, just clear the pointer, and let poll() erase it later.
		it->reset();
		return true;
	} else {
		return false;
	}
}

void CoroutineSet::poll(double dt)
{
	// Take care to allow calls to start(), erase() and clear() while this is running (from within poll()):
	for (size_t i = 0; i < _list.size(); ++i) {
		auto cr_ptr = _list[i];
		if (cr_ptr) {
			cr_ptr->poll(dt);
			if (cr_ptr->done()) {
				_list[i] = nullptr;
			}
		}
	}

	auto it = std::remove_if(_list.begin(), _list.end(),
		[&](const auto& cr_ptr) { return cr_ptr == nullptr; } );
	_list.erase(it, _list.end());
}

} // namespace cr
} // namespace emilib
