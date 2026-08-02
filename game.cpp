#include "game.hpp"
#include "common.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace {
// Thrown by askInt/askLine when stdin runs dry (EOF), so an automated or
// piped session ends gracefully (auto-save + exit) instead of looping.
struct EndOfInput {};

int askInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) throw EndOfInput{};
        try {
            size_t consumed = 0;
            int v = std::stoi(line, &consumed);
            if (v >= lo && v <= hi) return v;
        } catch (...) {}
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

std::string askLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw EndOfInput{};
    return line;
}
} // namespace

Club& Game::userClub() { return teams[userIdx]; }

std::vector<std::string> Game::teamNamesList() const {
    std::vector<std::string> names;
    names.reserve(teams.size());
    for (auto& t : teams) names.push_back(t.name);
    return names;
}

void Game::generateMarket() {
    market.clear();
    struct Plan { Position pos; int n; };
    const Plan plan[] = {{Position::GK, 1}, {Position::DF, 4}, {Position::MF, 5}, {Position::FW, 4}};
    for (auto& pl : plan)
        for (int i = 0; i < pl.n; ++i)
            market.push_back(genPlayer(nextId, pl.pos, util::randInt(62, 84), usedNames));
    std::sort(market.begin(), market.end(), [](const Player& a, const Player& b) { return a.ovr > b.ovr; });
}

void Game::newGame(const std::string& clubName) {
    nextId = 1; usedNames.clear(); teams.clear(); table.clear(); results.clear();
    season = 1; matchday = 0; userIdx = 0; budget = 28.0; seasonEarnings = 0.0; trainCharges = 1;

    const int bases[8] = {78, 75, 73, 71, 69, 67, 65, 62};
    const int userBase = 67;
    std::vector<std::string> clubNames = {
        "AS Velocite", "Real Aurora", "FC Tempete", "Inter Meridian", "Atletico Solaris",
        "United Phenix", "Sporting Maree", "Olympique Lumiere", "Dynamo Boreal"
    };

    Club user;
    user.name = clubName.empty() ? "FC Etoile" : clubName;
    user.colorId = 0;
    user.squad = genSquad(nextId, userBase, usedNames);
    user.formation = "4-3-3"; user.mentality = "bal"; user.press = "bal";
    user.teamwork = 660; user.glory = 0; user.coins = 65;
    autoLineup(user);
    teams.push_back(user);

    std::shuffle(clubNames.begin(), clubNames.end(), util::rng());
    std::vector<int> aiBases;
    for (int b : bases) if (b != userBase) aiBases.push_back(b);
    for (int i = 0; i < 7; ++i) {
        Club c;
        c.name = clubNames[i];
        c.colorId = i + 1;
        c.squad = genSquad(nextId, aiBases[i], usedNames);
        c.formation = FORMATION_NAMES[util::randInt(0, (int)FORMATION_NAMES.size() - 1)];
        c.mentality = "bal"; c.press = "bal";
        c.teamwork = util::randInt(600, 720);
        c.glory = util::randInt(0, 30);
        c.coins = 60;
        autoLineup(c);
        teams.push_back(c);
    }

    for (int i = 0; i < (int)teams.size(); ++i) { StandingsRow r; r.idx = i; table.push_back(r); }
    schedule = roundRobin((int)teams.size());
    generateMarket();
    updateRanks(table, teamNamesList());
}

void Game::promptNewGame() {
    std::string name = askLine("Name your club (Enter for \"FC Etoile\"): ");
    newGame(name);
    std::cout << "\nWelcome to " << userClub().name << "! Fictional clubs and players only.\n";
}

Fixture Game::nextFixtureFor(int md) const {
    Fixture fx;
    if (md < 0 || md >= (int)schedule.size()) return fx;
    for (auto& pair : schedule[md]) {
        if (pair.first == userIdx || pair.second == userIdx) {
            fx.homeIdx = pair.first;
            fx.awayIdx = pair.second;
            fx.userHome = (pair.first == userIdx);
            fx.valid = true;
            return fx;
        }
    }
    return fx;
}

