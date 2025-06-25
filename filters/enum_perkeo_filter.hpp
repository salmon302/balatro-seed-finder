#pragma once

#include "filter_base.hpp"

std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {

        Instance::Instance inst(seed);
        inst.initLocks(1, false, false);
        
        if (inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) return 0;
        
        auto cards = inst.nextArcanaPack_enum(5, 1);
        bool foundSoul = false;

        for (int i = 0; i < cards.tarots.size(); i++) {
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
        
        auto jokerData = inst.nextJoker_enum("sou", 1, false);
        if (jokerData.joker != Items::Joker::PERKEO) return 0;
        
        return 1;
    };
    
    return createCustomFilter(filterFunc, {"Perkeo + Soul + Charm"}, "Optimized Enum Perkeo Filter");
}