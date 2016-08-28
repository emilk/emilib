#include <cstdlib>
#include <shared_mutex>
#include <thread>
#include <vector>

#include <emilib/read_write_mutex.hpp>

using namespace emilib;

const size_t NUM_RUNS   = 3; // Best of NUM_RUNS
const size_t MIN_WRITES = 1;
const double MIN_BENCHMARK_DURATION = 0.05; // Warn if a test is run for less time than this

const std::vector<size_t> num_threads_vec     = {1, 2, 4, 6, 8, 10};
const std::vector<size_t> reads_per_write_vec = {0, 1, 2, 5, 10, 100, 1000, 100000};

// ----------------------------------------------------------------------------

struct StdMutexDB
{
	std::mutex _mutex;
	size_t     _resource = 0;

	size_t read()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _resource;
	}

	void inc()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		++_resource;
	}
};

struct StdSharedMutexDB
{
	std::shared_mutex _mutex;
	size_t            _resource = 0;

	size_t read()
	{
		std::shared_lock<std::shared_mutex> lock(_mutex);
		return _resource;
	}

	void inc()
	{
		std::unique_lock<std::shared_mutex> lock(_mutex);
		++_resource;
	}
};

struct FastRwMutexDB
{
	FastReadWriteMutex _mutex;
	size_t             _resource = 0;

	size_t read()
	{
		ReadLock<FastReadWriteMutex> lock(_mutex);
		// std::shared_lock<FastReadWriteMutex> lock(_mutex); // Equivalent
		return _resource;
	}

	void inc()
	{
		WriteLock<FastReadWriteMutex> lock(_mutex);
		// std::unique_lock<FastReadWriteMutex> lock(_mutex); // Equivalent
		++_resource;
	}
};

struct SlowRwMutexDB
{
	SlowReadWriteMutex _mutex;
	size_t             _resource = 0;

	size_t read()
	{
		ReadLock<SlowReadWriteMutex> lock(_mutex);
		// std::shared_lock<SlowReadWriteMutex> lock(_mutex); // Equivalent
		return _resource;
	}

