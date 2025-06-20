# Custom Search Filters

This directory contains the modular search filter system for TheSoul. Each filter is defined in its own `.hpp` file and can be built independently.

## Available Filters

### 1. `perkeo_filter.hpp` - Original Perkeo Filter
The complete original filter that searches for:
- **Perkeo**: Charm tag + The Soul card + Perkeo legendary joker
- **Perkeo + Triboulet**: Above + second Charm tag + The Soul + Triboulet
- **Perkeo + Triboulet + Yorick**: Above + third Charm tag + The Soul + Yorick

Result names: "Perkeo", "Perkeo + Triboulet", "Perkeo + Triboulet + Yorick"

### 2. `perkeo_only_filter.hpp` - Simplified Perkeo Filter
Only searches for the first condition:
- **Perkeo**: Charm tag + The Soul card + Perkeo legendary joker

Result names: "Perkeo"

### 3. `any_legendary_filter.hpp` - Any Legendary Joker Filter
Searches for any legendary joker in the first shop:
- **Perkeo**: Charm tag + The Soul + Perkeo found
- **Triboulet**: Charm tag + The Soul + Triboulet found  
- **Yorick**: Charm tag + The Soul + Yorick found

Result names: "Perkeo", "Triboulet", "Yorick"

### 4. `charm_tag_filter.hpp` - Simple Charm Tag Filter
Basic demonstration filter that only checks for a Charm Tag in ante 1:
- **Charm Tag Found**: Any seed with a Charm Tag in ante 1

Result names: "Charm Tag Found"

## Building with Filters

Use the build script to compile with a specific filter:

```bash
cd include/tools
./build_simple.sh <filter_name>
```

### Examples

```bash
# Build with the original complete Perkeo filter
./build_simple.sh perkeo

# Build with only Perkeo detection
./build_simple.sh perkeo_only

# Build with any legendary joker detection
./build_simple.sh any_legendary

# Build with simple charm tag detection
./build_simple.sh charm_tag
```

The compiled executables are placed in `include/dist/` with names like `immolate_<filter_name>`.

## Creating Custom Filters

### Method 1: Class-based Filter

Create a new `.hpp` file following this pattern:

```cpp
#pragma once

#include "filter_base.hpp"

// Forward declarations
class Instance;
bool fast_string_equals(const std::string& a, const char* b);
Instance initInstance(const std::string& seed, std::vector<bool> selectedOptions);

// Constants you might need
extern const char* CHARM_TAG;
extern const char* THE_SOUL;
// ... other constants

class MyCustomFilter : public SearchFilter {
public:
    int apply(const std::string& seed, std::ostream& debugOut = std::cout) override {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        // Your custom logic here
        // Return 0 for no match, 1+ for different match types
        
        return 0;
    }
    
    std::vector<std::string> getResultNames() const override {
        return {"Result Type 1", "Result Type 2", "Result Type 3"};
    }
    
    std::string getName() const override {
        return "My Custom Filter";
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<MyCustomFilter>();
}
```

### Method 2: Lambda-based Filter

```cpp
#pragma once

#include "filter_base.hpp"

// Forward declarations and constants as above...

std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(seed, selectedOptions);
        
        // Your custom logic here
        if (/* some condition */) return 1;
        if (/* another condition */) return 2;
        
        return 0; // No match
    };
    
    return createCustomFilter(filterFunc, 
                             {"Match Type 1", "Match Type 2"}, 
                             "My Lambda Filter");
}
```

## Filter Return Values

- **0**: No match
- **1+**: Different types of matches, corresponding to indices in `getResultNames()`

The system automatically tracks statistics for each result type and displays them with the configured names.

## Available Instance Methods

When implementing filters, you can use these Instance methods:
- `inst.nextTag(ante)` - Get the tag for a specific ante
- `inst.nextArcanaPack(size, ante)` - Get cards from an arcana pack
- `inst.nextJoker(type, ante, legendary)` - Get joker information
- `inst.nextBoss(ante)` - Get boss information
- `inst.nextVoucher(ante)` - Get voucher information

## Constants Available

These constants are available for string comparisons:
- `CHARM_TAG` - "Charm Tag"
- `THE_SOUL` - "The Soul"  
- `PERKEO` - "Perkeo"
- `TRIBOULET` - "Triboulet"
- `YORICK` - "Yorick"
- `SOU` - "sou"

## Performance Tips

1. Use `fast_string_equals()` for string comparisons instead of `==`
2. Return early (return 0) as soon as you determine there's no match
3. Structure your conditions from most restrictive to least restrictive
4. The `selectedOptions(61, true)` parameter enables all game content for maximum flexibility