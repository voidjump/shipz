#ifndef SHIPZ_TEAM_H
#define SHIPZ_TEAM_H

#include "base.h"

// teams/bases ownership:
enum SHIPZ_TEAM {
    NEUTRAL,
    RED,
    BLUE,
};


class GameState {
    public:
        static uint16_t blue_bases;
        static uint16_t red_bases;
        static uint16_t neutral_bases;

        static void Update() {
            blue_bases = Base::GetTeamCount(blue_bases);
            red_bases = Base::GetTeamCount(red_bases);
            neutral_bases = Base::GetTeamCount(neutral_bases);
        }
};


#endif