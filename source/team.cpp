#include "team.h"

Team red_team;
Team blue_team;

Team::Team() {
    players = 0;
    frags = 0;

    // TODO: this should be a dynamic property?
    bases = 0;
}