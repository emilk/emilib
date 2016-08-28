#include <emilib/coroutine.cpp>
#include <emilib/timer.cpp>

// emilib::cr uses loguru:
#define LOGURU_IMPLEMENTATION 1
#include <loguru.hpp>

#include <cstdio>

namespace cr = emilib::cr;

double kSpeedup = 2; // Speed up execution to show that you can =)
double kSecondsBetweenLines = 2.0; // This will be divided by kSpeedup.

int main()
{
	cr::CoroutineSet coroutine_set;

	// Common state:
	bool time_for_captain_to_show = false;
	bool captain_has_been_painted = false;
	bool bomb_has_ben_set_up      = false;

	// A coroutine for writing a script in a timely fashion:
	auto text_cr = coroutine_set.start("intro_text", [&](cr::InnerControl& ic){
		printf("In A.D. 2101\n");
		ic.wait_sec(kSecondsBetweenLines);
		printf("War was beginning.\n");
		ic.wait_sec(kSecondsBetweenLines);
		time_for_captain_to_show = true;
		ic.wait_for([&](){ return captain_has_been_painted; });
		printf("Captain: What happen?\n");
		ic.wait_sec(kSecondsBetweenLines);
		printf("Mechanic: Somebody set up us the bomb.\n");
		bomb_has_ben_set_up = true;
		ic.wait_sec(kSecondsBetweenLines);
		printf("Operator: We get signal.\n");
		ic.wait_sec(kSecondsBetweenLines);
		printf("Captain: What !\n");
	});

	// Start up second (unnecessary) coroutine for demonstrative purposes:
	coroutine_set.start("intro_graphics", [&](cr::InnerControl& ic){
		ic.wait_for([&](){ return time_for_captain_to_show; });
		printf("[INSERT CAPTAIN DRAWING HERE]\n");
		captain_has_been_painted = true;
	});

	emilib::Timer frame_timer;

	while (!coroutine_set.empty()) {
		double dt = frame_timer.reset();

		dt *= kSpeedup;

		// Allow coroutines to execute:
		coroutine_set.poll(dt);

		if (bomb_has_ben_set_up && text_cr) {
			printf("(Aborting early to demonstrate how)\n");
			text_cr->stop();
			text_cr = nullptr;
		}
	}
}
