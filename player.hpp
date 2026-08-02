#pragma once
#include <string>
#include <set>
#include <vector>

enum class Position { GK, DF, MF, FW };

std::string posToString(Position p);
char posToChar(Position p);
Position posFromChar(char c);

struct Player {
    int id = 0;
    std::string name;
    Position pos = Position::MF;
    int age = 20;
    int ovr = 60;      // overall rating, 40-92
    int stamina = 100; // 0-100, current match energy
    double value = 0;  // market value in millions
};

// Market value model: rises steeply above 50 OVR, discounted by age.
double valueOf(int ovr, int age);

// Generates one fictional player of the given position/base rating.
// nextId is incremented; usedNames prevents duplicate names within a save.
Player genPlayer(int& nextId, Position pos, int baseOvr, std::set<std::string>& usedNames);

// Decorative PES-style role labels for a band of N players in a formation,
// e.g. 4 defenders -> {"LB","CB","CB","RB"}. Purely cosmetic for display.
std::vector<std::string> roleLabels(Position band, int count);
