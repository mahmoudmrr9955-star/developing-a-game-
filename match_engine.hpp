#pragma once
#include <string>
#include <vector>
#include "club.hpp"

struct MatchEvent {
    int minute;
    std::string type;       // "goal" | "yellow" | "red"
    std::string team;       // "home" | "away"
    std::string playerName;
};

struct MatchResult {
    int homeGoals = 0;
    int awayGoals = 0;
    std::vector<MatchEvent> events; // chronological, sorted by minute
};

// Picks a plausible scorer/carder biased toward attackers, mirroring the
// weighted-by-position selection used in the web version's engine.
Player* weightedPlayer(std::vector<Player*>& xi);

// Minute-by-minute simulation (1..90) driven by each side's effective attack
// and defence (mentality/press applied). Deterministic in structure, random
// in outcome — same model as the browser game, ported to C++.
MatchResult simulateMatch(Club& home, Club& away);
