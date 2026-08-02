# developing-a-game-
A terminal-based football club management game in modern C++17, with no external dependencies. Manage a fictional club through squad selection, tactics (5 formations, mentality, pressing), a minute-by-minute match simulation engine, transfers, player training, and a 14-matchday league season with save/load support. 
# Club Manager (C++ console edition)

A terminal-based football club management game written in modern C++17 — a sister project to the browser version, reimplemented as a native, dependency-free console app. Same core loop: squad management, tactics, transfers, training, and a 14-matchday league season, all with **100% fictional** clubs and players.

## 🎮 Play

No external libraries required — just a C++17 compiler.

### Build with CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
./club_manager
```

### Build with Make
```bash
make
./club_manager
```

### Build directly with g++
```bash
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o club_manager
./club_manager
```

Progress saves automatically to `savegame.dat` in the working directory (plain text, human-readable) whenever you choose **Save** or **Save & quit**, and loads automatically on the next launch.

## ✨ Features

- **Squad** — 18 generated players per club (2 GK / 6 DF / 7 MF / 3 FW) with age, rating, energy, and market value.
- **Tactics** — 5 formations (4-4-2, 4-3-3, 4-2-3-1, 3-5-2, 4-1-3-2) with PES-style role labels (LB/CB/RB, CMF/LMF/RMF, CF/LWF/RWF), attacking/balanced/defensive mentality, pressing, auto-XI, and manual player swaps by slot.
- **Match engine** — minute-by-minute simulation (1–90') driven by each side's effective attack/defence (mentality + pressing applied), producing goals, yellow/red cards, and a full text match report.
- **League** — 8-club double round-robin (14 matchdays), sorted standings with goal difference and form, movement tracking between matchdays.
- **Transfers** — a rotating player market; buying/selling affects budget and squad cohesion (teamwork).
- **Training** — one session per matchday, age-dependent success chance, permanently raises a player's rating.
- **Season** — end-of-season bonus by final position, player aging, a fresh schedule and market for the new season.
- **Save/Load** — flat, human-readable save format; no external serialization library needed.

## 🛠️ Project structure

```
.
├── CMakeLists.txt
├── Makefile
├── include/
│   ├── common.hpp        # RNG, clamping, money/text formatting helpers
│   ├── player.hpp         # Player struct, generation, PES-style role labels
│   ├── club.hpp            # Club struct, formations, lineup logic, team stats
│   ├── match_engine.hpp    # Minute-by-minute match simulation
│   ├── league.hpp          # Round-robin scheduling, standings
│   └── game.hpp            # Menu-driven orchestrator, save/load
└── src/
    ├── player.cpp
    ├── club.cpp
    ├── match_engine.cpp
    ├── league.cpp
    ├── game.cpp
    └── main.cpp
```

## 🧪 Tested

Compiled clean with `-Wall -Wextra -Wpedantic` (zero warnings) and smoke-tested end to end: new game → tactics/formation changes → full 90-minute matches → transfers (buy/sell) → training → save → reload (state verified to match) → a complete 14-matchday season → season rollover. Invalid/garbage menu input is handled by re-prompting rather than crashing.

## 📄 License

Personal / learning project. Add the license of your choice (e.g. MIT) if you plan to share it publicly.
