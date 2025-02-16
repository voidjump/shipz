#include "timer.h"

uint64_t Timer::delta = 0;
uint64_t Timer::previous_tick_time = 0;
uint64_t Timer::current_tick_time = 0;
std::list<Timer*> Timer::instances;