void Game::showDashboard() {
    Club& c = userClub();
    auto ranked = standings(table, teamNamesList());
    int pos = 0;
    for (size_t i = 0; i < ranked.size(); ++i) if (ranked[i].idx == userIdx) { pos = (int)i + 1; break; }
    TeamStats st = teamStats(c);
    double avgEnergy = 0;
    for (auto& p : c.squad) avgEnergy += p.stamina;
    if (!c.squad.empty()) avgEnergy /= c.squad.size();

    std::cout << "\n== " << c.name << " ==\n";
    std::cout << "Position: " << pos << "/" << teams.size()
               << "   Budget: " << util::money(budget)
               << "   Coins: " << c.coins << "\n";
    std::cout << "OTR " << st.otr << "/" << st.otrMax
               << "   Attack " << st.attack << "   Defence " << st.defence
               << "   Teamwork " << st.teamwork << "   Glory " << st.glory << "\n";
    std::cout << "Squad avg energy: " << (int)std::round(avgEnergy) << "%\n";

    Fixture fx = nextFixtureFor(matchday);
    if (fx.valid) {
        Club& opp = teams[fx.userHome ? fx.awayIdx : fx.homeIdx];
        std::cout << "Next: Matchday " << (matchday + 1) << " vs " << opp.name
                   << (fx.userHome ? " (Home)" : " (Away)") << "\n";
    } else {
        std::cout << "Season complete -- check the League table to close it out.\n";
    }

    if (!results.empty()) {
        std::cout << "Recent results:\n";
        int shown = 0;
        for (auto it = results.rbegin(); it != results.rend() && shown < 5; ++it, ++shown) {
            std::cout << "  MD" << it->matchday << " " << it->res << "  vs " << it->opponent
                       << " (" << (it->home ? "H" : "A") << ")  " << it->gf << "-" << it->ga << "\n";
        }
    }
}

void Game::showSquad() {
    Club& c = userClub();
    std::cout << "\n== Squad: " << c.name << " (" << c.squad.size() << " players) ==\n";
    std::cout << std::left << std::setw(4) << "Pos" << std::setw(22) << "Name" << std::setw(5) << "Age"
               << std::setw(5) << "OVR" << std::setw(9) << "Energy" << std::setw(9) << "Value" << "XI\n";

    std::vector<Player> sorted = c.squad;
    std::sort(sorted.begin(), sorted.end(), [](const Player& a, const Player& b) {
        if (a.pos != b.pos) return (int)a.pos < (int)b.pos;
        return a.ovr > b.ovr;
    });
    for (auto& p : sorted) {
        bool inXI = std::find(c.lineup.begin(), c.lineup.end(), p.id) != c.lineup.end();
        std::cout << std::left << std::setw(4) << posToString(p.pos) << std::setw(22) << p.name
                   << std::setw(5) << p.age << std::setw(5) << p.ovr
                   << std::setw(9) << (std::to_string(p.stamina) + "%")
                   << std::setw(9) << util::money(p.value) << (inXI ? "*" : "") << "\n";
    }
}

void Game::printLineupWithRoles(Club& c) {
    auto it = FORMATIONS.find(c.formation);
    const std::vector<Position>& slots = (it != FORMATIONS.end()) ? it->second : FORMATIONS.at("4-3-3");
    std::vector<std::string> labels(slots.size());

    size_t i = 0;
    while (i < slots.size()) {
        size_t j = i;
        while (j < slots.size() && slots[j] == slots[i]) ++j;
        auto lbls = roleLabels(slots[i], (int)(j - i));
        for (size_t k = i; k < j; ++k) labels[k] = lbls[k - i];
        i = j;
    }
    for (size_t s = 0; s < slots.size(); ++s) {
        int id = (s < c.lineup.size()) ? c.lineup[s] : -1;
        std::string nm = "-"; int ovr = 0;
        for (auto& p : c.squad) if (p.id == id) { nm = p.name; ovr = p.ovr; break; }
        std::cout << "  " << std::left << std::setw(5) << labels[s] << nm << " (OVR " << ovr << ")\n";
    }
}

