#pragma once

#include "filter_base.hpp"


// Original Perkeo filter implementation
class PerkeoFilter : public SearchFilter {
public:
    int apply(const std::string& seed, std::ostream& debugOut = std::cout) override {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        // Charm tag
        if (!fast_string_equals(inst.nextTag(1), "Charm Tag")) return 0;
        
        // When opened, contains The soul
        std::vector<std::string> cards = inst.nextArcanaPack(5, 1);
        bool foundSoul = false;
        for (int c = 0; c < 5; c++) {
            if (fast_string_equals(cards[c], "The Soul")) {
                foundSoul = true;
                break;
            }
        }
        if (!foundSoul) return 0;

        // Check that the legendary is PERKEO
        if (!fast_string_equals(inst.nextJoker("sou", 1, false).joker, "Perkeo")) return 0;
        return 1;
    }
    
    std::vector<std::string> getResultNames() const override {
        return {"Perkeo"};
    }
    
    std::string getName() const override {
        return "Perkeo Filter";
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<PerkeoFilter>();
}