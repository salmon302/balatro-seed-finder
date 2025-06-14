#pragma once

#include "filter_base.hpp"

bool nextTagCharmPerkeoTemperance(Instance inst, int ante) {
    if (!fast_string_equals(inst.nextTag(ante), "Charm Tag")) return false;
    std::vector<std::string> cards = inst.nextArcanaPack(5, ante);
    bool foundSoul = false;
    bool foundTemp = false;
    for (int c = 0; c < 5; c++) {
        if (fast_string_equals(cards[c], "The Soul")) {
            foundSoul = true;
        }
        if (fast_string_equals(cards[c], "Temperance")) {
            foundTemp = true;
        }
        if(foundSoul && foundTemp) {
            break;
        }
    }
    if(!foundSoul || !foundTemp) return false;
    return fast_string_equals(inst.nextJoker("sou", ante, false).joker, "Perkeo");
}

/*
Returns 1 when Telescope is found on first Ante
Returns 2 when Observatory is found on second Ante
*/
std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed) -> int {

        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);

        auto firstTag1 = nextTagCharmPerkeoTemperance(inst, 1);
        auto secondTag1 = nextTagCharmPerkeoTemperance(inst, 1);
        auto firstTag2 = nextTagCharmPerkeoTemperance(inst, 2);
        auto secondTag2 = nextTagCharmPerkeoTemperance(inst, 2);

        if(!firstTag1 && !secondTag1 && !firstTag2 && !secondTag2) return 0;
        
        if (!fast_string_equals(inst.nextVoucher(1), "Telescope")) return 1;
        inst.unlockDirect(OBSERVATORY);
        if (!fast_string_equals(inst.nextVoucher(2), "Observatory")) return 2;
        return 3;
    };
    
    return createCustomFilter(filterFunc, {"Perkeo + Temp", "Telescope", "Observatory"}, "Observatory on Ante 2 + Perkeo and Temp in Skip Tag");
}