void Game::swapPlayerMenu() {
    Club& c = userClub();
    auto it = FORMATIONS.find(c.formation);
    const std::vector<Position>& slots = (it != FORMATIONS.end()) ? it->second : FORMATIONS.at("4-3-3");

    std::cout << "Pick a slot to change (1-" << slots.size() << "):\n";
    for (size_t i = 0; i < slots.size(); ++i) {
        int id = (i < c.lineup.size()) ? c.lineup[i] : -1;
        std::string nm = "-";
        for (auto& p : c.squad) if (p.id == id) { nm = p.name; break; }
        std::cout << "  " << (i + 1) << ") " << posToString(slots[i]) << " - " << nm << "\n";
    }
    int slot = askInt("> ", 1, (int)slots.size()) - 1;
    Position want = slots[slot];

    std::vector<Player*> bench;
    for (auto& p : c.squad) {
        bool used = std::find(c.lineup.begin(), c.lineup.end(), p.id) != c.lineup.end();
        if (!used && p.pos == want) bench.push_back(&p);
    }
    std::sort(bench.begin(), bench.end(), [](Player* a, Player* b) { return a->ovr > b->ovr; });
    if (bench.empty()) { std::cout << "No available " << posToString(want) << " on the bench.\n"; return; }

    std::cout << "Available " << posToString(want) << ":\n";
    for (size_t i = 0; i < bench.size(); ++i)
        std::cout << "  " << (i + 1) << ") " << bench[i]->name << " (OVR " << bench[i]->ovr << ")\n";
    int pick = askInt("> ", 1, (int)bench.size()) - 1;
    c.lineup[slot] = bench[pick]->id;
    std::cout << "Lineup updated.\n";
}

void Game::tacticsMenu() {
    Club& c = userClub();
    bool inMenu = true;
    while (inMenu) {
        fixLineup(c);
        TeamStats st = teamStats(c);
        std::cout << "\n== Tactics: " << c.name << " ==\n";
        std::cout << "Formation: " << c.formation << "   Mentality: " << c.mentality
                   << "   Press: " << c.press << "\n";
        std::cout << "OTR " << st.otr << "/" << st.otrMax << "  Attack " << st.attack
                   << "  Defence " << st.defence << "  Teamwork " << st.teamwork << "\n";
        std::cout << "Starting XI:\n";
        printLineupWithRoles(c);

        std::cout << "\n1) Change formation\n2) Change mentality\n3) Change press\n"
                     "4) Auto-pick best XI\n5) Swap a player\n0) Back\n";
        int ch = askInt("> ", 0, 5);
        switch (ch) {
            case 1: {
                std::cout << "Formations: ";
                for (size_t i = 0; i < FORMATION_NAMES.size(); ++i)
                    std::cout << (i + 1) << ")" << FORMATION_NAMES[i] << "  ";
                std::cout << "\n";
                int f = askInt("> ", 1, (int)FORMATION_NAMES.size());
                c.formation = FORMATION_NAMES[f - 1];
                autoLineup(c);
                break;
            }
            case 2: {
                int m = askInt("1) Defensive  2) Balanced  3) Offensive\n> ", 1, 3);
                c.mentality = (m == 1 ? "def" : m == 2 ? "bal" : "off");
                break;
            }
            case 3: {
                int pr = askInt("1) Cautious  2) Normal  3) Aggressive\n> ", 1, 3);
                c.press = (pr == 1 ? "con" : pr == 2 ? "bal" : "agg");
                break;
            }
            case 4:
                autoLineup(c);
                std::cout << "Best available XI selected.\n";
                break;
            case 5:
                swapPlayerMenu();
                break;
            case 0:
                inMenu = false;
                break;
        }
    }
}

