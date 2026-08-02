#pragma once
#include <vector>
#include <utility>
#include <string>

struct StandingsRow {
    int idx = 0;
    int played = 0, won = 0, draw = 0, lost = 0;
    int goalsFor = 0, goalsAgainst = 0, points = 0;
    std::vector<char> form; // 'W'/'D'/'L', most recent last
    int prevPos = 0;        // rank before the last matchday (for movement arrows)
    int pos = 0;            // current rank, 1-based
};

// Double round-robin (circle method): for n teams returns 2*(n-1) rounds,
// each with n/2 (homeIdx, awayIdx) fixtures. Second half mirrors venues.
std::vector<std::vector<std::pair<int,int>>> roundRobin(int n);

// Applies a result to both teams' rows: points, goal difference, form.
void recordResult(StandingsRow& homeRow, StandingsRow& awayRow, int homeGoals, int awayGoals);

// Returns table sorted by points, then goal difference, then goals for.
std::vector<StandingsRow> standings(const std::vector<StandingsRow>& table,
                                     const std::vector<std::string>& teamNames);

// Refreshes pos/prevPos on the raw table (call after each matchday).
void updateRanks(std::vector<StandingsRow>& table, const std::vector<std::string>& teamNames);