	void inc()
	{
		WriteLock<SlowReadWriteMutex> lock(_mutex);
		// std::unique_lock<SlowReadWriteMutex> lock(_mutex); // Equivalent
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
	double std_mutex        = std::numeric_limits<double>::infinity();
	double std_shared_mutex = std::numeric_limits<double>::infinity();
	double fast_rw          = std::numeric_limits<double>::infinity();
	double slow_rw          = std::numeric_limits<double>::infinity();
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
	if (sec < MIN_BENCHMARK_DURATION) {
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
	// Try to lower number of writes for benchmark we now to be slow.
	// This doesn't affect the end results, just the time we have to wait for them.
	size_t num_writes = 15000000;
	num_writes /= (1 + reads_per_write);

	Setup setup_rw_mutex{num_writes, num_threads, reads_per_write};
	Setup setup_std_mutex = setup_rw_mutex;

	if (setup_std_mutex.num_threads > 1) {
		if (reads_per_write == 0) {
			setup_rw_mutex.num_writes /= 200;
		} else if (reads_per_write < 5) {
			setup_rw_mutex.num_writes /= 100;
		} else if (reads_per_write == 5) {
			setup_rw_mutex.num_writes /= 10;
		} else {
			setup_rw_mutex.num_writes /= 4;
		}

		setup_std_mutex.num_writes /= 200;
	}

	Setup setup_std_shared_mutex = setup_rw_mutex;
	if (reads_per_write > 1) {
		setup_std_shared_mutex.num_writes /= 4;
	}

	Results results;

	for (size_t i = 0; i < NUM_RUNS; ++i) {
		results.std_mutex        = std::min(results.std_mutex,        benchDataBase<StdMutexDB>(setup_std_mutex));
		results.std_shared_mutex = std::min(results.std_shared_mutex, benchDataBase<StdSharedMutexDB>(setup_std_shared_mutex));
		results.fast_rw          = std::min(results.fast_rw,          benchDataBase<FastRwMutexDB>(setup_rw_mutex));
		results.slow_rw          = std::min(results.slow_rw,          benchDataBase<SlowRwMutexDB>(setup_rw_mutex));
	}

	printf("  %6.3f        %6.3f      %6.3f              %6.3f  μs/access (lower is better)\n",
	       1e6 * results.std_mutex, 1e6 * results.std_shared_mutex, 1e6 * results.fast_rw, 1e6 * results.slow_rw);
	fflush(stdout);
}


// ----------------------------------------------------------------------------

int main()
{
	for (const size_t reads_per_write : reads_per_write_vec) {
		printf("%lu reads per write:\n", reads_per_write);
		printf("           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex\n");
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

Clang:

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



GCC5:
	0 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.022            0.056          0.048               0.048  μs/access (lower is better)
	 2 threads:     3.893            0.339          3.729               3.718  μs/access (lower is better)
	 4 threads:     4.385            1.259          4.469               4.214  μs/access (lower is better)
	 6 threads:     4.207            4.613          4.545               4.738  μs/access (lower is better)
	 8 threads:     4.712           26.227          4.783               4.788  μs/access (lower is better)
	10 threads:     4.650           37.148          4.810               4.709  μs/access (lower is better)

	1 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.022            0.050          0.029               0.029  μs/access (lower is better)
	 2 threads:     3.541            0.392          1.846               1.935  μs/access (lower is better)
	 4 threads:     4.589            1.395          2.203               2.270  μs/access (lower is better)
	 6 threads:     4.608           12.475          2.376               2.400  μs/access (lower is better)
	 8 threads:     4.394           16.157          2.251               2.207  μs/access (lower is better)
	10 threads:     4.420           23.489          2.352               2.409  μs/access (lower is better)

	10 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.022            0.045          0.015               0.015  μs/access (lower is better)
	 2 threads:     4.018            0.858          0.364               0.370  μs/access (lower is better)
	 4 threads:     4.473            2.321          0.420               0.444  μs/access (lower is better)
	 6 threads:     4.498            6.979          0.416               0.423  μs/access (lower is better)
	 8 threads:     4.730            9.932          0.413               0.426  μs/access (lower is better)
	10 threads:     4.557            9.561          0.411               0.420  μs/access (lower is better)

	100 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023            0.046          0.013               0.013  μs/access (lower is better)
	 2 threads:     3.582            3.496          0.041               0.041  μs/access (lower is better)
	 4 threads:     4.109            0.953          0.048               0.051  μs/access (lower is better)
	 6 threads:     4.400            1.457          0.051               0.050  μs/access (lower is better)
	 8 threads:     4.478            5.065          0.043               0.043  μs/access (lower is better)
	10 threads:     4.183            8.110          0.042               0.043  μs/access (lower is better)

	1000 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.026            0.050          0.013               0.014  μs/access (lower is better)
	 2 threads:     3.712            6.317          0.051               0.059  μs/access (lower is better)
	 4 threads:     4.286            8.848          0.051               0.058  μs/access (lower is better)
	 6 threads:     4.170            8.983          0.047               0.068  μs/access (lower is better)
	 8 threads:     4.284            8.749          0.046               0.069  μs/access (lower is better)
	10 threads:     4.307            8.548          0.049               0.066  μs/access (lower is better)

	100000 reads per write:
	           std::mutex  std::shared_timed_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.022            0.044          0.012               0.013  μs/access (lower is better)
	 2 threads:     3.711            7.937          0.060               0.059  μs/access (lower is better)
	 4 threads:     4.466            9.201          0.062               0.063  μs/access (lower is better)
	 6 threads:     4.631            9.159          0.058               0.062  μs/access (lower is better)
	 8 threads:     4.628            9.301          0.057               0.060  μs/access (lower is better)
	10 threads:     4.407            8.874          0.056               0.060  μs/access (lower is better)


GCC6:
	0 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.049       0.050               0.049  μs/access (lower is better)
	 2 threads:     3.116         2.271       3.526               3.483  μs/access (lower is better)
	 4 threads:     3.915         2.560       3.946               3.967  μs/access (lower is better)
	 6 threads:     3.884         2.706       3.930               3.978  μs/access (lower is better)
	 8 threads:     3.933         2.757       3.968               4.026  μs/access (lower is better)
	10 threads:     3.929         2.793       3.984               4.011  μs/access (lower is better)

	1 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.048       0.030               0.030  μs/access (lower is better)
	 2 threads:     3.166         2.302       1.776               1.810  μs/access (lower is better)
	 4 threads:     3.867         2.865       1.971               1.992  μs/access (lower is better)
	 6 threads:     3.878         2.860       1.970               2.002  μs/access (lower is better)
	 8 threads:     3.978         2.881       2.011               2.073  μs/access (lower is better)
	10 threads:     3.938         2.833       2.018               2.006  μs/access (lower is better)

	2 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.048       0.024               0.024  μs/access (lower is better)
	 2 threads:     3.444         1.756       1.185               1.203  μs/access (lower is better)
	 4 threads:     3.863         1.959       1.321               1.331  μs/access (lower is better)
	 6 threads:     3.903         2.474       1.325               1.337  μs/access (lower is better)
	 8 threads:     3.918         2.688       1.332               1.337  μs/access (lower is better)
	10 threads:     3.921         2.665       1.333               1.346  μs/access (lower is better)

	5 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.048       0.018               0.018  μs/access (lower is better)
	 2 threads:     3.482         0.987       0.600               0.609  μs/access (lower is better)
	 4 threads:     3.845         1.205       0.694               0.673  μs/access (lower is better)
	 6 threads:     3.984         2.275       0.697               0.691  μs/access (lower is better)
	 8 threads:     4.253         3.345       0.714               0.711  μs/access (lower is better)
	10 threads:     4.061         3.360       0.691               0.682  μs/access (lower is better)

	10 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.049       0.016               0.016  μs/access (lower is better)
	 2 threads:     2.935         0.575       0.333               0.336  μs/access (lower is better)
	 4 threads:     3.866         0.817       0.379               0.374  μs/access (lower is better)
	 6 threads:     3.874         1.611       0.380               0.379  μs/access (lower is better)
	 8 threads:     3.956         2.044       0.371               0.372  μs/access (lower is better)
	10 threads:     3.953         2.238       0.370               0.371  μs/access (lower is better)

	100 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.048       0.013               0.013  μs/access (lower is better)
	 2 threads:     3.113         0.116       0.041               0.042  μs/access (lower is better)
	 4 threads:     3.886         0.220       0.042               0.042  μs/access (lower is better)
	 6 threads:     3.893         0.276       0.042               0.042  μs/access (lower is better)
	 8 threads:     3.923         0.323       0.042               0.042  μs/access (lower is better)
	10 threads:     3.952         0.327       0.042               0.042  μs/access (lower is better)

	1000 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.048       0.013               0.013  μs/access (lower is better)
	 2 threads:     3.262         0.131       0.050               0.056  μs/access (lower is better)
	 4 threads:     3.890         0.153       0.048               0.058  μs/access (lower is better)
	 6 threads:     3.882         0.180       0.047               0.068  μs/access (lower is better)
	 8 threads:     3.904         0.203       0.046               0.073  μs/access (lower is better)
	10 threads:     3.914         0.201       0.047               0.066  μs/access (lower is better)

	100000 reads per write:
	           std::mutex  std::shared_mutex  FastReadWriteMutex  SlowReadWriteMutex
	 1 threads:     0.023         0.049       0.013               0.013  μs/access (lower is better)
	 2 threads:     2.989         0.121       0.054               0.055  μs/access (lower is better)
	 4 threads:     3.857         0.151       0.054               0.054  μs/access (lower is better)
	 6 threads:     3.896         0.172       0.056               0.060  μs/access (lower is better)
	 8 threads:     3.962         0.185       0.053               0.059  μs/access (lower is better)
	10 threads:     3.956         0.162       0.050               0.052  μs/access (lower is better)

*/
