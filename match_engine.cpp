#include "match_engine.hpp"
#include "common.hpp"
#include <algorithm>

Player* weightedPlayer(std::vector<Player*>& xi) {
    if (xi.empty()) return nullptr;
    double total = 0;
    std::vector<double> weights;
    weights.reserve(xi.size());
    for (auto* p : xi) {
        double w = p->pos == Position::FW ? 5.0
                 : p->pos == Position::MF ? 2.4
                 : p->pos == Position::DF ? 0.6
                 : 0.05;
        weights.push_back(w);
        total += w;
    }
    double r = util::randDouble(0.0, total);
    for (size_t i = 0; i < xi.size(); ++i) {
        r -= weights[i];
        if (r <= 0) return xi[i];
    }
    return xi.back();
}

MatchResult simulateMatch(Club& home, Club& away) {
    Forces H = effForces(home, false);
    Forces A = effForces(away, false);
    auto hXI = starters(home);
    auto aXI = starters(away);

    double hRate = util::clampD(1.40 * (H.att / std::max(1.0, A.def)), 0.25, 4.2) / 90.0;
    double aRate = util::clampD(1.12 * (A.att / std::max(1.0, H.def)), 0.25, 4.2) / 90.0;
    double hMul = 1.0, aMul = 1.0;

    MatchResult result;
    for (int minute = 1; minute <= 90; ++minute) {
        if (util::chance(hRate * hMul)) {
            result.homeGoals++;
            Player* scorer = weightedPlayer(hXI);
            result.events.push_back({minute, "goal", "home", scorer ? scorer->name : "?"});
        }
        if (util::chance(aRate * aMul)) {
            result.awayGoals++;
            Player* scorer = weightedPlayer(aXI);
            result.events.push_back({minute, "goal", "away", scorer ? scorer->name : "?"});
        }
        if (util::chance(0.004)) {
            bool isHome = util::chance(0.5);
            auto& xi = isHome ? hXI : aXI;
            Player* p = weightedPlayer(xi);
            result.events.push_back({minute, "red", isHome ? "home" : "away", p ? p->name : "?"});
            if (isHome) hMul *= 0.78; else aMul *= 0.78;
        } else if (util::chance(0.03)) {
            bool isHome = util::chance(0.5);
            auto& xi = isHome ? hXI : aXI;
            Player* p = weightedPlayer(xi);
            result.events.push_back({minute, "yellow", isHome ? "home" : "away", p ? p->name : "?"});
        }
    }
    std::stable_sort(result.events.begin(), result.events.end(),
                      [](const MatchEvent& a, const MatchEvent& b) { return a.minute < b.minute; });
    return result;
}
