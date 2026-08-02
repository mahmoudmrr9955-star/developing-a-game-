#pragma once
#include <string>
#include <vector>
#include <set>
#include "club.hpp"
#include "league.hpp"
#include "match_engine.hpp"

// One completed fixture involving the user's club, kept for the "recent
// results" list and the season schedule view.
struct DisplayResult {
    int matchday = 0;
    std::string opponent;
    bool home = false;
    int gf = 0, ga = 0;
    char res = 'D'; // 'W' | 'D' | 'L'
};

// A resolved fixture for a given matchday: which two clubs play, and
// whether the user's club is the home side. `valid` is false once the
// 14-matchday season has been fully played.
struct Fixture {
    int homeIdx = -1;
    int awayIdx = -1;
    bool userHome = false;
    bool valid = false;
};

// Orchestrates the whole console game: state, menus, season flow, and
// save/load. main() only needs to construct one and call run().
class Game {
public:
    void run();

private:
    // --- persistent state ---
    int nextId = 1;
    std::set<std::string> usedNames;
    int season = 1;
    int matchday = 0; // 0-based index of the next fixture to play
    int userIdx = 0;
    double budget = 28.0;
    double seasonEarnings = 0.0;
    int trainCharges = 1;
    std::vector<Club> teams;
    std::vector<StandingsRow> table;
    std::vector<std::vector<std::pair<int,int>>> schedule;
    std::vector<Player> market;
    std::vector<DisplayResult> results;

    // --- setup ---
    void promptNewGame();
    void newGame(const std::string& clubName);
    void generateMarket();

    // --- menu screens ---
    void showDashboard();
    void showSquad();
    void tacticsMenu();
    void printLineupWithRoles(Club& c);
    void swapPlayerMenu();
    void playNextMatch();
    void showTable();
    void showSchedule();
    void transfersMenu();
    void trainingMenu();
    void financesMenu();

    // --- season flow ---
    Fixture nextFixtureFor(int md) const;
    void applyMatchdayResults(const MatchResult& userMatch, const Fixture& userFixture);
    void endSeason();
    void nextSeason(double bonus);

    // --- helpers ---
    Club& userClub();
    std::vector<std::string> teamNamesList() const;

    // --- persistence ---
    bool save(const std::string& path);
    bool load(const std::string& path);
};
