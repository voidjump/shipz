#ifndef SHIPZ_TEAM_H
#define SHIPZ_TEAM_H

// #include "player.h"


// teams/bases ownership:
enum SHIPZ_TEAM {
    NEUTRAL,
    RED,
    BLUE,
};

class Team
{
    public:
        int players; // number of players on the Team
        int bases; // number of bases a team has 
        int frags;

    Team();
};	

extern Team red_team;
extern Team blue_team;

#endif