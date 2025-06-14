#pragma once

#include "filter_base.hpp"

// Forward declarations
class Instance;
bool fast_string_equals(const std::string& a, const char* b);
Instance initInstance(const std::string& seed, std::vector<bool> selectedOptions);

// Constants (extern declarations - defined in main)
extern const char* CHARM_TAG;

std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed) -> int {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        // Check for Charm Tag in ante 1
        if (fast_string_equals(inst.nextTag(1), CHARM_TAG)) {
            return 1;
        }
        
        return 0;
    };
    
    return createCustomFilter(filterFunc, {"Charm Tag Found"}, "Charm Tag Filter");
}