void Game::applyMatchdayResults(const MatchResult& userMatch, const Fixture& userFixture) {
    for (auto& fx : schedule[matchday]) {
        int hg, ag;
        if (fx.first == userFixture.homeIdx && fx.second == userFixture.awayIdx) {
            hg = userMatch.homeGoals; ag = userMatch.awayGoals;
        } else {
            MatchResult sim = simulateMatch(teams[fx.first], teams[fx.second]);
            hg = sim.homeGoals; ag = sim.awayGoals;
        }
        StandingsRow* homeRow = nullptr; StandingsRow* awayRow = nullptr;
        for (auto& r : table) {
            if (r.idx == fx.first) homeRow = &r;
            if (r.idx == fx.second) awayRow = &r;
        }
        recordResult(*homeRow, *awayRow, hg, ag);

        if (fx.first == userIdx || fx.second == userIdx) {
            bool wasHome = (fx.first == userIdx);
            int gf = wasHome ? hg : ag, ga = wasHome ? ag : hg;
            char res = gf > ga ? 'W' : gf < ga ? 'L' : 'D';
            Club& opp = teams[wasHome ? fx.second : fx.first];
            results.push_back({matchday + 1, opp.name, wasHome, gf, ga, res});
        }
    }
    updateRanks(table, teamNamesList());

    for (auto& t : teams) {
        for (auto& p : t.squad) {
            bool inXI = std::find(t.lineup.begin(), t.lineup.end(), p.id) != t.lineup.end();
            int delta = inXI ? -util::randInt(10, 20) : util::randInt(8, 18);
            p.stamina = util::clampI(p.stamina + delta, 5, 100);
        }
    }

    if (!results.empty()) {
        auto& last = results.back();
        double prize = last.res == 'W' ? 2.4 : last.res == 'D' ? 1.0 : 0.4;
        budget = std::round((budget + prize + 0.4) * 10) / 10;
        seasonEarnings = std::round((seasonEarnings + prize + 0.4) * 10) / 10;
        Club& uc = userClub();
        uc.teamwork = util::clampD(uc.teamwork + 6, 480, 960);
        if (last.res == 'W') { uc.coins += 2; uc.glory += 3; }
        else if (last.res == 'D') { uc.glory += 1; }
    }

    matchday++;
    trainCharges = 1;
    if (matchday % 3 == 0) generateMarket();

    if (matchday >= 14) std::cout << "\nThe season is over! Head to the League table to close it out.\n";
}

void Game::playNextMatch() {
    Fixture fx = nextFixtureFor(matchday);
    if (!fx.valid) { std::cout << "No fixture left this season -- check the League table.\n"; return; }

    Club& home = teams[fx.homeIdx];
    Club& away = teams[fx.awayIdx];
    std::cout << "\n== Matchday " << (matchday + 1) << ": " << home.name << " vs " << away.name << " ==\n";
    std::cout << (fx.userHome ? "You are playing at home.\n" : "You are playing away.\n");
    int ch = askInt("1) Kick off  0) Back\n> ", 0, 1);
    if (ch == 0) return;

    MatchResult mr = simulateMatch(home, away);
    std::cout << "\n-- Match report --\n";
    for (auto& ev : mr.events) {
        std::string side = (ev.team == "home") ? home.name : away.name;
        std::string tag = ev.type == "goal" ? "GOAL" : ev.type == "red" ? "RED CARD" : "Yellow card";
        std::cout << "  " << std::setw(3) << ev.minute << "' " << tag << " - " << ev.playerName
                  << " (" << side << ")\n";
    }
    std::cout << "\nFull time: " << home.name << " " << mr.homeGoals << " - " << mr.awayGoals
              << " " << away.name << "\n";

    applyMatchdayResults(mr, fx);
}

void Game::showTable() {
    auto ranked = standings(table, teamNamesList());
    std::cout << "\n== League Table -- Season " << season << " ==\n";
    std::cout << std::left << std::setw(4) << "#" << std::setw(20) << "Club" << std::setw(4) << "P"
               << std::setw(4) << "W" << std::setw(4) << "D" << std::setw(4) << "L"
               << std::setw(6) << "GD" << std::setw(8) << "Form" << "Pts\n";
    for (size_t i = 0; i < ranked.size(); ++i) {
        auto& r = ranked[i];
        std::string mark = (r.idx == userIdx) ? " *" : "";
        int gd = r.goalsFor - r.goalsAgainst;
        std::string form;
        int start = std::max(0, (int)r.form.size() - 5);
        for (int k = start; k < (int)r.form.size(); ++k) form += r.form[k];
        std::cout << std::left << std::setw(4) << (i + 1)
                   << std::setw(20) << (teams[r.idx].name + mark)
                   << std::setw(4) << r.played << std::setw(4) << r.won
                   << std::setw(4) << r.draw << std::setw(4) << r.lost
                   << std::setw(6) << ((gd >= 0 ? "+" : "") + std::to_string(gd))
                   << std::setw(8) << form << r.points << "\n";
    }
    std::cout << "(* = your club)\n";
    if (matchday >= 14) {
        int ch = askInt("\nSeason complete.\n1) End season and continue  0) Back\n> ", 0, 1);
        if (ch == 1) endSeason();
    }
}

