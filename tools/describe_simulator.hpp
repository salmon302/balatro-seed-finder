#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "../instance.hpp"
#include "../env.hpp"
#include "../items.hpp"

namespace DescribeSim {

struct CardTiming {
    std::string name;
    std::string slot; // e.g., "tarot", "joker"
    int position;     // position in the pack or -1
    int count;
    int turn;         // predicted play/score turn (conservative)
    std::string when; // semantic timing: "pack_open", "shop_joker", "predicted_play"
};

// Produce a conservative predicted description JSON for a seed.
// This is a lightweight, deterministic predictor (proof-of-concept) that
// records pack contents and predicts they will be played on turn 1.
// It is intentionally conservative and not a full game replay.
std::string describe_seed_timing(const std::string& seed);

} // namespace DescribeSim

// Inline implementation to avoid adding a separate compile unit.
namespace DescribeSim {

static std::string escape_json_simple(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

inline std::string describe_seed_timing(const std::string& seed) {
    try {
        Instance::Instance inst(seed);
        EnvConfig e = getGlobalEnv();
        if (!e.deck.empty()) inst.setDeck(e.deck);
        if (!e.stake.empty()) inst.setStake(e.stake);
        inst.setShowman(e.showman);
        inst.setSixesFactor(e.sixesFactor);
        inst.setVersion(e.version);
        inst.setForceAllContent(e.forceAllContent);
        inst.initLocks(1, e.freshProfile, e.freshRun);

        std::ostringstream out;
        out << "{\"index\": 1, \"name\": \"describe_simulator_prediction\", \"cards\": [";

        // Tag
        auto tag = inst.nextTag_enum(1);
        out << "{\"name\": \"" << escape_json_simple(Items::toString(tag)) << "\", \"slot\": \"tag\", \"position\": -1, \"count\": 1, \"turn\": 0, \"when\": \"pack_open\"}, ";

        // Arcana
        auto pack = inst.nextArcanaPack_enum(5,1);
        for (int i = 0; i < (int)pack.tarots.size(); ++i) {
            std::string cname;
            if (pack.isSpectral[i]) cname = Items::toString(pack.spectrals[i]);
            else cname = Items::toString(pack.tarots[i]);
            out << "{\"name\": \"" << escape_json_simple(cname) << "\", \"slot\": \"tarot\", \"position\": " << i << ", \"count\": 1, \"turn\": 1, \"when\": \"predicted_play\"}";
            if (i + 1 < (int)pack.tarots.size()) out << ", ";
            else out << ", ";
        }

        // Joker
        auto joker = inst.nextJoker_enum("sou",1,false);
        out << "{\"name\": \"" << escape_json_simple(Items::toString(joker.joker)) << "\", \"slot\": \"joker\", \"position\": -1, \"count\": 1, \"turn\": 1, \"when\": \"shop_joker\"}";

        out << "]}";
        return out.str();
    } catch (...) {
        return std::string();
    }
}

} // namespace DescribeSim
