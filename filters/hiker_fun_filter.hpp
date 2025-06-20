#pragma once

#include "filter_base.hpp"

/*
Returns 1 when Uncommon Tag on first Round
Returns 2 when the Uncommon tag gives Negative Hiker
Returns 3 when we open the first Joker pack and find Blueprint
*/
std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {

        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        // We skip the first small blind for rare tags
        if (!fast_string_equals(inst.nextTag(1), "Uncommon Tag")) return 0;
        auto nextJoker = inst.nextJoker("uta", 1, false);
        if (!fast_string_equals(nextJoker.joker, "Hiker") || nextJoker.edition != "Negative") return 1;
        // We see the big blind shop
        // First pack of game is always a Buffoon Pack
        auto pack = openPack(inst, 1, packInfo(inst.nextPack(1)));
        if(!packHasJoker(pack, "Blueprint")) {
            return 2;
        }
        return 3;
    };
    
    return createCustomFilter(filterFunc, {"Uncommon Tag", "Negative Hiker", "Blueprint"}, "Hiker + Blueprint");
}