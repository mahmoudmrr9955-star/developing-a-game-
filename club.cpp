#include "club.hpp"
#include "common.hpp"
#include <algorithm>
#include <cmath>

const std::map<std::string, std::vector<Position>> FORMATIONS = {
    {"4-4-2",    {Position::GK, Position::DF, Position::DF, Position::DF, Position::DF,
                  Position::MF, Position::MF, Position::MF, Position::MF,
                  Position::FW, Position::FW}},
    {"4-3-3",    {Position::GK, Position::DF, Position::DF, Position::DF, Position::DF,
                  Position::MF, Position::MF, Position::MF,
                  Position::FW, Position::FW, Position::FW}},
    {"4-2-3-1",  {Position::GK, Position::DF, Position::DF, Position::DF, Position::DF,
                  Position::MF, Position::MF, Position::MF, Position::MF, Position::MF,
                  Position::FW}},
    {"3-5-2",    {Position::GK, Position::DF, Position::DF, Position::DF,
                  Position::MF, Position::MF, Position::MF, Position::MF, Position::MF,
                  Position::FW, Position::FW}},
    {"4-1-3-2",  {Position::GK, Position::DF, Position::DF, Position::DF, Position::DF,
                  Position::MF, Position::MF, Position::MF, Position::MF,
                  Position::FW, Position::FW}},
};

const std::vector<std::string> FORMATION_NAMES = {
    "4-4-2", "4-3-3", "4-2-3-1", "3-5-2", "4-1-3-2"
};

std::vector<Player> genSquad(int& nextId, int baseOvr, std::set<std::string>& usedNames) {
    std::vector<Player> squad;
    struct Plan { Position pos; int count; };
    const Plan plan[] = {{Position::GK, 2}, {Position::DF, 6}, {Position::MF, 7}, {Position::FW, 3}};
    for (auto& pl : plan) {
        for (int i = 0; i < pl.count; ++i) {
            int bump = (i == 0) ? 2 : 0; // first of each band is a touch stronger (a "starter" quality bias)
            squad.push_back(genPlayer(nextId, pl.pos, baseOvr + bump, usedNames));
        }
    }
    return squad;
}

void autoLineup(Club& club) {
    auto it = FORMATIONS.find(club.formation);
    const std::vector<Position>& slots = (it != FORMATIONS.end()) ? it->second : FORMATIONS.at("4-3-3");

    std::map<Position, std::vector<Player*>> byPos;
    std::vector<Player*> sorted;
    for (auto& p : club.squad) sorted.push_back(&p);
    std::sort(sorted.begin(), sorted.end(), [](Player* a, Player* b) { return a->ovr > b->ovr; });
    for (auto* p : sorted) byPos[p->pos].push_back(p);

    std::vector<int> used;
    std::vector<int> line;
    auto isUsed = [&](int id) { return std::find(used.begin(), used.end(), id) != used.end(); };

    for (auto band : slots) {
        Player* pick = nullptr;
        for (auto* p : byPos[band]) {
            if (!isUsed(p->id)) { pick = p; break; }
        }
        if (pick) { used.push_back(pick->id); line.push_back(pick->id); }
        else line.push_back(-1);
    }
    // Fill any remaining gaps with the best still-unused player, regardless of position.
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == -1) {
            for (auto* p : sorted) {
                if (!isUsed(p->id)) { used.push_back(p->id); line[i] = p->id; break; }
            }
        }
    }
    club.lineup = line;
}

void fixLineup(Club& club) {
    auto it = FORMATIONS.find(club.formation);
    const std::vector<Position>& slots = (it != FORMATIONS.end()) ? it->second : FORMATIONS.at("4-3-3");

    if (club.lineup.size() != slots.size()) { autoLineup(club); return; }

    std::vector<int> validIds;
    for (auto& p : club.squad) validIds.push_back(p.id);
    auto exists = [&](int id) { return std::find(validIds.begin(), validIds.end(), id) != validIds.end(); };

    std::vector<int> used;
    auto isUsed = [&](int id) { return std::find(used.begin(), used.end(), id) != used.end(); };

    for (size_t i = 0; i < slots.size(); ++i) {
        int id = club.lineup[i];
        if (id != -1 && exists(id) && !isUsed(id)) { used.push_back(id); continue; }

        Player* best = nullptr;
        for (auto& p : club.squad) {
            if (p.pos == slots[i] && !isUsed(p.id)) {
                if (!best || p.ovr > best->ovr) best = &p;
            }
        }
        if (!best) {
            for (auto& p : club.squad) {
                if (!isUsed(p.id)) {
                    if (!best || p.ovr > best->ovr) best = &p;
                }
            }
        }
        if (best) { club.lineup[i] = best->id; used.push_back(best->id); }
        else club.lineup[i] = -1;
    }
}

std::vector<Player*> starters(Club& club) {
    std::vector<Player*> xi;
    for (int id : club.lineup) {
        if (id == -1) continue;
        for (auto& p : club.squad) {
            if (p.id == id) { xi.push_back(&p); break; }
        }
    }
    return xi;
}

Forces lineStrength(Club& club, bool full) {
    auto xi = starters(club);
    std::map<Position, std::vector<Player*>> g;
    for (auto* p : xi) g[p->pos].push_back(p);

    auto avg = [&](Position band) -> double {
        auto& v = g[band];
        if (v.empty()) return 60.0;
        double sum = 0;
        for (auto* p : v) {
            double factor = full ? 1.0 : (0.6 + 0.4 * p->stamina / 100.0);
            sum += p->ovr * factor;
        }
        return sum / v.size();
    };

    double GK = avg(Position::GK), DF = avg(Position::DF), MF = avg(Position::MF), FW = avg(Position::FW);
    Forces f;
    f.att = 0.55 * FW + 0.35 * MF + 0.10 * DF;
    f.def = 0.42 * GK + 0.46 * DF + 0.12 * MF;
    return f;
}

Forces effForces(Club& club, bool full) {
    Forces f = lineStrength(club, full);
    if (club.mentality == "off") { f.att *= 1.13; f.def *= 0.90; }
    if (club.mentality == "def") { f.att *= 0.89; f.def *= 1.13; }
    if (club.press == "agg") { f.att *= 1.05; f.def *= 0.96; }
    if (club.press == "con") { f.att *= 0.96; f.def *= 1.05; }
    return f;
}

int teamOVR(Club& club) {
    auto xi = starters(club);
    if (xi.empty()) return 0;
    int sum = 0;
    for (auto* p : xi) sum += p->ovr;
    return (int)std::round((double)sum / xi.size());
}

TeamStats teamStats(Club& club) {
    Forces fc = effForces(club, false);
    Forces fp = effForces(club, true);
    int attack = (int)std::round(fc.att * 10);
    int defence = (int)std::round(fc.def * 10);
    int twk = (int)std::round(util::clampD(club.teamwork, 400, 999));
    int otr = (int)std::round(attack * 0.38 + defence * 0.38 + twk * 0.24);
    int attackMax = (int)std::round(fp.att * 10);
    int defenceMax = (int)std::round(fp.def * 10);
    int otrMax = (int)std::round(attackMax * 0.38 + defenceMax * 0.38 + twk * 0.24);
    TeamStats st;
    st.otr = otr; st.otrMax = otrMax; st.attack = attack; st.defence = defence;
    st.teamwork = twk; st.glory = (int)std::round(club.glory);
    return st;
}
