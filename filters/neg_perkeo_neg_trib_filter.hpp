#pragma once

#include "filter_base.hpp"

struct LegendarySource {
    std::string legendary;
    std::string source;
};

std::string packNextLeg(Instance inst, int ante, std::string packType, int packSize) {
    std::vector<std::string> cards;
    if(fast_string_equals(packType, "Arcana Pack")) {
        cards = inst.nextArcanaPack(packSize, ante);
    } else if(fast_string_equals(packType, "Spectral Pack")) {
        cards = inst.nextSpectralPack(packSize, ante);
    } else {
        return "";
    }
    bool foundSoul = false;
    for (int c = 0; c < packSize; c++) {
        if (fast_string_equals(cards[c], "The Soul")) {
            foundSoul = true;
            break;
        }
    }
    if(!foundSoul) return "";
    auto j = inst.nextJoker("sou", ante, false);
    if(fast_string_equals(j.edition, "Negative")) {
        return j.joker;
    }
    return "";
}

std::string nextTagWithSoulAndNegLegendary(Instance inst, int ante) {
    auto tag = inst.nextTag(ante);
    if(fast_string_equals(tag, "Charm Tag")) {
        return packNextLeg(inst, ante, "Arcana Pack", 5);
    }
    if(fast_string_equals(tag, "Ethereal Tag")) {
        return packNextLeg(inst, ante, "Spectral Pack", 2);
    }
    return "";
}

LegendarySource charmTagOrShopForNegLegendary(Instance inst, int nShop) {
    auto tagLeg = nextTagWithSoulAndNegLegendary(inst, 1);
    LegendarySource s;
    if(tagLeg.length() == 0) {
        // We don't skip so check the shop
        if(nShop == 1) {
            // First pack is always a buffoon pack
            inst.nextPack(1);
        }
        auto p = packInfo(inst.nextPack(1));
        s.legendary = packNextLeg(inst, 1, p.type, p.size);
        s.source = "shop";
        return s; 
    }
    s.legendary = tagLeg;
    s.source = "tag";
    return s;
}

/*
Returns 1 when one of Neg Perkeo or Neg Trib is found
Returns 2 when both are found
*/
std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed) -> int {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);

        int score = 0;

        auto first = charmTagOrShopForNegLegendary(inst, 1);
        auto second = charmTagOrShopForNegLegendary(inst, first.source == "shop" ? 2 : 1);

        if(fast_string_equals(first.legendary,"Triboulet") || fast_string_equals(second.legendary, "Triboulet")) {
            score++;
        }

        if(fast_string_equals(first.legendary,"Perkeo") || fast_string_equals(second.legendary, "Perkeo")) {
            score++;
        }

        return score;
        
    };
    
    return createCustomFilter(filterFunc, {"Neg Perkeo or Trib", "Neg perkeo AND Trib"}, "Find neg Trib + neg Perkeo");
}