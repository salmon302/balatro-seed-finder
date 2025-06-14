#pragma once

#include "filter_base.hpp"

bool nextPackIsArcanaWithSoulAndTrib(Instance inst, int ante) {
    Pack p = packInfo(inst.nextPack(ante));
    auto cards = inst.nextArcanaPack(p.size, ante);
    bool foundSoul = false;
    for (int c = 0; c < p.size; c++) {
        if (fast_string_equals(cards[c], "The Soul")) {
            foundSoul = true;
            break;
        }
    }
    if(!foundSoul) return false;
    return fast_string_equals(inst.nextJoker("sou", ante, false).joker, "Triboulet");
}

bool nextTagIsCharmWithSoulAndTrib(Instance inst, int ante) {
    if(!fast_string_equals(inst.nextTag(ante), "Charm Tag")) return false;
    auto cards = inst.nextArcanaPack(5, ante);
    bool foundSoul = false;
    for (int c = 0; c < 5; c++) {
        if (fast_string_equals(cards[c], "The Soul")) {
            foundSoul = true;
            break;
        }
    }
    if(!foundSoul) return false;
    return fast_string_equals(inst.nextJoker("sou", ante, false).joker, "Triboulet");
}

/*
Returns 1 when Rare Tag is on first round
Returns 2 when the rare tag gives Negative Blueprint
Returns 3 when Triboulet is available either in an Arcana pack in the second shop or on a Charm Tag on the small bind of the second ante
*/
std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed) -> int {

        // First tag is Rare
        // It gives blueprint on big blind shop 
        // Boss shop has Showman and a Mad Joker
        // Tag small blind ante 2 is Rare
        // It gives blueprint on big blind shop 

        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        // We skip the first small blind for rare tags
        if (!fast_string_equals(inst.nextTag(1), "Rare Tag")) return 0;
        // We see the big blind shop with Blueprint
        auto nextJoker = inst.nextJoker("rta", 1, false);
        if (!fast_string_equals(nextJoker.joker, "Blueprint")) return 1;
        // We discard the next item of the big blind shop
        inst.nextShopItem(1);

        // Boss shop
        
        

        // First pack of game is always a Buffoon Pack
        inst.nextPack(1);
        bool shop = nextPackIsArcanaWithSoulAndTrib(inst, 1);
        bool skipTag = nextTagIsCharmWithSoulAndTrib(inst, 2);
        if(!shop && !skipTag) return 2;
        return 3;
    };
    
    return createCustomFilter(filterFunc, {"Rare Tag", "Negative Blueprint", "Triboulet"}, "Rare skip for Neg Blueprint + Arcana or Charm to get Trib");
}