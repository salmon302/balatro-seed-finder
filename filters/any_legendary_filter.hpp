#pragma once

#include "../instance.hpp"
#include "filter_base.hpp"

// Forward declarations
class Instance;
bool fast_string_equals(const std::string& a, const char* b);
Instance initInstance(const std::string& seed, std::vector<bool> selectedOptions);

// Constants (extern declarations - defined in main)
extern const char* CHARM_TAG;
extern const char* THE_SOUL;
extern const char* PERKEO;
extern const char* TRIBOULET;
extern const char* YORICK;
extern const char* SOU;

std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed) -> int {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        if (!fast_string_equals(inst.nextTag(1), CHARM_TAG)) return 0;
        
        std::vector<std::string> cards = inst.nextArcanaPack(5, 1);
        bool foundSoul = false;
        for (int c = 0; c < 5; c++) {
            if (fast_string_equals(cards[c], THE_SOUL)) {
                foundSoul = true;
                break;
            }
        }
        if (!foundSoul) return 0;
        
        std::string legendary = inst.nextJoker(SOU, 1, false).joker;
        
        // Check for different legendary jokers
        if (fast_string_equals(legendary, PERKEO)) return 1;
        if (fast_string_equals(legendary, TRIBOULET)) return 2;
        if (fast_string_equals(legendary, YORICK)) return 3;
        
        return 0; // No legendary found
    };
    
    return createCustomFilter(filterFunc, 
                             {"Perkeo", "Triboulet", "Yorick"}, 
                             "Any Legendary Filter");
}