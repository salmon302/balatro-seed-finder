#pragma once

#include "filter_base.hpp"



const char* CHARM_TAG = "Charm Tag";
const char* THE_SOUL = "The Soul";
const char* PERKEO = "Perkeo";
const char* TRIBOULET = "Triboulet";
const char* YORICK = "Yorick";
const char* SOU = "sou";


// Original Perkeo filter implementation
class PerkeoFilter : public SearchFilter {
public:
    int apply(const std::string& seed) override {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        // Charm tag
        if (!fast_string_equals(inst.nextTag(1), CHARM_TAG)) return 0;
        
        // When opened, contains The soul
        std::vector<std::string> cards = inst.nextArcanaPack(5, 1);
        bool foundSoul = false;
        for (int c = 0; c < 5; c++) {
            if (fast_string_equals(cards[c], THE_SOUL)) {
                foundSoul = true;
                break;
            }
        }
        if (!foundSoul) return 0;

        // Check that the legendary is PERKEO
        if (!fast_string_equals(inst.nextJoker(SOU, 1, false).joker, PERKEO)) return 0;

        // Again
        if (!fast_string_equals(inst.nextTag(1), CHARM_TAG)) return 1;

        cards = inst.nextArcanaPack(5, 1);
        foundSoul = false;
        for (int c = 0; c < 5; c++) {
            if (fast_string_equals(cards[c], THE_SOUL)) {
                foundSoul = true;
                break;
            }
        }
        if (!foundSoul) return 1;

        if (!fast_string_equals(inst.nextJoker(SOU, 1, false).joker, TRIBOULET)) return 1;

        // Ante 2
        if (!fast_string_equals(inst.nextTag(2), CHARM_TAG)) return 2;
        cards = inst.nextArcanaPack(5, 2);
        foundSoul = false;
        for (int c = 0; c < 5; c++) {
            if (fast_string_equals(cards[c], THE_SOUL)) {
                foundSoul = true;
                break;
            }
        }
        if (!foundSoul) return 2;
        if (!fast_string_equals(inst.nextJoker(SOU, 2, false).joker, YORICK)) return 2;
        return 3;
    }
    
    std::vector<std::string> getResultNames() const override {
        return {"Perkeo", "Perkeo + Triboulet", "Perkeo + Triboulet + Yorick"};
    }
    
    std::string getName() const override {
        return "Perkeo Filter";
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<PerkeoFilter>();
}