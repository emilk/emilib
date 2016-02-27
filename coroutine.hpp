#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <string>

namespace emilib {
namespace cr {

struct AbortException {};

class InnerControl;

// This acts like a coroutine, but is implemented as a separate thread.
class Coroutine
{
public:
	// A running count of all coroutines will be appended to debug_name.
	// The resulting name is then given to the thread, and written or errors.
	Coroutine(const char* debug_name, std::function<void(InnerControl& ic)> fun);
	~Coroutine();

	void stop();

	// Returns 'true' on done
	bool poll(double dt);

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

// This is used from within the coroutine.
class InnerControl
{
public:
	InnerControl(Coroutine& cr) : _cr(cr) { }

	// Total running time of this coroutine (sum of all dt).
	double time() const { return _time; }

	// Inner thread: return execution to Outer thread until fun() is true.
	template<typename Fun>
	void wait_for(const Fun& fun)
	{
		while (!fun()) {
			yield();
		}
	}

	// Inner thread: return execution to Outer thread for the next s seconds.
	void wait_sec(double s);

	// Inner thread: Return execution to Outer thread.
	void yield();

	// Called from Outer:
	void poll(double dt);

private:
	Coroutine& _cr;
	double     _time = 0;
};

// -----------------------------------------------

// Helper for handling several coroutines
class CoroutineSet
{
public:
	bool empty() const { return _list.empty(); }
	auto size()  const { return _list.size();  }

	void clear();

	std::shared_ptr<Coroutine> start(const char* debug_name, std::function<void(InnerControl& ic)> fun);

	bool erase(std::shared_ptr<Coroutine> cr);

	void poll(double dt);

private:
	std::list<std::shared_ptr<Coroutine>> _list;
};

} // namespace cr
} // namespace emilib
