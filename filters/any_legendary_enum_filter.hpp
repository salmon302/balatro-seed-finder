#pragma once

#include "filter_base.hpp"


std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {

        Instance::Instance inst(seed);
        inst.initLocks(1, false, false);

        if(inst.nextTag_enum(1) != Items::Tag::CHARM_TAG) return 0;
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
        
        auto legendary = inst.nextJoker_enum("sou", 1, false).joker;
        
        // Check for different legendary jokers
        if (legendary == Items::Joker::PERKEO) return 1;
        if (legendary == Items::Joker::TRIBOULET) return 2;
        if (legendary == Items::Joker::YORICK) return 3;
        if (legendary == Items::Joker::CHICOT) return 4;
        if (legendary == Items::Joker::CANIO) return 5;
        
        return 0; // No legendary found
    };
    
    return createCustomFilter(filterFunc, 
                             {"Perkeo", "Triboulet", "Yorick", "Chicot", "Canio"}, 
                             "Any Legendary Filter");
}