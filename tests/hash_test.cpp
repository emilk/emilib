#include <string>

#include <catch.hpp>

#include <emilib/hash_map.hpp>
#include <emilib/hash_set.hpp>

using namespace std;

TEST_CASE( "[int -> double]", "HashMap" ) {
	emilib::HashMap<int, double> map;
	REQUIRE(map.empty());
	REQUIRE(map.size() == 0);
	map.insert(1, 1.0);
	REQUIRE(map.size() == 1);
	REQUIRE(map[1] == 1.0);
	map.insert(2, 2.0);
	REQUIRE(map.size() == 2);
	REQUIRE(map[2] == 2.0);
	map.insert(3, 3.0);
	REQUIRE(map.size() == 3);
	REQUIRE(map[3] == 3.0);
	map.insert(4, 4.0);
	REQUIRE(map.size() == 4);
	REQUIRE(map[4] == 4.0);
	REQUIRE(map.count(2) == 1);
	bool did_erase = map.erase(2);
	REQUIRE(did_erase);
	REQUIRE(map.count(2) == 0);
	REQUIRE(map.size() == 3);
	REQUIRE(map[4] == 4.0);
}

TEST_CASE( "[string -> string]", "HashMap" ) {
	emilib::HashMap<string, string> map;
	map["1"] = "one";
	map["2"] = "two";
	map["3"] = "three";
	map["4"] = "four";
	map["5"] = "five";
	map["6"] = "six";
	REQUIRE(map["1"] == "one");
	REQUIRE(map["2"] == "two");
	REQUIRE(map["3"] == "three");
	REQUIRE(map["4"] == "four");
	REQUIRE(map["5"] == "five");
	REQUIRE(map["6"] == "six");
}

TEST_CASE( "copy+moving", "HashMap" ) {
	emilib::HashMap<string, string> map;
	map["1"] = "one";
	map["2"] = "two";

	{
		auto copy = map;
		REQUIRE(copy.size() == 2);
		REQUIRE(copy["1"] == "one");
		REQUIRE(copy["2"] == "two");

		copy["3"] = "three";
		REQUIRE(copy.size() == 3);
		REQUIRE(copy["1"] == "one");
		REQUIRE(copy["2"] == "two");
		REQUIRE(copy["3"] == "three");

		REQUIRE(map.size() == 2);
		REQUIRE(map["1"] == "one");
		REQUIRE(map["2"] == "two");

		map = copy;
		REQUIRE(map.size() == 3);
		REQUIRE(map["1"] == "one");
		REQUIRE(map["2"] == "two");
		REQUIRE(map["3"] == "three");

		REQUIRE(copy.size() == 3);
		REQUIRE(copy["1"] == "one");
		REQUIRE(copy["2"] == "two");
		REQUIRE(copy["3"] == "three");
	}

	{
		auto moved = std::move(map);
		REQUIRE(moved.size() == 3);
		REQUIRE(moved["1"] == "one");
		REQUIRE(moved["2"] == "two");
		REQUIRE(moved["3"] == "three");
	}

	REQUIRE(map.empty());
}

TEST_CASE( "[string]", "HashSet" ) {
	emilib::HashSet<string> set;
	set.insert("1");
	set.insert("2");
	set.insert("3");
	REQUIRE(set.count("0") == 0);
	REQUIRE(set.count("1") == 1);
	REQUIRE(set.count("2") == 1);
	REQUIRE(set.count("3") == 1);
	REQUIRE(set.size() == 3);
	set.insert("4");
	set.insert("5");
	set.insert("6");
	REQUIRE(set.count("1") == 1);
	REQUIRE(set.count("2") == 1);
	REQUIRE(set.count("3") == 1);
	REQUIRE(set.count("4") == 1);
	REQUIRE(set.count("5") == 1);
	REQUIRE(set.count("6") == 1);
	REQUIRE(set.size() == 6);
	REQUIRE(set.count("2") == 1);
	set.erase("2");
	REQUIRE(set.size() == 5);
	REQUIRE(set.count("2") == 0);
}
