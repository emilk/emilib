// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY
//   Originally made for Ghostel in 2014.

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace emilib {

/**
* Coroutine-ish feature implemented using a thread.
* Useful for implementing a script of some sort where a state-machine would be cumbersome.
* The coroutine (inner) thread is executed only when the owning (outer) thread is paused, and vice versa.
*
* The coroutine has helper functions for waiting for a certain amount of time etc.
* To keep track of the time, a time delta must be supplied when polling a coroutine.
* This allows the library user to for instance slow down time by supplying smaller time deltas
* then the wall clock time.
*
* Example usage can be found in examples/ folder.
*/
namespace cr {

class InnerControl;

// ----------------------------------------------------------------------------

/// This acts like a coroutine, but is implemented as a separate thread.
class Coroutine
{
public:
	/// A running count of all coroutines will be appended to debug_name.
	/// The resulting name is used to name the inner thread and will also be written on errors.
	Coroutine(const char* debug_name, std::function<void(InnerControl& ic)> fun);

	/// Will stop() the coroutine, if not already done().
	~Coroutine();

	/// Abort the inner thread, if not done().
	void stop();

	/// Give control to the coroutine thread.
	/// dt = elapsed time since last call in seconds.
	void poll(double dt);

	/// Has the inner thread finished its execution?
	bool done() const { return _is_done; }

private:
	Coroutine(Coroutine&) = delete;
	Coroutine(Coroutine&&) = delete;
	Coroutine& operator=(Coroutine&) = delete;
	Coroutine& operator=(Coroutine&&) = delete;

	class Thread;
	friend class InnerControl;

	std::string                   _debug_name = "";
	std::unique_ptr<InnerControl> _inner;
	std::unique_ptr<Thread>       _thread;
	std::atomic<bool>             _is_done { false };
	std::mutex                    _mutex;
	std::condition_variable       _cond;
	std::atomic<bool>             _control_is_outer { true };
	std::atomic<bool>             _abort { false };
};

/// ----------------------------------------------------------------------------

/// This is used from within the coroutine.
class InnerControl
{
public:
	InnerControl(Coroutine& cr) : _cr(cr) { }

	/// Total running time of this coroutine (sum of all dt).
	double time() const { return _time; }

	/// Return execution to Outer thread until fun() is true.
	template<typename Fun>
	void wait_for(const Fun& fun)
	{
		while (!fun()) {
			yield();
		}
	}

	/// Return execution to Outer thread for the next s seconds.
	void wait_sec(double s);

	/// Return execution to Outer thread.
	void yield();

private:
	friend Coroutine;
	Coroutine& _cr;
	double     _time = 0;
};

// ----------------------------------------------------------------------------

/// Helper for handling several coroutines.
class CoroutineSet
{
public:
	bool   empty() const { return _list.empty(); }
	size_t size()  const { return _list.size();  }

	/// Stop all running coroutines with no outside handles to them.
	void clear();

	/// You can save the returned handle so you can stop() or erase() it later.
	std::shared_ptr<Coroutine> start(const char* debug_name, std::function<void(InnerControl& ic)> fun);

	/// Remove it from the set. If there are no more handles left for the routine, it will be stopped.
	/// Returns false iff the given handle was not found.
	bool erase(const std::shared_ptr<Coroutine>& cr);

	/// poll all contained coroutines. dt = elapsed time since last call in seconds.
	/// It is safe to call clear(), start() and erase() on this CoroutineSet from within a coroutine.
	void poll(double dt);

private:
	std::vector<std::shared_ptr<Coroutine>> _list;
};

} // namespace cr
} // namespace emilib
