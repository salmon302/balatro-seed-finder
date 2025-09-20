#pragma once

#include <string>
#include <vector>

struct EnvConfig {
    std::string deck = "Red Deck";
    std::string stake = "White Stake";
    bool showman = false;
    std::string tag = "";
    // New: list of unlocked tag names (human-friendly names matching Items::toString(Items::Tag))
    std::vector<std::string> unlockedTags;
    bool freshProfile = false;
    bool freshRun = false;
    int sixesFactor = 1;
    long version = 10106;
    bool forceAllContent = true;
    // Optional explicit selectedOptions (61 entries). Empty vector -> not explicitly set.
    std::vector<bool> selectedOptions = {};
    bool selectedOptionsSet = false;
    // Optional explicit unlocked jokers (human-friendly names matching Items::toString(Items::Joker))
    std::vector<std::string> unlockedJokers;
};

void setGlobalEnv(const EnvConfig& e);
EnvConfig getGlobalEnv();
