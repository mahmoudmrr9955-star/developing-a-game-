#pragma once
#include <string>
#include <vector>
#include <map>
#include "player.hpp"

// Formation -> ordered list of position bands (GK first, then DF, MF, FW).
// Slot order matters for display (roleLabels groups by consecutive band).
extern const std::map<std::string, std::vector<Position>> FORMATIONS;
extern const std::vector<std::string> FORMATION_NAMES; // stable iteration order

struct Club {
    std::string name;
    int colorId = 0;
    std::vector<Player> squad;
    std::string formation = "4-3-3";
    std::vector<int> lineup;      // 11 player ids, in FORMATIONS[formation] slot order
    std::string mentality = "bal"; // "def" | "bal" | "off"
    std::string press = "bal";     // "con" | "bal" | "agg"
    double teamwork = 640.0;       // 400-999, squad cohesion
    double glory = 0.0;            // prestige, accumulates with results
    int coins = 60;                // cosmetic premium currency, mirrors the web version
};

struct Forces { double att; double def; };

// Fills club.squad with a generated 18-player roster (2 GK, 6 DF, 7 MF, 3 FW).
std::vector<Player> genSquad(int& nextId, int baseOvr, std::set<std::string>& usedNames);

// Picks the strongest available player per slot for the current formation.
void autoLineup(Club& club);

// Repairs club.lineup after squad changes (sold/transferred players, etc.)
// or a formation switch, keeping valid picks and filling gaps by rating.
void fixLineup(Club& club);

// Returns the 11 Player* currently in the lineup (skips missing ids safely).
std::vector<Player*> starters(Club& club);

// Average line strengths for the current XI. If `full` is true, stamina is
// ignored (used for the OTR "ceiling" stat); otherwise energy discounts it.
Forces lineStrength(Club& club, bool full);

// lineStrength() adjusted for mentality (attacking/balanced/defensive) and
// pressing (cautious/normal/aggressive) trade-offs.
Forces effForces(Club& club, bool full);

// Whole-XI overall rating (simple average of the 11 starters' OVR).
int teamOVR(Club& club);

struct TeamStats { int otr; int otrMax; int attack; int defence; int teamwork; int glory; };
TeamStats teamStats(Club& club);
