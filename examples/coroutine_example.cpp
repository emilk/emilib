#include <emilib/coroutine.cpp>
#include <emilib/timer.cpp>

// emilib::cr uses loguru.
// This example also uses Loguru to show off which thread each line is printed from.
#define LOGURU_IMPLEMENTATION 1
#include <loguru.hpp>

#include <cstdio>

namespace cr = emilib::cr;

double kSecondsBetweenLines = 1.0;

int main(int argc, char* argv[])
{
	loguru::init(argc, argv); // Just called to get a proper thread name for main thread.

	cr::CoroutineSet coroutine_set;

	// Common state:
	bool time_for_captain_to_show = false;
	bool captain_has_been_painted = false;
	bool bomb_has_ben_set_up      = false;

	// A coroutine for writing a script in a timely fashion:
	auto text_cr = coroutine_set.start("intro_text", [&](cr::InnerControl& ic){
		LOG_F(INFO, "In A.D. 2101");
		ic.wait_sec(kSecondsBetweenLines);
		LOG_F(INFO, "War was beginning.");
		ic.wait_sec(kSecondsBetweenLines);
		time_for_captain_to_show = true;
		ic.wait_for([&](){ return captain_has_been_painted; });
		LOG_F(INFO, "Captain: What happen?");
		ic.wait_sec(kSecondsBetweenLines);
		LOG_F(INFO, "Mechanic: Somebody set up us the bomb.");
		bomb_has_ben_set_up = true;
		ic.wait_sec(kSecondsBetweenLines);
		LOG_F(INFO, "Operator: We get signal.");
		ic.wait_sec(kSecondsBetweenLines);
		LOG_F(INFO, "Captain: What !");
	});

	// Start up second (unnecessary) coroutine for demonstrative purposes:
	coroutine_set.start("intro_graphics", [&](cr::InnerControl& ic){
		ic.wait_for([&](){ return time_for_captain_to_show; });
		LOG_F(INFO, "[INSERT CAPTAIN DRAWING HERE]");
		captain_has_been_painted = true;
	});

	emilib::Timer frame_timer;

	while (!coroutine_set.empty()) {
		double dt = frame_timer.reset();
		coroutine_set.poll(dt); // Run all coroutines

		if (bomb_has_ben_set_up && text_cr) {
			LOG_F(INFO, "(Aborting early to demonstrate how)");
			text_cr->stop();
			text_cr = nullptr;
		}
	}
}