void Game::showSchedule() {
    std::cout << "\n== Schedule -- Season " << season << " ==\n";
    for (int md = 0; md < (int)schedule.size(); ++md) {
        for (auto& fx : schedule[md]) {
            if (fx.first == userIdx || fx.second == userIdx) {
                bool home = (fx.first == userIdx);
                Club& opp = teams[home ? fx.second : fx.first];
                std::string scoreStr = "--";
                for (auto& r : results) if (r.matchday == md + 1) { scoreStr = std::to_string(r.gf) + "-" + std::to_string(r.ga); break; }
                std::string marker = (md == matchday && md < 14) ? "  <- next" : "";
                std::cout << " MD" << std::setw(2) << (md + 1) << "  vs " << std::setw(20) << opp.name
                           << (home ? "(H)" : "(A)") << "  " << scoreStr << marker << "\n";
                break;
            }
        }
    }
}

void Game::transfersMenu() {
    bool inMenu = true;
    while (inMenu) {
        Club& c = userClub();
        std::cout << "\n== Transfers ==  Budget: " << util::money(budget) << "\n";
        int ch = askInt("1) View market (buy)\n2) Sell a player\n0) Back\n> ", 0, 2);
        if (ch == 0) { inMenu = false; break; }

        if (ch == 1) {
            if (market.empty()) { std::cout << "The market is empty right now.\n"; continue; }
            for (size_t i = 0; i < market.size(); ++i) {
                auto& p = market[i];
                std::cout << "  " << (i + 1) << ") " << posToString(p.pos) << " " << p.name
                           << " age " << p.age << " OVR " << p.ovr << " - " << util::money(p.value) << "\n";
            }
            int pick = askInt("0) Cancel\n> ", 0, (int)market.size());
            if (pick == 0) continue;
            Player chosen = market[pick - 1];
            if (chosen.value > budget) { std::cout << "Not enough budget.\n"; continue; }
            if (c.squad.size() >= 25) { std::cout << "Squad is full (25 max).\n"; continue; }
            budget = std::round((budget - chosen.value) * 10) / 10;
            market.erase(market.begin() + (pick - 1));
            c.squad.push_back(chosen);
            c.teamwork = util::clampD(c.teamwork - 18, 480, 960);
            fixLineup(c);
            std::cout << "Signed " << chosen.name << "!\n";
        } else if (ch == 2) {
            if (c.squad.size() <= 14) { std::cout << "You must keep at least 14 players.\n"; continue; }
            std::vector<Player> sorted = c.squad;
            std::sort(sorted.begin(), sorted.end(), [](const Player& a, const Player& b) { return a.ovr > b.ovr; });
            for (size_t i = 0; i < sorted.size(); ++i) {
                double sellPrice = std::round(sorted[i].value * 0.9 * 10) / 10;
                std::cout << "  " << (i + 1) << ") " << posToString(sorted[i].pos) << " " << sorted[i].name
                           << " OVR " << sorted[i].ovr << " - sells for " << util::money(sellPrice) << "\n";
            }
            int pick = askInt("0) Cancel\n> ", 0, (int)sorted.size());
            if (pick == 0) continue;
            Player target = sorted[pick - 1];
            double gain = std::round(target.value * 0.9 * 10) / 10;
            budget = std::round((budget + gain) * 10) / 10;
            c.squad.erase(std::remove_if(c.squad.begin(), c.squad.end(),
                          [&](const Player& p) { return p.id == target.id; }), c.squad.end());
            c.teamwork = util::clampD(c.teamwork - 12, 480, 960);
            fixLineup(c);
            std::cout << "Sold " << target.name << " for " << util::money(gain) << ".\n";
        }
    }
}

