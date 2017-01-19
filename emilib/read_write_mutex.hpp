// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY
//                 - 2013-01-24 - Initial conception
//                 - 2016-01-28 - Cleaned up.
//                 - 2016-08-09 - Added to emilib.
//   Version 1.0.0 - 2016-08-14 - Made into a drop-in std::shared_mutex replacement.
//   Version 1.0.1 - 2016-08-24 - Bug fix in try_lock (thanks, Ninja101!)
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace emilib {

/*
Two mutex classes that acts like C++17's std::shared_mutex, but are faster and C++11

Mutex optimized for locking things that are often read, seldom written.
	* Many can read at the same time.
	* Only one can write at the same time.
	* No-one can read while someone is writing.

Is ReadWriteMutex right for you?
	* If most access are writes, use std::mutex.
	* Else if you almost always access the resource from one thread, ReadLock is slightly faster than std::mutex.
	* Else (if most access are read-only, and there is more than one thread): ReadWriteMutex will be up to 100x faster then std::mutex.

What about std::shared_timed_mutex?
   * I benchmarked GCC5:s std::shared_timed_mutex in rw_mutex_benchmark.cpp, and it's SLOW. AVOID!

 What about std::shared_mutex?
	* It is C++17, so it's currently from the future.
	* I benched GCC6:s std::shared_mutex and:
		* Unique locking is 50% faster than this library.
		* Shared locking is 3x-4x slower than ReadWriteMutex.
		* When mixing read/write locks, std::shared_mutex can be up to 6x slower than this library.

 The FastWriteLock will spin-wait for zero readers. This is the best choice when you expect the reader to be done quickly.
 With SlowWriteLock there is a std::mutex instead of a spin-lock in WriteLock. This is useful to save CPU if the read operation can take a long time, e.g. file or network access.

 The mutex is *not* recursive, i.e. you must not lock it twice on the same thread.

 Example usage:

	class StringMonitor
	{
	public:
		std:string get() const
		{
			FastReadLock lock(_mutex);
			return _name;
		}

		void set(std::string str)
		{
			FastWriteLock lock(_mutex);
			_name.swap(str);
		}

	private:
		mutable FastReadWriteMutex _mutex;
		std::string                _name;
	};
 */

// ----------------------------------------------------------------------------

/// Use this if reads are quick.
/// This is a drop-in replacement for C++17's std::shared_mutex.
/// This mutex is NOT recursive!
class FastReadWriteMutex
{
public:
	FastReadWriteMutex() { }

	/// Locks the mutex for exclusive access (e.g. for a write operation).
	/// If another thread has already locked the mutex, a call to lock will block execution until the lock is acquired.
	/// This will be done using a spin-lock. If this is wasting too much CPU, consider using SlowReadWriteMutex instead.
	/// If lock is called by a thread that already owns the mutex in any mode (shared or exclusive), the behavior is undefined.
	void lock()
	{
		_write_mutex.lock(); // Ensure we are the only one writing
		_has_writer = true; // Steer new readers into a lock (provided by the above mutex)

		// Wait for all readers to finish.
		// Busy spin-waiting
		while (_num_readers != 0) {
			std::this_thread::yield(); // Give the reader-threads a chance to finish.
		}

		// All readers have finished - we are not locked exclusively!
	}

	/// Tries to lock the mutex. Returns immediately. On successful lock acquisition returns true, otherwise returns false.
	/// This function is allowed to fail spuriously and return false even if the mutex is not currently locked by any other thread.
	/// If try_lock is called by a thread that already owns the mutex, the behavior is undefined.
	bool try_lock()
	{
		if (!_write_mutex.try_lock()) {
			return false;
		}

		_has_writer = true;

		if (_num_readers == 0) {
			return true;
		} else {
			_write_mutex.unlock();
			_has_writer = false;
			return false;
		}
	}

	/// Unlocks the mutex.
	/// The mutex must be locked by the current thread of execution, otherwise, the behavior is undefined.
	void unlock()
	{
		_has_writer = false;
		_write_mutex.unlock();
	}

	/// Acquires shared ownership of the mutex (e.g. for a read operation).
	/// If another thread is holding the mutex in exclusive ownership,
	/// a call to lock_shared will block execution until shared ownership can be acquired.
	void lock_shared()
	{
		while (_has_writer) {
			// First check here to stop readers while write is in progress.
			// This is to ensure _num_readers can go to zero (needed for write to start).
			std::lock_guard<std::mutex>{_write_mutex}; // wait for the writer to be done
		}

		// If a writer starts here, it may think there are no readers, which is why we re-check _has_writer again.

		++_num_readers; // Tell any writers that there is now someone reading

		// Check so no write began before we incremented _num_readers
		while (_has_writer) {
			// A write is in progress or is waiting to start!
			--_num_readers; // We changed our mind

			std::lock_guard<std::mutex>{_write_mutex}; // wait for the writer to be done

			++_num_readers; // Let's try again
		}
	}

	/// Tries to lock the mutex in shared mode. Returns immediately. On successful lock acquisition returns true, otherwise returns false.
	/// This function is allowed to fail spuriously and return false even if the mutex is not currenly exclusively locked by any other thread.
	bool try_lock_shared()
	{
		if (_has_writer) {
			return false;
		}

		++_num_readers; // Tell any writers that there is now someone reading

		// Check so no write began before we incremented _num_readers
		if (_has_writer) {
			--_num_readers;
			return false;
		}

		return true;
	}

	/// Releases the mutex from shared ownership by the calling thread.
	/// The mutex must be locked by the current thread of execution in shared mode, otherwise, the behavior is undefined.
	void unlock_shared()
	{
		--_num_readers;
	}

private:
	FastReadWriteMutex(FastReadWriteMutex&) = delete;
	FastReadWriteMutex(FastReadWriteMutex&&) = delete;
	FastReadWriteMutex& operator=(FastReadWriteMutex&) = delete;
	FastReadWriteMutex& operator=(FastReadWriteMutex&&) = delete;

	std::atomic<int>  _num_readers{0};
	std::atomic<bool> _has_writer{false}; // Is there a writer working (or trying to) ?
	std::mutex        _write_mutex;
};

// ----------------------------------------------------------------------------

/// This is a good mutex if reading is slow to save on CPU usage when there is a thread waiting to write.
/// This is a drop-in replacement for C++17's std::shared_mutex.
/// This mutex is NOT recursive!
class SlowReadWriteMutex
{
public:
	SlowReadWriteMutex() { }

	/// Locks the mutex for exclusive access (e.g. for a write operation).
	/// If another thread has already locked the mutex, a call to lock will block execution until the lock is acquired.
	/// If lock is called by a thread that already owns the mutex in any mode (shared or exclusive), the behavior is undefined.
	void lock()
	{
		_write_mutex.lock(); // Ensure we are the only one writing
		_has_writer = true; // Steer new readers into a lock (provided by the above mutex)

		// Wait for all readers to finish.
		if (_num_readers != 0) {
			std::unique_lock<std::mutex> lock{_reader_done_mutex};
			_reader_done_cond.wait(lock, [=]{ return _num_readers==0; });
		}

		// All readers have finished - we are not locked exclusively!
	}

	/// Tries to lock the mutex. Returns immediately. On successful lock acquisition returns true, otherwise returns false.
	/// This function is allowed to fail spuriously and return false even if the mutex is not currently locked by any other thread.
	/// If try_lock is called by a thread that already owns the mutex, the behavior is undefined.
	bool try_lock()
	{
		if (!_write_mutex.try_lock()) {
			return false;
		}

		_has_writer = true;

		if (_num_readers == 0) {
			return true;
		} else {
			_write_mutex.unlock();
			_has_writer = false;
			return false;
		}
	}

	/*
	Unlocks the mutex.
	The mutex must be locked by the current thread of execution, otherwise, the behavior is undefined.
	*/
	void unlock()
	{
		_has_writer = false;
		_write_mutex.unlock();
	}

	/// Acquires shared ownership of the mutex (e.g. for a read operation).
	/// If another thread is holding the mutex in exclusive ownership,
	/// a call to lock_shared will block execution until shared ownership can be acquired.
	void lock_shared()
	{
		while (_has_writer) {
			// First check here to stop readers while write is in progress.
			// This is to ensure _num_readers can go to zero (needed for write to start).
			std::lock_guard<std::mutex> lock{_write_mutex}; // wait for the writer to be done
		}

		// If a writer starts here, it may think there are no readers, which is why we re-check _has_writer again.

		++_num_readers; // Tell any writers that there is now someone reading

		// Check so no write began before we incremented _num_readers
		while (_has_writer) {
			// A write is in progress or is waiting to start!

			{
				std::lock_guard<std::mutex> lock{_reader_done_mutex};
				--_num_readers; // We changed our mind
			}

			_reader_done_cond.notify_one(); // Tell the writer we did (he may be waiting for _num_readers to go to zero)

			std::lock_guard<std::mutex>{_write_mutex}; // wait for the writer to be done

			++_num_readers; // Let's try again
		}
	}

	/// Tries to lock the mutex in shared mode. Returns immediately. On successful lock acquisition returns true, otherwise returns false.
	/// This function is allowed to fail spuriously and return false even if the mutex is not currenly exclusively locked by any other thread.
	bool try_lock_shared()
	{
		if (_has_writer) {
			return false;
		}

		++_num_readers; // Tell any writers that there is now someone reading

		// Check so no write began before we incremented _num_readers
		if (_has_writer) {
			--_num_readers;
			return false;
		}

		return true;
	}

	/// Releases the mutex from shared ownership by the calling thread.
	/// The mutex must be locked by the current thread of execution in shared mode, otherwise, the behavior is undefined.
	void unlock_shared()
	{
		--_num_readers;

		if (_has_writer) {
			// A writer is waiting for all readers to finish. Tell him one more has:
			_reader_done_cond.notify_one();
		}
	}

private:
	SlowReadWriteMutex(SlowReadWriteMutex&) = delete;
	SlowReadWriteMutex(SlowReadWriteMutex&&) = delete;
	SlowReadWriteMutex& operator=(SlowReadWriteMutex&) = delete;
	SlowReadWriteMutex& operator=(SlowReadWriteMutex&&) = delete;

	std::atomic<int>        _num_readers{0};
	std::atomic<bool>       _has_writer{false}; // Is there a writer working (or trying to) ?
	std::mutex              _write_mutex;
	std::condition_variable _reader_done_cond;  // Signals a waiting writer that a read has finishes
	std::mutex              _reader_done_mutex; // Synchronizes _num_readers vs _reader_done_cond
};

// ----------------------------------------------------------------------------

/// This is a drop-in replacement for C++14's std::shared_lock
template<typename MutexType>
class ReadLock
{
public:
	explicit ReadLock(MutexType& mut) : _rw_mutex(mut)
	{
		lock();
	}

	/// Won't lock right away
	ReadLock(MutexType& mut, std::defer_lock_t) : _rw_mutex(mut)
	{
	}

	~ReadLock() { unlock(); }

	/// Lock, unless already locked.
	void lock()
	{
		if (!_locked) {
			_rw_mutex.lock_shared();
			_locked = true;
		}
	}

	/// Does not block. Returns true iff the mutex is locked by this thread after the call.
	bool try_lock()
	{
		if (!_locked) {
			_locked = _rw_mutex.try_lock_shared();
		}
		return _locked;
	}

	/// unlock, unless already unlocked.
	void unlock()
	{
		if (_locked) {
			_rw_mutex.unlock_shared();
			_locked = false;
		}
	}

private:
	ReadLock(ReadLock&) = delete;
	ReadLock(ReadLock&&) = delete;
	ReadLock& operator=(ReadLock&) = delete;
	ReadLock& operator=(ReadLock&&) = delete;

	MutexType& _rw_mutex;
	bool       _locked{false};
};

// ----------------------------------------------------------------------------

/// This is a drop-in replacement for C++11's std::unique_lock
template<typename MutexType>
class WriteLock
{
public:
	explicit WriteLock(MutexType& mut) : _rw_mutex(mut)
	{
		lock();
	}

	/// Won't lock right away
	WriteLock(MutexType& mut, std::defer_lock_t) : _rw_mutex(mut)
	{
	}

	~WriteLock() { unlock(); }

	/// Lock, unless already locked.
	void lock()
	{
		if (!_locked) {
			_rw_mutex.lock();
			_locked = true;
		}
	}

	/// Does not block. Returns true iff the mutex is locked by this thread after the call.
	bool try_lock()
	{
		if (!_locked) {
			_locked = _rw_mutex.try_lock();
		}
		return _locked;
	}

	/// unlock, unless already unlocked.
	void unlock()
	{
		if (_locked) {
			_rw_mutex.unlock();
			_locked = false;
		}
	}

private:
	WriteLock(WriteLock&) = delete;
	WriteLock(WriteLock&&) = delete;
	WriteLock& operator=(WriteLock&) = delete;
	WriteLock& operator=(WriteLock&&) = delete;

	MutexType& _rw_mutex;
	bool       _locked{false};
};

// ----------------------------------------------------------------------------

} // namespace emilib
