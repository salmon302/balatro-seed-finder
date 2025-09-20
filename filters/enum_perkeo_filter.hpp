#pragma once

#include "filter_base.hpp"
// optional simulator helper
#include "../tools/describe_simulator.hpp"
// lightweight string utilities
#include <sstream>

class EnumPerkeoFilter : public SearchFilter {
public:
    int apply(const std::string& seed, std::ostream& debugOut = std::cout) override {
        Instance::Instance inst(seed);
        EnvConfig e = getGlobalEnv();
        if (!e.deck.empty()) inst.setDeck(e.deck);
        if (!e.stake.empty()) inst.setStake(e.stake);
        inst.setShowman(e.showman);
        inst.setSixesFactor(e.sixesFactor);
        inst.setVersion(e.version);
        inst.setForceAllContent(e.forceAllContent);
        inst.initLocks(1, e.freshProfile, e.freshRun);

        if (inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) return 0;
        auto cards = inst.nextArcanaPack_enum(5, 1);
        bool foundSoul = false;
        for (int i = 0; i < (int)cards.tarots.size(); i++) {
            if (cards.isSpectral[i]) {
                if (cards.spectrals[i] == Items::Spectral::SPECTRAL_THE_SOUL) foundSoul = true;
            } else {
                if (cards.tarots[i] == Items::Tarot::SPECIAL_THE_SOUL) foundSoul = true;
            }
        }
        if (!foundSoul) return 0;
        auto jokerData = inst.nextJoker_enum("sou", 1, false);
        if (jokerData.joker != Items::Joker::PERKEO) return 0;
        return 1;
    }

    std::vector<std::string> getResultNames() const override {
        return {"Perkeo + Soul + Charm"};
    }

    std::string getName() const override {
        return "Optimized Enum Perkeo Filter";
    }

    // Provide a structured JSON description for the seed when possible
    std::string describeMatch(const std::string& seed) const override {
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

            int idx = 0;
            if (inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) {
                idx = 0;
            } else {
                auto cards = inst.nextArcanaPack_enum(5, 1);
                bool foundSoul = false;
                for (int i = 0; i < (int)cards.tarots.size(); i++) {
                    if (cards.isSpectral[i]) {
                        if (cards.spectrals[i] == Items::Spectral::SPECTRAL_THE_SOUL) foundSoul = true;
                    } else {
                        if (cards.tarots[i] == Items::Tarot::SPECIAL_THE_SOUL) foundSoul = true;
                    }
                }
                if (!foundSoul) {
                    idx = 0;
                } else {
                    auto jokerData = inst.nextJoker_enum("sou", 1, false);
                    if (jokerData.joker != Items::Joker::PERKEO) idx = 0; else idx = 1;
                }
            }

            // If not a match, still return minimal index
            if (idx == 0) {
                return std::string();
            }

            // Try to use the lightweight describe simulator to produce richer timing info.
            try {
                std::string sim = DescribeSim::describe_seed_timing(seed);
                if (!sim.empty()) {
                    // Simple string-merge: insert/replace index and name fields while keeping cards
                    // This avoids depending on a JSON library inside the header.
                    // Find the "cards" array in simulator output.
                    auto pos = sim.find("\"cards\"");
                    if (pos != std::string::npos) {
                        // Build new JSON: {"index": idx, "name": "...", "cards": <rest-from-cards>}
                        auto bracePos = sim.find('{');
                        auto cardsPos = sim.find('[', pos);
                        if (cardsPos != std::string::npos) {
                            std::string tail = sim.substr(cardsPos);
                            std::ostringstream oss;
                            oss << "{\"index\": " << idx << ", \"name\": \"Optimized Enum Perkeo Filter\", \"cards\": " << tail;
                            return oss.str();
                        }
                    }
                }
            } catch(...) {
                // fall back to simple output below
            }

            // Fallback: original simple JSON (keeps older behavior)
            std::string out = "{";
            out += "\"index\": 1, ";
            out += "\"name\": \"Optimized Enum Perkeo Filter\", ";
            out += "\"cards\": [";

            // Re-simulate to gather cards: the arcana pack and the joker
            Instance::Instance inst2(seed);
            if (!e.deck.empty()) inst2.setDeck(e.deck);
            if (!e.stake.empty()) inst2.setStake(e.stake);
            inst2.setShowman(e.showman);
            inst2.setSixesFactor(e.sixesFactor);
            inst2.setVersion(e.version);
            inst2.setForceAllContent(e.forceAllContent);
            inst2.initLocks(1, e.freshProfile, e.freshRun);
            auto cards2 = inst2.nextArcanaPack_enum(5,1);
            bool first = true;
            for (int i = 0; i < (int)cards2.tarots.size(); i++) {
                std::string cname;
                if (cards2.isSpectral[i]) cname = std::string(Items::toString(cards2.spectrals[i]));
                else cname = std::string(Items::toString(cards2.tarots[i]));
                if (!first) out += ", ";
                first = false;
                // include slot position and best-effort timing placeholders
                // "turn": 0 indicates immediate pack-open effect; "when": "pack_open" as semantic hint
                out += "{\"name\": \"" + cname + "\", \"slot\": \"tarot\", \"position\": " + std::to_string(i) + ", \"count\": 1, \"turn\": 0, \"when\": \"pack_open\"}";
            }
            auto jokerData2 = inst2.nextJoker_enum("sou",1,false);
            std::string jname = std::string(Items::toString(jokerData2.joker));
            if (!first) out += ", ";
            // Joker emitted from shop/nextJoker: mark as turn 0 and when="shop_joker" (best-effort)
            out += "{\"name\": \"" + jname + "\", \"slot\": \"joker\", \"position\": -1, \"count\": 1, \"turn\": 0, \"when\": \"shop_joker\"}";

            out += "]}";
            return out;
        } catch (...) {
            return std::string();
        }
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<EnumPerkeoFilter>();
}