void Game::trainingMenu() {
    Club& c = userClub();
    std::cout << "\n== Training ==  Sessions left today: " << trainCharges << "\n";
    if (trainCharges <= 0) { std::cout << "No sessions left -- play the next match to get one back.\n"; return; }

    std::vector<Player> sorted = c.squad;
    std::sort(sorted.begin(), sorted.end(), [](const Player& a, const Player& b) { return a.ovr > b.ovr; });
    for (size_t i = 0; i < sorted.size(); ++i) {
        int chance = sorted[i].age <= 21 ? 75 : sorted[i].age <= 25 ? 55 : sorted[i].age <= 29 ? 35 : sorted[i].age <= 32 ? 18 : 8;
        std::cout << "  " << (i + 1) << ") " << posToString(sorted[i].pos) << " " << sorted[i].name
                   << " age " << sorted[i].age << " OVR " << sorted[i].ovr << " (success " << chance << "%)\n";
    }
    int pick = askInt("0) Cancel\n> ", 0, (int)sorted.size());
    if (pick == 0) return;

    Player chosen = sorted[pick - 1];
    if (chosen.ovr >= 92) { std::cout << chosen.name << " is already at peak rating.\n"; return; }
    int chance = chosen.age <= 21 ? 75 : chosen.age <= 25 ? 55 : chosen.age <= 29 ? 35 : chosen.age <= 32 ? 18 : 8;
    trainCharges--;
    bool success = util::chance(chance / 100.0);
    for (auto& p : c.squad) {
        if (p.id == chosen.id) {
            if (success) { p.ovr = std::min(92, p.ovr + 1); p.value = valueOf(p.ovr, p.age); }
            break;
        }
    }
    fixLineup(c);
    std::cout << (success ? ("Great session! " + chosen.name + " improved.\n")
                          : ("No progress this time for " + chosen.name + ".\n"));
}

void Game::financesMenu() {
    Club& c = userClub();
    double squadValue = 0;
    for (auto& p : c.squad) squadValue += p.value;
    std::cout << "\n== Finances ==\n";
    std::cout << "Budget: " << util::money(budget) << "\n";
    std::cout << "Season earnings so far: " << util::money(seasonEarnings) << "\n";
    std::cout << "Premium coins: " << c.coins << "\n";
    std::cout << "Squad value: " << util::money(squadValue) << "\n";
    std::cout << "Match bonuses -- Win: " << util::money(2.4) << "  Draw: " << util::money(1.0)
               << "  Loss: " << util::money(0.4) << "  Matchday gate: " << util::money(0.4) << "\n";
}

void Game::endSeason() {
    auto ranked = standings(table, teamNamesList());
    int rank = 0;
    for (size_t i = 0; i < ranked.size(); ++i) if (ranked[i].idx == userIdx) { rank = (int)i + 1; break; }
    const double bonuses[8] = {12, 8, 6, 4, 3, 2, 1.5, 1};
    double bonus = (rank >= 1 && rank <= 8) ? bonuses[rank - 1] : 1.0;
    std::string headline = rank == 1 ? "CHAMPIONS!" : rank <= 3 ? "Podium finish"
                          : rank >= 7 ? "Struggled near the bottom" : "A solid mid-table season";
    std::cout << "\n=== Season " << season << " report ===\n";
    std::cout << "Final position: " << rank << "/" << teams.size() << " -- " << headline << "\n";
    std::cout << "End-of-season bonus: " << util::money(bonus) << "\n";
    nextSeason(bonus);
}

