#include "timer.h"

float Timer::delta = 0;
float Timer::previous_tick_time = 0;
float Timer::current_tick_time = 0;
std::list<Timer*> Timer::instances;
