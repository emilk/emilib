// Created by Emil Ernerfeldt on 2013-01-24.
// Cleaned up 2016-01-28.
// Added to emilib on 2016-08-09.
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace emilib {

/*
Mutex optimized for locking things that are often read, seldom written.
    * Many can read at the same time.
    * Only one can write at the same time.
    * No-one can read while someone is writing.

Is ReadWriteMutex right for you?
    * If most access are writes, use std::mutex.
    * Else if you almost always access the resource from one thread, ReadLock is slightly faster than std::mutex.
    * Else (if most access are read-only, and there is more than one thread): ReadWriteMutex will be up to 100x faster then std::mutex.

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

// Use this if reads are quick.
class FastReadWriteMutex
{
public:
    FastReadWriteMutex() { }

private:
    FastReadWriteMutex(FastReadWriteMutex&) = delete;
    FastReadWriteMutex(FastReadWriteMutex&&) = delete;
    FastReadWriteMutex& operator=(FastReadWriteMutex&) = delete;
    FastReadWriteMutex& operator=(FastReadWriteMutex&&) = delete;

    friend class FastReadLock;
    friend class FastWriteLock;

    std::atomic<int>  _num_readers{0};
    std::atomic<bool> _has_writer{false}; // Is there a writer working (or trying to) ?
    std::mutex        _write_mutex;
};

// ----------------------------------------------------------------------------

class FastReadLock
{
public:
    explicit FastReadLock(FastReadWriteMutex& mut) : _rw_mutex(mut)
    {
        lock();
    }

    // Won't lock right away
    FastReadLock(FastReadWriteMutex& mut, std::defer_lock_t) : _rw_mutex(mut)
    {
    }

    ~FastReadLock() { unlock(); }

    // Lock, unless already locked.
    void lock()
    {
        if (_locked) {
            return;
        }

        while (_rw_mutex._has_writer) {
            // First check here to stop readers while write is in progress.
            // This is to ensure _rw_mutex._num_readers can go to zero (needed for write to start).
            std::lock_guard<std::mutex> l(_rw_mutex._write_mutex); // wait for the writer to be done
        }

        // If a writer starts here, it may think there are no readers, which is why we re-check _rw_mutex._has_writer again.

        ++_rw_mutex._num_readers; // Tell any writers that there is now someone reading

        // Check so no write began before we incremented _rw_mutex._num_readers
        while (_rw_mutex._has_writer) {
            // A write is in progress or is waiting to start!
            --_rw_mutex._num_readers; // We changed our mind

            std::lock_guard<std::mutex> l(_rw_mutex._write_mutex); // wait for the writer to be done

            ++_rw_mutex._num_readers; // Let's try again
        }

        _locked = true;
    }

    // unlock, unless already unlocked.
    void unlock()
    {
        if (_locked)
        {
            --_rw_mutex._num_readers;
            _locked = false;
        }
    }

private:
    FastReadLock(FastReadLock&) = delete;
    FastReadLock(FastReadLock&&) = delete;
    FastReadLock& operator=(FastReadLock&) = delete;
    FastReadLock& operator=(FastReadLock&&) = delete;

    FastReadWriteMutex& _rw_mutex;
    bool                _locked{false};
};

// ----------------------------------------------------------------------------

class FastWriteLock
{
public:
    explicit FastWriteLock(FastReadWriteMutex& mut) : _rw_mutex(mut)
    {
        lock();
    }

    // Won't lock right away
    FastWriteLock(FastReadWriteMutex& mut, std::defer_lock_t) : _rw_mutex(mut)
    {
    }

    ~FastWriteLock() { unlock(); }

    // Lock, unless already locked.
    void lock()
    {
        if (_locked) {
            return;
        }

        _rw_mutex._write_mutex.lock(); // Ensure we are the only one writing
        _rw_mutex._has_writer = true; // Steer new readers into a lock (provided by the above mutex)

        // Wait for all readers to finish.
        // Busy spin-waiting
        while (_rw_mutex._num_readers != 0) {
            std::this_thread::yield(); // Give the reader-threads a chance to finish.
        }

        // All readers have finished - time to write!
        _locked = true;
    }

    // unlock, unless already unlocked.
    void unlock()
    {
        if (_locked) {
            _rw_mutex._has_writer = false;
            _rw_mutex._write_mutex.unlock();
            _locked = false;
        }
    }

private:
    FastWriteLock(FastWriteLock&) = delete;
    FastWriteLock(FastWriteLock&&) = delete;
    FastWriteLock& operator=(FastWriteLock&) = delete;
    FastWriteLock& operator=(FastWriteLock&&) = delete;

    FastReadWriteMutex& _rw_mutex;
    bool                _locked{false};
};

// ----------------------------------------------------------------------------

// This is a good mutex if reading is slow to save on CPU usage when there is a thread waiting to write.
class SlowReadWriteMutex
{
public:
    SlowReadWriteMutex() { }

private:
    SlowReadWriteMutex(SlowReadWriteMutex&) = delete;
    SlowReadWriteMutex(SlowReadWriteMutex&&) = delete;
    SlowReadWriteMutex& operator=(SlowReadWriteMutex&) = delete;
    SlowReadWriteMutex& operator=(SlowReadWriteMutex&&) = delete;

    friend class SlowReadLock;
    friend class SlowWriteLock;

    std::atomic<int>        _num_readers{0};
    std::atomic<bool>       _has_writer{false}; // Is there a writer working (or trying to) ?
    std::mutex              _write_mutex;
    std::condition_variable _reader_done_cond;  // Signals a waiting writer that a read has finishes
    std::mutex              _reader_done_mutex; // TODO: this mutex isn't really needed, but can't use a condition_variable without it
};

// ----------------------------------------------------------------------------

class SlowReadLock
{
public:
    explicit SlowReadLock(SlowReadWriteMutex& mut) : _rw_mutex(mut)
    {
        lock();
    }

    // Won't lock right away
    SlowReadLock(SlowReadWriteMutex& mut, std::defer_lock_t) : _rw_mutex(mut)
    {
    }

    ~SlowReadLock() { unlock(); }

    // Lock, unless already locked.
    void lock()
    {
        if (_locked) {
            return;
        }

        while (_rw_mutex._has_writer) {
            // First check here to stop readers while write is in progress.
            // This is to ensure _rw_mutex._num_readers can go to zero (needed for write to start).
            std::lock_guard<std::mutex> l(_rw_mutex._write_mutex); // wait for the writer to be done
        }

        // If a writer starts here, it may think there are no readers, which is why we re-check _rw_mutex._has_writer again.

        ++_rw_mutex._num_readers; // Tell any writers that there is now someone reading

        // Check so no write began before we incremented _rw_mutex._num_readers
        while (_rw_mutex._has_writer) {
            // A write is in progress or is waiting to start!
            --_rw_mutex._num_readers; // We changed our mind

            {
                std::unique_lock<std::mutex> l(_rw_mutex._reader_done_mutex);
                _rw_mutex._reader_done_cond.notify_one(); // Tell the writer we did (he may be waiting for _rw_mutex._num_readers to go to zero)
            }

            std::lock_guard<std::mutex> l(_rw_mutex._write_mutex); // wait for the writer to be done

            ++_rw_mutex._num_readers; // Let's try again
        }

        _locked = true;
    }

    // unlock, unless already unlocked.
    void unlock()
    {
        if (_locked)
        {
            --_rw_mutex._num_readers;

            if (_rw_mutex._has_writer) {
                // A writer is waiting for all readers to finish. Tell him one more has:
                std::unique_lock<std::mutex> l(_rw_mutex._reader_done_mutex);
                _rw_mutex._reader_done_cond.notify_one();
            }

            _locked = false;
        }
    }

private:
    SlowReadLock(SlowReadLock&) = delete;
    SlowReadLock(SlowReadLock&&) = delete;
    SlowReadLock& operator=(SlowReadLock&) = delete;
    SlowReadLock& operator=(SlowReadLock&&) = delete;

    SlowReadWriteMutex& _rw_mutex;
    bool                _locked{false};
};

// ----------------------------------------------------------------------------

class SlowWriteLock
{
public:
    explicit SlowWriteLock(SlowReadWriteMutex& mut) : _rw_mutex(mut)
    {
        lock();
    }

    // Won't lock right away
    SlowWriteLock(SlowReadWriteMutex& mut, std::defer_lock_t) : _rw_mutex(mut)
    {
    }

    ~SlowWriteLock() { unlock(); }

    // Lock, unless already locked.
    void lock()
    {
        if (_locked) {
            return;
        }

        _rw_mutex._write_mutex.lock(); // Ensure we are the only one writing
        _rw_mutex._has_writer = true; // Steer new readers into a lock (provided by the above mutex)

        // Wait for all readers to finish.
        if (_rw_mutex._num_readers!=0) {
            std::unique_lock<std::mutex> l(_rw_mutex._reader_done_mutex); // TODO: this mutex isn't really needed, but can't use a condition_variable without it
            _rw_mutex._reader_done_cond.wait(l, [=]{ return _rw_mutex._num_readers==0; });
        }

        // All readers have finished - time to write!
        _locked = true;
    }

    // unlock, unless already unlocked.
    void unlock()
    {
        if (_locked) {
            _rw_mutex._has_writer = false;
            _rw_mutex._write_mutex.unlock();
            _locked = false;
        }
    }

private:
    SlowWriteLock(SlowWriteLock&) = delete;
    SlowWriteLock(SlowWriteLock&&) = delete;
    SlowWriteLock& operator=(SlowWriteLock&) = delete;
    SlowWriteLock& operator=(SlowWriteLock&&) = delete;

    SlowReadWriteMutex& _rw_mutex;
    bool                _locked{false};
};

// ----------------------------------------------------------------------------

} // namespace emilib
