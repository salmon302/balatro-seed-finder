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

### 5. `synergy_enum_filter.hpp` - Synergy-Based Early Game Combos
Searches ante 1 shop items for strong Joker pair synergies (explicit and theoretical) such as:
- Pareidolia + any face-card payoff (Scary Face, Smiley Face, Photograph, Sock and Buskin, Midas Mask)
- Smeared Joker + suit-scalers (Greedy/Lusty/Wrathful/Gluttonous, Bloodstone, Arrowhead, Onyx Agate)
- Astronomer + Constellation/Satellite packages
- Four Fingers + straight/flush supports (Crazy, Droll, Shortcut, Space Joker)
- Fortune Teller + Tarot generation (Hallucination, Cartomancer, Vagabond)
- Superposition + straight enablers (Four Fingers/Shortcut)
- Riff-Raff + Abstract Joker (free jokers increase Abstract scaling)
- Blueprint/Brainstorm + high-value copy targets (Constellation, Astronomer, Baron, Fortune Teller, Obelisk, Satellite, Campfire, Hiker, Bootstraps)
- Baron + Shoot the Moon (Kings/Queens in hand multipliers)
- Hiker + Retriggerers (Dusk, Seltzer, Sock and Buskin, Hack)
- Vampire + Midas Mask (steady enhanced faces to feed Vampire)
- Gift Card + Swashbuckler (sell value ramp → Mult)
- Hologram + DNA/Certificate (more cards added → faster scaling)
- Bootstraps + Bull (cash → Mult and Chips)
- Multi-face payoff stack (2+ face payoffs without Pareidolia)
- Ceremonial Dagger + Egg/Gift Card
- Egg + Swashbuckler
- Campfire + Gift Card
- Blackboard + Spade/Club scalers (Onyx Agate, Wrathful, Arrowhead)
- Smeared + Ancient Joker
- Hack + Walkie Talkie
- Hack + Fibonacci
- Baseball Card + key Uncommons (Hiker, Constellation, Satellite)
- To the Moon + Bull/Bootstraps
- Steel Joker + The Chariot
- Stone Joker + The Tower
- Glass Joker + Justice
- Golden Ticket + The Devil
- Rough Gem + The Star
- Bloodstone + The Sun
- Arrowhead + The World
- Onyx Agate + The Moon
- Vampire + Enhancing Tarots (Hierophant/Empress/Devil/Chariot)
- Fortune Teller + The Emperor
- Constellation + The High Priestess
- Flash Card + Reroll vouchers (Surplus/Glut) or D6 Tag
- To the Moon + Investment Tag
- Throwback + Speed Tag
- Constellation/Astronomer + Planet Merchant/Tycoon vouchers
- Fortune Teller/Cartomancer + Tarot Merchant/Tycoon vouchers
- Triboulet + face multipliers (Baron/Shoot the Moon/Photograph)
- Yorick + discard economy (Mail-In Rebate, Trading Card, Hit the Road)
- Red Card + Campfire
- Smeared + The Idol
- Seeing Double + Onyx Agate
- Hack + Even Steven/Odd Todd
- Astronomer + Satellite + Bootstraps/Bull

Result names: a list covering each synergy above.

### 6. `erratic_enum_filter.hpp` - Erratic Deck Synergy Combos
Targets seeds that are especially potent on the Erratic Deck (randomized ranks/suits). It sets the deck to "Erratic Deck" and searches ante 1 shop items for:
- Smeared Joker packages:
    - Smeared + suit scalers (Greedy/Lusty/Wrathful/Gluttonous, Bloodstone, Arrowhead, Onyx Agate)
    - Smeared + Ancient Joker
    - Smeared + The Idol
- Pareidolia + any face-card payoff (Scary Face, Smiley Face, Photograph, Sock and Buskin, Midas Mask, Business Card, Reserved Parking)
- Four Fingers + straight/flush supports (Crazy, Droll, Shortcut, Space Joker)
- Hack + Even Steven/Odd Todd
- Seeing Double + Onyx Agate
- Suit-scaler + suit-conversion tarot:
    - Onyx Agate + The Moon (Clubs)
    - Arrowhead + The World (Spades)
    - Bloodstone + The Sun (Hearts)
    - Rough Gem + The Star (Diamonds)
- Superposition + straight enabler (Four Fingers or Shortcut)

Result names: a list covering each synergy above.

## Building with Filters

Use the build script to compile with a specific filter:

```powershell
tools\build.bat <filter_name>
```

### Examples

```powershell
# Build with the original complete Perkeo filter
tools\build.bat perkeo

# Build with only Perkeo detection
tools\build.bat perkeo_only

# Build with any legendary joker detection
tools\build.bat any_legendary

# Build with simple charm tag detection
tools\build.bat charm_tag

# Build Erratic Deck synergy filter
tools\build.bat erratic_enum
```

The compiled executables are placed in `dist/` with names like `immolate_<filter_name>.exe`.

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