#include <cstdlib>
#include <thread>
#include <vector>

#include <read_write_mutex.hpp>

using namespace emilib;

const size_t NUM_RUNS   = 5; // Best of NUM_RUNS
const size_t MIN_WRITES = 1;

// ----------------------------------------------------------------------------

struct StdMutexDB
{
	std::mutex _mutex;
	size_t     _resource = 0;

	size_t read()
	{
		std::lock_guard<std::mutex> l(_mutex);
		return _resource;
	}

	void inc()
	{
		std::lock_guard<std::mutex> l(_mutex);
		++_resource;
	}
};

struct FastRwMutexDB
{
	FastReadWriteMutex _mutex;
	size_t             _resource = 0;

	size_t read()
	{
		FastReadLock l(_mutex);
		return _resource;
	}

	void inc()
	{
		FastWriteLock l(_mutex);
		++_resource;
	}
};

struct SlowRwMutexDB
{
	SlowReadWriteMutex _mutex;
	size_t             _resource = 0;

	size_t read()
	{
		SlowReadLock l(_mutex);
		return _resource;
	}

	void inc()
	{
		SlowWriteLock l(_mutex);
		++_resource;
	}
};

// ----------------------------------------------------------------------------

class TicToc
{
public:
	using Clock = std::chrono::high_resolution_clock;

	TicToc() : _start(Clock::now()) { }

	double sec() const
	{
		using namespace std::chrono;
		auto end = Clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(end - _start).count() * 1e-9;
	}

private:
	Clock::time_point _start;
};

// ----------------------------------------------------------------------------

struct Setup
{
	size_t num_writes;      // Number of writes made by each thread.
	size_t num_threads;     // Number of threads using the database.
	size_t reads_per_write; // Number of reads for each write made by each thread.
};

// sec/access for different versions; best of several runs:
struct Results
{
	double std_mutex = std::numeric_limits<double>::infinity();
	double fast_rw   = std::numeric_limits<double>::infinity();
	double slow_rw   = std::numeric_limits<double>::infinity();
};

// ----------------------------------------------------------------------------

// Returns sec/access
template<class DB>
double benchDataBase(Setup setup)
{
	setup.num_writes = std::max(setup.num_writes, MIN_WRITES);

	DB db;
	std::atomic<bool> start;
	start = false;

	auto job = [&]{
		while (!start);

		for (size_t wi=0; wi<setup.num_writes; ++wi) {
			for (size_t ri=0; ri<setup.reads_per_write; ++ri) {
				db.read();
			}
			db.inc();
		}
	};

	size_t num_reads_tot = 0;
	size_t num_writes_tot = 0;

	std::vector<std::thread> threads;

	for (size_t i=0; i<setup.num_threads; ++i) {
		threads.emplace_back(job);
		num_reads_tot += setup.num_writes * setup.reads_per_write;
		num_writes_tot += setup.num_writes;
	}

	TicToc tt;
	start = true;

	for (auto& t : threads) {
		t.join();
	}

	auto sec = tt.sec();
	if (sec < 0.1) {
		printf("WARNING: %s took just %f sec\n", typeid(DB).name(), sec);
	}
	double sec_per_access = sec / (num_writes_tot + num_reads_tot);

	if (db.read() != num_writes_tot) {
		printf("Something is broken!\n");
		std::abort();
	}

	return sec_per_access;
}

// ----------------------------------------------------------------------------

