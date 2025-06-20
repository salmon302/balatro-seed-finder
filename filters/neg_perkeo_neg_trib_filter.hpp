#pragma once

#include "filter_base.hpp"
struct LegendarySource {
    std::string legendary;
    std::string source;
};

std::string packHasTheSoulAndNegativeLegendary(Instance& inst, int ante, AnyPack p, std::ostream& debugStream) {
    for(int i = 0; i < p.others.size(); i++) {
        auto c = p.others[i];
        if(fast_string_equals(c, "The Soul")) {
            auto j = inst.nextJoker("sou", ante, false);
            if (j.edition == "Negative") {
                return j.joker;
            }
        }
    }
    return "";
}

std::string nextTagWithSoulAndNegLegendary(Instance& inst, int ante, std::ostream& debugStream) {
    auto tag = inst.nextTag(ante);
    if(fast_string_equals(tag, "Charm Tag")) {
        Pack p = {
            "Arcana Pack",
            5,
            2
        };
        auto op = openPack(inst, ante, p);
        return packHasTheSoulAndNegativeLegendary(inst, ante, op, debugStream);
    }
    if(fast_string_equals(tag, "Ethereal Tag")) {
        Pack p = {
            "Spectral Pack",
            2,
            1
        };
        auto op = openPack(inst, ante, p);
        return packHasTheSoulAndNegativeLegendary(inst, ante, op, debugStream);
    }
    return "";
}

LegendarySource charmTagOrShopForNegLegendary(Instance& inst, int ante, std::ostream& debugStream) {
    debugStream << "charmTagOrShopForNegLegendary " << ante << std::endl;
    auto tagLeg = nextTagWithSoulAndNegLegendary(inst, ante, debugStream);
    LegendarySource s;
    // We don't skip so check the shop
    if(tagLeg.length() == 0) {
        s.source = "shop";
        auto pack = inst.nextPack(ante);
        auto p = openPack(inst, ante, packInfo(pack));
    
        // First pack is always a buffoon pack, check if we already generated first pack in the past
        if(inst.cache.generatedFirstPack) {
            auto leg = packHasTheSoulAndNegativeLegendary(inst, ante, p, debugStream);
            if(leg.length() != 0) {
                s.legendary = leg;
                return s;
            }
        }
        pack = inst.nextPack(ante);
        p = openPack(inst, ante, packInfo(pack));
        s.legendary = packHasTheSoulAndNegativeLegendary(inst, ante, p, debugStream);
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
    auto filterFunc = [](const std::string& seed, std::ostream& debugStream) -> int {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);

        int score = 0;

        auto first = charmTagOrShopForNegLegendary(inst, 1, debugStream);
        if(first.legendary.length() != 0) {
            std::cout << first.legendary << std::endl;
            inst.lock(first.legendary);
        }
        auto second = charmTagOrShopForNegLegendary(inst, 1, debugStream);

        debugStream << first.source << " " << first.legendary << " " << second.source << " " << second.legendary << std::endl;

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