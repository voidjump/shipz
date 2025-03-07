#ifndef SHIPZ_TIMER_H
#define SHIPZ_TIMER_H

#include <SDL3/SDL.h>

#include <functional>
#include <list>
#include "log.h"

class Timer {
   private:
    // delta between current tick and last tick in seconds
    static float delta;
    // 'timestamp' of the previous tick in seconds
    static float previous_tick_time;
    // 'timestamp' of the current tick in seconds
    static float current_tick_time;

    static std::list<Timer*> instances;

    std::function<void()> callback = nullptr;

    float fire_time;
    float period;
    bool armed;

    // Execute function
    inline void Fire() {
        if (static_cast<bool>(callback)) {
            callback();
        }
    }

   public:
    // Constructor, no period
    Timer() {
        this->armed = false;
        this->callback = nullptr;
        this->Register();
    }

    // Create a timer that is armed
    Timer(std::function<void()> cb, float frequency, bool periodic) {
        float wait = 1.0/frequency;
        this->armed = true;
        this->fire_time = current_tick_time + wait;
        if(periodic) {
            period = wait;
        }
        callback = std::move(cb);
        this->Register();
    }

    ~Timer() {
        instances.remove(this);
    }

    void Register() {
        instances.push_back(this);
    }

    // Tick all timers, firing any triggered timers.
    inline static float Tick() {
        previous_tick_time = current_tick_time;
        current_tick_time = SDL_GetTicksNS() * 0.000000001;
        delta = current_tick_time - previous_tick_time;
        for (auto timer : instances) {
            if (!timer->armed) {
                continue;
            }
            if (current_tick_time > timer->fire_time) {
                timer->Fire();
                if (timer->period != 0) {
                    timer->Arm(timer->period);
                } else {
                    timer->armed = false;
                }
            }
        }
        return delta;
    }
    // Arm timer for x time in the future
    inline void Arm(float arm_period) {
        armed = true;
        fire_time = arm_period + current_tick_time;
    }

    static inline float LastTick() {
        return delta;
    }
};

#endif