#include "player.hpp"
#include "common.hpp"
#include <array>

std::string posToString(Position p) {
    switch (p) {
        case Position::GK: return "GK";
        case Position::DF: return "DF";
        case Position::MF: return "MF";
        case Position::FW: return "FW";
    }
    return "??";
}

char posToChar(Position p) {
    switch (p) {
        case Position::GK: return 'G';
        case Position::DF: return 'D';
        case Position::MF: return 'M';
        case Position::FW: return 'F';
    }
    return '?';
}

Position posFromChar(char c) {
    switch (c) {
        case 'G': return Position::GK;
        case 'D': return Position::DF;
        case 'M': return Position::MF;
        case 'F': return Position::FW;
        default:  return Position::MF;
    }
}

namespace {
const std::array<const char*, 40> FIRST_NAMES = {
    "Lucas","Mateo","Diego","Karim","Yann","Bruno","Tariq","Eden","Niko","Vito",
    "Aron","Dani","Felix","Omar","Ivo","Theo","Leon","Rui","Sami","Noa",
    "Marco","Hugo","Elias","Nael","Pavel","Joao","Andre","Sora","Kofi","Mio",
    "Adil","Remy","Tomas","Luka","Ezra","Bilal","Nuno","Kian","Yuki","Saul"
};
const std::array<const char*, 40> LAST_NAMES = {
    "Veron","Adeyemi","Costa","Lindqvist","Moreau","Falcao","Benhima","Sorin","Krause","Delacroix",
    "Okafor","Navarro","Ferreira","Halloum","Petrov","Mancini","Dubois","Aslan","Nakamura","Mensah",
    "Roca","Vidic","Lozano","Brandt","Sissoko","Aguero","Klein","Vasquez","Tamm","Bjorn",
    "Reyes","Marchand","Olsen","Diallo","Castro","Werner","Saric","Lund","Mertens","Ozdemir"
};
} // namespace

double valueOf(int ovr, int age) {
    double base = std::max(0, ovr - 50);
    base = base * base * 0.012;
    double ageFactor = age <= 22 ? 1.3 : age <= 27 ? 1.1 : age <= 30 ? 0.85 : age <= 33 ? 0.55 : 0.3;
    return std::max(0.2, std::round(base * ageFactor * 10.0) / 10.0);
}

Player genPlayer(int& nextId, Position pos, int baseOvr, std::set<std::string>& usedNames) {
    std::string name;
    do {
        name = std::string(FIRST_NAMES[util::randInt(0, (int)FIRST_NAMES.size() - 1)]) + " " +
               std::string(LAST_NAMES[util::randInt(0, (int)LAST_NAMES.size() - 1)]);
    } while (usedNames.count(name));
    usedNames.insert(name);

    int spread = (pos == Position::GK) ? util::randInt(-2, 3) : util::randInt(-4, 5);
    int ovr = util::clampI(baseOvr + spread, 48, 91);
    int age = util::randInt(18, 35);

    Player p;
    p.id = nextId++;
    p.name = name;
    p.pos = pos;
    p.age = age;
    p.ovr = ovr;
    p.stamina = util::randInt(82, 100);
    p.value = valueOf(ovr, age);
    return p;
}

std::vector<std::string> roleLabels(Position band, int count) {
    if (band == Position::GK) return std::vector<std::string>(count, "GK");
    if (band == Position::DF) {
        if (count == 3) return {"CB", "CB", "CB"};
        if (count == 4) return {"LB", "CB", "CB", "RB"};
        if (count == 5) return {"LB", "CB", "CB", "CB", "RB"};
        return std::vector<std::string>(count, "DF");
    }
    if (band == Position::MF) {
        if (count == 2) return {"CMF", "CMF"};
        if (count == 3) return {"CMF", "CMF", "CMF"};
        if (count == 4) return {"LMF", "CMF", "CMF", "RMF"};
        if (count == 5) return {"LMF", "CMF", "CMF", "CMF", "RMF"};
        return std::vector<std::string>(count, "MF");
    }
    // Forwards
    if (count == 1) return {"CF"};
    if (count == 2) return {"CF", "CF"};
    if (count == 3) return {"LWF", "CF", "RWF"};
    return std::vector<std::string>(count, "FW");
}
