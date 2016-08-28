#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include <emilib/hash_cache.hpp>
#include <emilib/hash_set.hpp>
#include <emilib/timer.cpp>

#define LOGURU_IMPLEMENTATION 1
#include <loguru.hpp>

std::vector<std::string> generateKeys(const std::string& prefix, const std::string& suffix)
{
	std::vector<std::string> results;

	for (char a='a'; a<='z'; ++a) {
		for (char b='a'; b<='z'; ++b) {
			for (char c='a'; c<='z'; ++c) {
				for (char d='a'; d<='z'; ++d) {
					std::string str;
					str.reserve(prefix.size() + 4 + suffix.size());
					str += prefix;
					str += a;
					str += b;
					str += c;
					str += d;
					str += suffix;
					results.emplace_back(std::move(str));
				}
			}
		}
	}

	return results;
}

template<typename HashSet>
double time_once(const std::vector<std::string>& keys)
{
	emilib::Timer timer;
	HashSet set;

	// Add keys individually to force rehashing every now and then:
	for (const auto& key : keys) {
		set.emplace(key);
	}

	return timer.secs();
}

template<typename HashSet>
double time_best_of_five(const std::vector<std::string>& keys)
{
	double best_time = std::numeric_limits<double>::infinity();
	for (size_t i = 0; i < 5; ++i) {
		best_time = std::min(best_time, time_once<HashSet>(keys));
	}
	return best_time;
}

int main()
{
	using namespace std;
	using namespace emilib;

	std::vector<std::string> short_keys  = generateKeys("", "");
	std::vector<std::string> long_prefix = generateKeys(std::string(64, 'x'), "");
	std::vector<std::string> long_suffix = generateKeys("", std::string(64, 'x'));

	printf("\nShort keys:\n");
	printf("unordered_set<string>:            %5.3f s\n", time_best_of_five<unordered_set<string>>(short_keys));
	printf("unordered_set<HashCache<string>>: %5.3f s\n", time_best_of_five<unordered_set<HashCache<string>>>(short_keys));
	printf("HashSet<string>:                  %5.3f s\n", time_best_of_five<HashSet<string>>(short_keys));
	printf("HashSet<HashCache<string>>:       %5.3f s\n", time_best_of_five<HashSet<HashCache<string>>>(short_keys));

	printf("\nLong prefixes:\n");
	printf("unordered_set<string>:            %5.3f s\n", time_best_of_five<unordered_set<string>>(long_prefix));
	printf("unordered_set<HashCache<string>>: %5.3f s\n", time_best_of_five<unordered_set<HashCache<string>>>(long_prefix));
	printf("HashSet<string>:                  %5.3f s\n", time_best_of_five<HashSet<string>>(long_prefix));
	printf("HashSet<HashCache<string>>:       %5.3f s\n", time_best_of_five<HashSet<HashCache<string>>>(long_prefix));

	printf("\nLong suffxes:\n");
	printf("unordered_set<string>:            %5.3f s\n", time_best_of_five<unordered_set<string>>(long_suffix));
	printf("unordered_set<HashCache<string>>: %5.3f s\n", time_best_of_five<unordered_set<HashCache<string>>>(long_suffix));
	printf("HashSet<string>:                  %5.3f s\n", time_best_of_five<HashSet<string>>(long_suffix));
	printf("HashSet<HashCache<string>>:       %5.3f s\n", time_best_of_five<HashSet<HashCache<string>>>(long_suffix));
}
