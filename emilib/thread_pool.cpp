// By Emil Ernerfeldt 2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "thread_pool.hpp"

#include <algorithm>

#include <loguru.hpp>

namespace emilib {

ThreadPool::ThreadPool() : ThreadPool(std::max(2u, std::thread::hardware_concurrency()))
{
}

ThreadPool::ThreadPool(size_t num_threads)
{
    CHECK_NE_F(num_threads, 0u);
    for (size_t i = 0; i < num_threads; ++i) {
        _threads.emplace_back([=](){ _thread_worker(i); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        // Stop the threads by posting empty jobs:
        std::lock_guard<std::mutex> lock(_mutex);
        for (size_t i = 0; i < _threads.size(); ++i) {
            _job_queue.push_back({});
            _new_job_cond.notify_one();
        }
    }

    for (auto& thread : _threads) {
        thread.join();
    }
}

void ThreadPool::add_void(const Job& job)
{
    CHECK_F(!!job);
    std::lock_guard<std::mutex> lock(_mutex);
    _job_queue.push_back(job);
    ++_num_unfinished_jobs;
    _new_job_cond.notify_one();
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _job_finished_cond.wait(lock, [this]{ return _num_unfinished_jobs == 0; });
}

void ThreadPool::clear()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _num_unfinished_jobs -= _job_queue.size();
    _job_queue.clear();
}

void ThreadPool::_thread_worker(size_t thread_nr)
{
    char thread_name[32];
    snprintf(thread_name, sizeof(thread_name) - 1, "pool_worker_%lu", thread_nr);
    loguru::set_thread_name(thread_name);

    while (true) {
        Job job;

        {
            std::unique_lock<std::mutex> lock(_mutex);
            _new_job_cond.wait(lock, [this]{ return !_job_queue.empty(); });
            CHECK_F(!_job_queue.empty());
            job = std::move(_job_queue.front());
            _job_queue.pop_front();
        }

        if (!job) { break; }

        job();

        std::unique_lock<std::mutex> lock(_mutex);
        --_num_unfinished_jobs;
        _job_finished_cond.notify_all();
    }
}

} // namespace emilib