void Game::nextSeason(double bonus) {
    const double glories[8] = {40, 25, 18, 12, 9, 6, 4, 2};
    auto ranked = standings(table, teamNamesList());
    int rank = 0;
    for (size_t i = 0; i < ranked.size(); ++i) if (ranked[i].idx == userIdx) { rank = (int)i + 1; break; }

    Club& c = userClub();
    budget = std::round((budget + bonus) * 10) / 10;
    c.glory += (rank >= 1 && rank <= 8) ? glories[rank - 1] : 1.0;

    season++; matchday = 0; results.clear(); trainCharges = 1; seasonEarnings = 0;
    table.clear();
    for (int i = 0; i < (int)teams.size(); ++i) { StandingsRow r; r.idx = i; table.push_back(r); }
    schedule = roundRobin((int)teams.size());

    for (auto& t : teams) {
        for (auto& p : t.squad) {
            p.age++;
            if (p.age > 31 && util::chance(0.4)) p.ovr = std::max(48, p.ovr - 1);
            p.stamina = 100;
            p.value = valueOf(p.ovr, p.age);
        }
    }
    generateMarket();
    updateRanks(table, teamNamesList());
    std::cout << "\nSeason " << season << " begins. Good luck!\n";
}

bool Game::save(const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "CLUBMANAGER_SAVE_V1\n";
    out << "NEXTID " << nextId << "\n";
    out << "SEASON " << season << "\n";
    out << "MATCHDAY " << matchday << "\n";
    out << "USERIDX " << userIdx << "\n";
    out << "BUDGET " << budget << "\n";
    out << "SEASONEARN " << seasonEarnings << "\n";
    out << "TRAINCHARGES " << trainCharges << "\n";
    out << "TEAMCOUNT " << teams.size() << "\n";
    for (auto& t : teams) {
        out << "TEAM " << util::toToken(t.name) << " " << t.formation << " " << t.mentality << " "
            << t.press << " " << t.teamwork << " " << t.glory << " " << t.coins << "\n";
        out << "SQUADCOUNT " << t.squad.size() << "\n";
        for (auto& p : t.squad) {
            out << "PLAYER " << p.id << " " << util::toToken(p.name) << " " << posToChar(p.pos) << " "
                << p.age << " " << p.ovr << " " << p.stamina << " " << p.value << "\n";
        }
        out << "LINEUPCOUNT " << t.lineup.size() << "\n";
        out << "LINEUP";
        for (int id : t.lineup) out << " " << id;
        out << "\n";
    }
    out << "TABLECOUNT " << table.size() << "\n";
    for (auto& r : table) {
        std::string form;
        for (char ch : r.form) form += ch;
        if (form.empty()) form = "-";
        out << "ROW " << r.idx << " " << r.played << " " << r.won << " " << r.draw << " " << r.lost
            << " " << r.goalsFor << " " << r.goalsAgainst << " " << r.points << " "
            << r.prevPos << " " << r.pos << " " << form << "\n";
    }
    out << "MARKETCOUNT " << market.size() << "\n";
    for (auto& p : market) {
        out << "PLAYER " << p.id << " " << util::toToken(p.name) << " " << posToChar(p.pos) << " "
            << p.age << " " << p.ovr << " " << p.stamina << " " << p.value << "\n";
    }
    out << "RESULTCOUNT " << results.size() << "\n";
    for (auto& r : results) {
        out << "RESULT " << r.matchday << " " << util::toToken(r.opponent) << " " << (r.home ? 1 : 0)
            << " " << r.gf << " " << r.ga << " " << r.res << "\n";
    }
    out << "USEDNAMES " << usedNames.size() << "\n";
    for (auto& n : usedNames) out << "NAME " << util::toToken(n) << "\n";
    out << "END\n";
    return true;
}

