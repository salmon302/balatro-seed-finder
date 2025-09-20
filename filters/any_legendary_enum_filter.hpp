#pragma once

#include "filter_base.hpp"

// optional simulator helper
#include "../tools/describe_simulator.hpp"
#include <sstream>


class AnyLegendaryFilter : public SearchFilter {
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

        if(inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) return 0;
        auto cards = inst.nextArcanaPack_enum(5, 1);
        bool foundSoul = false;

        for (int i = 0; i < (int)cards.tarots.size(); i++) {
            if(cards.isSpectral[i]) {
                if(cards.spectrals[i] == Items::Spectral::SPECTRAL_THE_SOUL) {
                    foundSoul = true;
                }
            } else {
                if(cards.tarots[i] == Items::Tarot::SPECIAL_THE_SOUL) {
                    foundSoul = true;
                }
            }
        }
        if (!foundSoul) return 0;
        
        auto legendary = inst.nextJoker_enum("sou", 1, false).joker;
        if (legendary == Items::Joker::PERKEO) return 1;
        if (legendary == Items::Joker::TRIBOULET) return 2;
        if (legendary == Items::Joker::YORICK) return 3;
        if (legendary == Items::Joker::CHICOT) return 4;
        if (legendary == Items::Joker::CANIO) return 5;
        return 0;
    }

    std::vector<std::string> getResultNames() const override {
        return {"Perkeo", "Triboulet", "Yorick", "Chicot", "Canio"};
    }

    std::string getName() const override {
        return "Any Legendary Filter";
    }

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

            if(inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) return std::string();
            auto cards2 = inst.nextArcanaPack_enum(5,1);
            bool foundSoul = false;
            for (int i = 0; i < (int)cards2.tarots.size(); i++) {
                if(cards2.isSpectral[i]) {
                    if(cards2.spectrals[i] == Items::Spectral::SPECTRAL_THE_SOUL) foundSoul = true;
                } else {
                    if(cards2.tarots[i] == Items::Tarot::SPECIAL_THE_SOUL) foundSoul = true;
                }
            }
            if (!foundSoul) return std::string();
            auto jokerData2 = inst.nextJoker_enum("sou",1,false);
            auto legendary = jokerData2.joker;
            int idx = 0;
            if (legendary == Items::Joker::PERKEO) idx = 1;
            else if (legendary == Items::Joker::TRIBOULET) idx = 2;
            else if (legendary == Items::Joker::YORICK) idx = 3;
            else if (legendary == Items::Joker::CHICOT) idx = 4;
            else if (legendary == Items::Joker::CANIO) idx = 5;
            if (idx == 0) return std::string();

            // Try simulator first, fall back to manual JSON if it fails
            try {
                std::string sim = DescribeSim::describe_seed_timing(seed);
                if (!sim.empty()) {
                    auto pos = sim.find("\"cards\"");
                    if (pos != std::string::npos) {
                        auto cardsPos = sim.find('[', pos);
                        if (cardsPos != std::string::npos) {
                            std::string tail = sim.substr(cardsPos);
                            std::ostringstream oss;
                            oss << "{\"index\": " << idx << ", \"name\": \"Any Legendary Filter\", \"cards\": " << tail;
                            return oss.str();
                        }
                    }
                }
            } catch(...) {}

            std::string out = "{";
            out += "\"index\": " + std::to_string(idx) + ", ";
            out += "\"name\": \"Any Legendary Filter\", ";
            out += "\"cards\": [";
            bool first = true;
            for (int i = 0; i < (int)cards2.tarots.size(); i++) {
                std::string cname;
                if (cards2.isSpectral[i]) cname = std::string(Items::toString(cards2.spectrals[i]));
                else cname = std::string(Items::toString(cards2.tarots[i]));
                if (!first) out += ", ";
                first = false;
                // include best-effort timing placeholders: pack cards are "pack_open"
                out += "{\"name\": \"" + cname + "\", \"slot\": \"tarot\", \"position\": " + std::to_string(i) + ", \"count\": 1, \"turn\": 0, \"when\": \"pack_open\"}";
            }
            std::string jname = std::string(Items::toString(jokerData2.joker));
            if (!first) out += ", ";
            // Joker timing placeholder: shop_joker
            out += "{\"name\": \"" + jname + "\", \"slot\": \"joker\", \"position\": -1, \"count\": 1, \"turn\": 0, \"when\": \"shop_joker\"}";
            out += "]}";
            return out;
        } catch (...) {
            return std::string();
        }
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<AnyLegendaryFilter>();
}