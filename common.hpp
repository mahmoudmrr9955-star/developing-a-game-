#pragma once
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Small, header-only helpers shared across the project: a process-wide RNG,
// clamping utilities, and display formatting. Kept dependency-free on purpose
// so the project builds with nothing but a standard C++17 compiler.
namespace util {

inline std::mt19937& rng() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

inline int randInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng());
}

inline double randDouble(double lo, double hi) {
    std::uniform_real_distribution<double> d(lo, hi);
    return d(rng());
}

inline bool chance(double probability) {
    return randDouble(0.0, 1.0) < probability;
}

inline int clampI(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

inline double clampD(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

// Formats a value in millions as e.g. "€3.4M" or "€120M" (no decimals >= 100).
inline std::string money(double v) {
    std::ostringstream os;
    os << "\xE2\x82\xAC" << std::fixed << std::setprecision(v >= 100.0 ? 0 : 1) << v << "M";
    return os.str();
}

// Replaces spaces with underscores (for the flat save-file format) and back.
inline std::string toToken(const std::string& s) {
    std::string out = s;
    std::replace(out.begin(), out.end(), ' ', '_');
    if (out.empty()) out = "_";
    return out;
}
inline std::string fromToken(const std::string& s) {
    std::string out = s;
    std::replace(out.begin(), out.end(), '_', ' ');
    return out;
}

} // namespace util