bool Game::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string tag;
    in >> tag;
    if (tag != "CLUBMANAGER_SAVE_V1") return false;

    teams.clear(); table.clear(); market.clear(); results.clear(); usedNames.clear();

    in >> tag >> nextId;
    in >> tag >> season;
    in >> tag >> matchday;
    in >> tag >> userIdx;
    in >> tag >> budget;
    in >> tag >> seasonEarnings;
    in >> tag >> trainCharges;

    int teamCount; in >> tag >> teamCount;
    for (int t = 0; t < teamCount; ++t) {
        Club c;
        std::string nameTok;
        in >> tag >> nameTok >> c.formation >> c.mentality >> c.press >> c.teamwork >> c.glory >> c.coins;
        c.name = util::fromToken(nameTok);

        int squadCount; in >> tag >> squadCount;
        for (int i = 0; i < squadCount; ++i) {
            Player p; std::string ptok; char posc;
            in >> tag >> p.id >> ptok >> posc >> p.age >> p.ovr >> p.stamina >> p.value;
            p.name = util::fromToken(ptok);
            p.pos = posFromChar(posc);
            c.squad.push_back(p);
        }
        int lineupCount; in >> tag >> lineupCount;
        in >> tag; // consumes the "LINEUP" marker
        c.lineup.resize(lineupCount);
        for (int i = 0; i < lineupCount; ++i) in >> c.lineup[i];

        teams.push_back(c);
    }

    int tableCount; in >> tag >> tableCount;
    for (int i = 0; i < tableCount; ++i) {
        StandingsRow r;
        std::string form;
        in >> tag >> r.idx >> r.played >> r.won >> r.draw >> r.lost >> r.goalsFor >> r.goalsAgainst
           >> r.points >> r.prevPos >> r.pos >> form;
        if (form != "-") for (char ch : form) r.form.push_back(ch);
        table.push_back(r);
    }

    int marketCount; in >> tag >> marketCount;
    for (int i = 0; i < marketCount; ++i) {
        Player p; std::string ptok; char posc;
        in >> tag >> p.id >> ptok >> posc >> p.age >> p.ovr >> p.stamina >> p.value;
        p.name = util::fromToken(ptok);
        p.pos = posFromChar(posc);
        market.push_back(p);
    }

    int resultCount; in >> tag >> resultCount;
    for (int i = 0; i < resultCount; ++i) {
        DisplayResult r; std::string otok; int homeFlag; char resc;
        in >> tag >> r.matchday >> otok >> homeFlag >> r.gf >> r.ga >> resc;
        r.opponent = util::fromToken(otok);
        r.home = homeFlag != 0;
        r.res = resc;
        results.push_back(r);
    }

    int usedCount; in >> tag >> usedCount;
    for (int i = 0; i < usedCount; ++i) {
        std::string ntok; in >> tag >> ntok;
        usedNames.insert(util::fromToken(ntok));
    }

    schedule = roundRobin((int)teams.size());
    return true;
}

void Game::run() {
    std::cout << "==================================\n";
    std::cout << "   CLUB MANAGER (console edition)\n";
    std::cout << "==================================\n";

    try {
        bool loaded = load("savegame.dat");
        if (loaded) {
            std::cout << "Save file found: " << userClub().name << ", season " << season << ".\n";
            int c = askInt("1) Continue  2) New game\n> ", 1, 2);
            if (c == 2) promptNewGame();
        } else {
            promptNewGame();
        }

        bool running = true;
        while (running) {
            std::cout << "\n--- " << userClub().name << " | Season " << season
                       << " | Matchday " << std::min(matchday + 1, 14) << "/14 ---\n";
            std::cout << "1) Dashboard\n2) Squad\n3) Tactics\n4) Play next match\n"
                         "5) League table\n6) Schedule\n7) Transfers\n8) Training\n"
                         "9) Finances\n10) Save\n0) Save & quit\n";
            int choice = askInt("> ", 0, 10);
            switch (choice) {
                case 1: showDashboard(); break;
                case 2: showSquad(); break;
                case 3: tacticsMenu(); break;
                case 4: playNextMatch(); break;
                case 5: showTable(); break;
                case 6: showSchedule(); break;
                case 7: transfersMenu(); break;
                case 8: trainingMenu(); break;
                case 9: financesMenu(); break;
                case 10: save("savegame.dat"); std::cout << "Saved.\n"; break;
                case 0: save("savegame.dat"); std::cout << "Saved. Goodbye!\n"; running = false; break;
            }
        }
    } catch (const EndOfInput&) {
        save("savegame.dat");
        std::cout << "\n[No more input -- auto-saved and exiting.]\n";
    }
}
