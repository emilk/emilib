#include "coroutine.hpp"

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
			LOG_F(1, "Aborting coroutine '%s'...", _debug_name.c_str());
			_abort = true;
			while (!_is_done) {
				_inner->poll(0);
			}
		}
		_thread->join();
		CHECK_F(_is_done);
		_thread = nullptr;
	}
}

// Returns 'true' on done
bool Coroutine::poll(double dt)
{
	CHECK_NOTNULL_F(_inner);
	if (!_is_done) {
		_inner->poll(dt);
	}
	return _is_done;
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

	// Let the outher thread do it's buisness. Wait for it to return to us:

	std::unique_lock<std::mutex> lock(_cr._mutex);
	_cr._cond.wait(lock, [=]{ return !_cr._control_is_outer; });
	lock.release();
	CHECK_EQ_F(_cr._control_is_outer.load(), false);

	if (_cr._abort) {
		DLOG_F(1, "%s: throwing AbortException", _cr._debug_name.c_str());
		throw AbortException();
	}
}

// Called from Outer:
void InnerControl::poll(double dt)
{
	CHECK_EQ_F(_cr._control_is_outer.load(), true);
	_cr._control_is_outer = false;
	_cr._mutex.unlock();
	_cr._cond.notify_one();

	// Let the inner thread do it's buisness. Wait for it to return to us:

	std::unique_lock<std::mutex> lock(_cr._mutex);
	_cr._cond.wait(lock, [=]{ return !!_cr._control_is_outer; });
	lock.release();
	CHECK_EQ_F(_cr._control_is_outer.load(), true);

	_time += dt;
}

// ----------------------------------------------------------------------------

void CoroutineSet::clear()
{
	_list.clear();
}

std::shared_ptr<Coroutine> CoroutineSet::start(const char* debug_name, std::function<void(InnerControl& ic)> fun)
{
	auto cr_ptr = std::make_shared<Coroutine>(debug_name, std::move(fun));
	_list.push_back(cr_ptr);
	return cr_ptr;
}

bool CoroutineSet::erase(std::shared_ptr<Coroutine> cr)
{
	auto it = std::find(begin(_list), end(_list), cr);
	if (it != _list.end()) {
		_list.erase(it);
		return true;
	} else {
		return false;
	}
}

void CoroutineSet::poll(double dt)
{
	for (auto it=begin(_list); it!=end(_list); ) {
		if ((*it)->poll(dt)) {
			it = _list.erase(it);
		} else {
			++it;
		}
	}
}

} // namespace cr
} // namespace emilib