void testAll(const size_t num_threads, const size_t reads_per_write)
{
	size_t num_writes = 20000000;
	num_writes /= (1 + reads_per_write);

	Setup setup_rw_mutex{num_writes, num_threads, reads_per_write};
	Setup setup_std_mutex = setup_rw_mutex;

	if (setup_std_mutex.num_threads > 1) {
		if (reads_per_write == 0) {
			setup_rw_mutex.num_writes /= 200;
		} else if (reads_per_write == 1) {
			setup_rw_mutex.num_writes /= 100;
		} else {
			setup_rw_mutex.num_writes /= 4;
		}

		setup_std_mutex.num_writes /= 200;
	}

	Results results;

	for (int i = 0; i < NUM_RUNS; ++i) {
		results.std_mutex = std::min(results.std_mutex, benchDataBase<StdMutexDB>(setup_std_mutex));
		results.fast_rw   = std::min(results.fast_rw,   benchDataBase<FastRwMutexDB>(setup_rw_mutex));
		results.slow_rw   = std::min(results.slow_rw,   benchDataBase<SlowRwMutexDB>(setup_rw_mutex));
	}

	printf("  %6.3f          %6.3f      %6.3f  μs/access (lower is better)\n",
	       1e6 * results.std_mutex, 1e6 * results.fast_rw, 1e6 * results.slow_rw);
	fflush(stdout);
}


// ----------------------------------------------------------------------------

int main()
{
	const std::vector<size_t> num_threads_vec     = {1, 2, 4, 6, 8, 10};
	const std::vector<size_t> reads_per_write_vec = {0, 1, 10, 100, 1000, 100000};

	for (const size_t reads_per_write : reads_per_write_vec) {
		printf("%lu reads per write:\n", reads_per_write);
		printf("           std::mutex  FastReadWriteMutex  SlowReadWriteMutex\n");
		for (const size_t num_threads : num_threads_vec) {
			printf("%2lu threads:  ", num_threads);
			fflush(stdout);
			testAll(num_threads, reads_per_write);
		}
		printf("\n");
	}
}


/*
MacBook pro retina 15" results:


Lower numbers are BETTER!

0 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.024           0.034       0.035  μs/access
 2 threads:     3.348           3.663       3.778  μs/access
 4 threads:     4.387           4.302       4.307  μs/access
 6 threads:     4.262           4.453       4.497  μs/access
 8 threads:     4.352           4.562       4.490  μs/access
10 threads:     4.568           4.677       4.836  μs/access

1 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.022           0.024       0.025  μs/access
 2 threads:     3.460           1.885       1.876  μs/access
 4 threads:     4.675           2.398       2.243  μs/access
 6 threads:     4.616           2.253       2.278  μs/access
 8 threads:     4.499           2.288       2.380  μs/access
10 threads:     4.563           2.334       2.401  μs/access

10 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.023           0.016       0.019  μs/access
 2 threads:     3.476           0.355       0.358  μs/access
 4 threads:     4.586           0.412       0.426  μs/access
 6 threads:     4.678           0.414       0.421  μs/access
 8 threads:     4.610           0.418       0.419  μs/access
10 threads:     4.643           0.418       0.425  μs/access

100 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.023           0.015       0.017  μs/access
 2 threads:     3.832           0.045       0.042  μs/access
 4 threads:     4.644           0.042       0.045  μs/access
 6 threads:     4.573           0.043       0.045  μs/access
 8 threads:     4.702           0.043       0.046  μs/access
10 threads:     4.592           0.045       0.046  μs/access

1000 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.022           0.014       0.017  μs/access
 2 threads:     2.989           0.057       0.061  μs/access
 4 threads:     4.394           0.054       0.068  μs/access
 6 threads:     4.598           0.051       0.075  μs/access
 8 threads:     4.572           0.050       0.081  μs/access
10 threads:     4.607           0.053       0.071  μs/access

100000 reads per write:
           std::mutex  FastReadWriteMutex  SlowReadWriteMutex
 1 threads:     0.023           0.015       0.018  μs/access
 2 threads:     3.307           0.056       0.063  μs/access
 4 threads:     4.515           0.060       0.066  μs/access
 6 threads:     4.299           0.057       0.061  μs/access
 8 threads:     4.313           0.055       0.060  μs/access
10 threads:     4.285           0.055       0.058  μs/access
*/
