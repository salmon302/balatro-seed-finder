#pragma once

#include "filter_base.hpp"

/*
Returns 1 when Telescope is found on first Ante
Returns 2 when Observatory is found on second Ante
*/
std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {

        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        if (!fast_string_equals(inst.nextVoucher(1), "Telescope")) return 0;
        inst.unlockDirect(OBSERVATORY);
        if (!fast_string_equals(inst.nextVoucher(2), "Observatory")) return 1;
        return 2;
    };
    
    return createCustomFilter(filterFunc, {"Telescope", "Observatory"}, "Observatory on Ante 2");
}