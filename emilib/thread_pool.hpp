// By Emil Ernerfeldt 2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace emilib {

class ThreadPool
{
public:
	using Job = std::function<void()>;

	/// As many threads as cores, but at least 2.
	ThreadPool();

	/// Use this many worker threads.
	explicit ThreadPool(size_t num_threads);

	/// Will block until all jobs have finished.
	~ThreadPool();

	/// Wait for all jobs to finish.
	void wait();

	/// Remove all jobs in the queue (but those that have already started will still finish).
	void clear();

	/// Add to queue and return immediately.
	void add_void(const Job& job);

	/// Add to queue and return immediately.
	template<typename Result>
	std::future<Result> add(std::function<Result()> job)
	{
		const auto promise = std::make_shared<std::promise<Result>>();
		std::future<Result> future = promise->get_future();
		add_void([=]() {
			promise->set_value(job());
		});
		return future;
	}

	// TODO: add way to add jobs to front of queue.
	// TODO: add way to add job after waiting for for empty queue first.

private:
	void _thread_worker(size_t thread_nr);

	std::mutex               _mutex;
	std::vector<std::thread> _threads;
	std::deque<Job>          _job_queue;
	size_t                   _num_unfinished_jobs = 0;
	std::condition_variable  _new_job_cond;
	std::condition_variable  _job_finished_cond;
};

} // namespace emilib
