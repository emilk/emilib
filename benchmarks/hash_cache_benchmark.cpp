#include <limits>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include <emilib/hash_cache.hpp>
#include <emilib/hash_set.hpp>
#include <emilib/timer.cpp>
#include <emilib/strprintf.cpp>

#define LOGURU_IMPLEMENTATION 1
#include <loguru.hpp>

std::vector<size_t> integerKeys()
{
    std::mt19937_64 rd(0);

	std::vector<size_t> numbers;
	for (size_t i = 0; i < 1000000; ++i) {
		numbers.push_back(rd());
	}

	return numbers;
}

std::vector<std::string> generateKeys(
    const std::string& prefix, const std::vector<size_t>& integer_keys, const std::string& suffix)
{
	std::vector<std::string> results;
	results.reserve(integer_keys.size());
	for (const auto& int_key : integer_keys) {
		results.emplace_back(prefix + std::to_string(int_key) + suffix);
		// results.emplace_back(prefix + std::string((const char*)&int_key, sizeof(int_key)) + suffix);
	}
	return results;
}

template<typename HashSet, typename Keys>
double time_once(const Keys& keys)
{
	emilib::Timer timer;
	HashSet set;

	// Add keys individually to force rehashing every now and then:
	for (const auto& key : keys) {
		set.emplace(key);
	}

	return timer.secs();
}

template<typename HashSet, typename Keys>
double best_of_many(const Keys& keys)
{
	double best_time = std::numeric_limits<double>::infinity();
	for (size_t i = 0; i < 10; ++i) {
		best_time = std::min(best_time, time_once<HashSet>(keys));
	}
	return best_time;
}

int main()
{
	using namespace std;
	using namespace emilib;

	const auto integer_keys  = integerKeys();
	const auto short_keys    = generateKeys("", integer_keys, "");
	const auto long_prefix   = generateKeys(std::string(81, 'x'), integer_keys, "");
	const auto long_suffix   = generateKeys("", integer_keys, std::string(81, 'x'));

	printf("\nInteger keys (e.g. %lu):\n", integer_keys[0]);
	printf("unordered_set<size_t>:            %5.0f ms\n", 1e3 * best_of_many<unordered_set<size_t>>(integer_keys));
	printf("unordered_set<HashCache<size_t>>: %5.0f ms\n", 1e3 * best_of_many<unordered_set<HashCache<size_t>>>(integer_keys));
	printf("HashSet<size_t>:                  %5.0f ms\n", 1e3 * best_of_many<HashSet<size_t>>(integer_keys));
	printf("HashSet<HashCache<size_t>>:       %5.0f ms\n", 1e3 * best_of_many<HashSet<HashCache<size_t>>>(integer_keys));

	printf("\nShort keys (e.g. \"%s\"):\n", short_keys[0].c_str());
	printf("unordered_set<string>:            %5.0f ms\n", 1e3 * best_of_many<unordered_set<string>>(short_keys));
	printf("unordered_set<HashCache<string>>: %5.0f ms\n", 1e3 * best_of_many<unordered_set<HashCache<string>>>(short_keys));
	printf("HashSet<string>:                  %5.0f ms\n", 1e3 * best_of_many<HashSet<string>>(short_keys));
	printf("HashSet<HashCache<string>>:       %5.0f ms\n", 1e3 * best_of_many<HashSet<HashCache<string>>>(short_keys));

	printf("\nLong suffixes (e.g. \"%s\"):\n", long_suffix[0].c_str());
	printf("unordered_set<string>:            %5.0f ms\n", 1e3 * best_of_many<unordered_set<string>>(long_suffix));
	printf("unordered_set<HashCache<string>>: %5.0f ms\n", 1e3 * best_of_many<unordered_set<HashCache<string>>>(long_suffix));
	printf("HashSet<string>:                  %5.0f ms\n", 1e3 * best_of_many<HashSet<string>>(long_suffix));
	printf("HashSet<HashCache<string>>:       %5.0f ms\n", 1e3 * best_of_many<HashSet<HashCache<string>>>(long_suffix));

	printf("\nLong prefixes (e.g. \"%s\"):\n", long_prefix[0].c_str());
	printf("unordered_set<string>:            %5.0f ms\n", 1e3 * best_of_many<unordered_set<string>>(long_prefix));
	printf("unordered_set<HashCache<string>>: %5.0f ms\n", 1e3 * best_of_many<unordered_set<HashCache<string>>>(long_prefix));
	printf("HashSet<string>:                  %5.0f ms\n", 1e3 * best_of_many<HashSet<string>>(long_prefix));
	printf("HashSet<HashCache<string>>:       %5.0f ms\n", 1e3 * best_of_many<HashSet<HashCache<string>>>(long_prefix));
}
