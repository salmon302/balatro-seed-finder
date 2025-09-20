#pragma once

#include "filter_base.hpp"

// Erratic Deck synergy search across early shop items (ante 1)
// Sets deck to "Erratic Deck" and looks for combos that benefit from
// randomized ranks/suits: Smeared packages, suit scalers + suit conversion,
// face engines with Pareidolia, straight/flush enablers, and parity engines.

std::unique_ptr<SearchFilter> createFilter() {
    auto filterFunc = [](const std::string& seed, std::ostream& debugOut) -> int {
        Instance::Instance inst(seed);
        // Apply global environment (deck/stake) if provided, then override deck for Erratic
        {
            EnvConfig e = getGlobalEnv();
            if (!e.deck.empty()) inst.setDeck(e.deck);
            if (!e.stake.empty()) inst.setStake(e.stake);
            inst.setShowman(e.showman);
            inst.setSixesFactor(e.sixesFactor);
            inst.setVersion(e.version);
            inst.setForceAllContent(e.forceAllContent);
        }
        // Erratic filter purposely forces Erratic Deck unless the global env explicitly set another
        inst.setDeck("Erratic Deck");
        {
            EnvConfig e = getGlobalEnv();
            inst.initLocks(1, e.freshProfile, e.freshRun);
        }

        // Scan ante 1 shop stream for Jokers and Tarots
        std::vector<Items::Joker> shopJokers;
        std::vector<Items::Tarot> shopTarots;
        shopJokers.reserve(32);
        shopTarots.reserve(16);
        for (int i = 0; i < 28; ++i) {
            auto item = inst.nextShopItem_enum(1);
            if (item.type == Items::OptimizedShopItem::Type::JOKER) {
                shopJokers.push_back(item.item.joker);
            } else if (item.type == Items::OptimizedShopItem::Type::TAROT) {
                shopTarots.push_back(item.item.tarot);
            }
        }

        auto has = [&](Items::Joker j) {
            for (auto x : shopJokers) if (x == j) return true; return false;
        };
        auto hasTarot = [&](Items::Tarot t) {
            for (auto x : shopTarots) if (x == t) return true; return false;
        };

        // Core Smeared packages (suits merged is extra strong on Erratic)
        bool smeared = has(Items::Joker::SMEARED_JOKER);
        bool suit_scaler = has(Items::Joker::GREEDY_JOKER) || has(Items::Joker::LUSTY_JOKER)
                        || has(Items::Joker::WRATHFUL_JOKER) || has(Items::Joker::GLUTTONOUS_JOKER)
                        || has(Items::Joker::BLOODSTONE) || has(Items::Joker::ARROWHEAD)
                        || has(Items::Joker::ONYX_AGATE);
        if (smeared && suit_scaler) return 1; // Smeared + Suit scalers

        if (smeared && has(Items::Joker::ANCIENT_JOKER)) return 2; // Smeared + Ancient Joker
        if (smeared && has(Items::Joker::THE_IDOL)) return 3;      // Smeared + The Idol

        // Face engines benefit from abundant/random faces + Pareidolia
        bool pareidolia = has(Items::Joker::PAREIDOLIA);
        bool face_support = has(Items::Joker::SCARY_FACE) || has(Items::Joker::SMILEY_FACE)
                         || has(Items::Joker::PHOTOGRAPH) || has(Items::Joker::SOCK_AND_BUSKIN)
                         || has(Items::Joker::MIDAS_MASK) || has(Items::Joker::BUSINESS_CARD)
                         || has(Items::Joker::RESERVED_PARKING);
        if (pareidolia && face_support) return 4; // Pareidolia + Face payoffs

        // Straight/Flush enablers: Four Fingers + helpers (randomized ranks help straights)
        bool four_fingers = has(Items::Joker::FOUR_FINGERS);
        bool straight_flush_support = has(Items::Joker::CRAZY_JOKER) || has(Items::Joker::DROLL_JOKER)
                                   || has(Items::Joker::SHORTCUT) || has(Items::Joker::SPACE_JOKER);
        if (four_fingers && straight_flush_support) return 5; // Four Fingers + support

        // Parity engine: Hack retriggers 2-5 which align with Even/Odd Todd payoffs
        if (has(Items::Joker::HACK) && (has(Items::Joker::EVEN_STEVEN) || has(Items::Joker::ODD_TODD))) return 6; // Hack + Even/Odd

        // Rank-duplicate helper: Seeing Double pairs nicely with Club Mult
        if (has(Items::Joker::SEEING_DOUBLE) && has(Items::Joker::ONYX_AGATE)) return 7; // Seeing Double + Onyx Agate

        // Suit-scaler + suit conversion tarots (great to focus random suits)
        if (has(Items::Joker::ONYX_AGATE) && hasTarot(Items::Tarot::THE_MOON)) return 8;   // Clubs
        if (has(Items::Joker::ARROWHEAD) && hasTarot(Items::Tarot::THE_WORLD)) return 9;   // Spades
        if (has(Items::Joker::BLOODSTONE) && hasTarot(Items::Tarot::THE_SUN)) return 10;   // Hearts
        if (has(Items::Joker::ROUGH_GEM) && hasTarot(Items::Tarot::THE_STAR)) return 11;   // Diamonds

        // Bonus: Four Fingers + Superposition (straight/Ace synergy)
        if (has(Items::Joker::SUPERPOSITION) && (four_fingers || has(Items::Joker::SHORTCUT))) return 12;

        return 0; // No synergy found
    };

    return createCustomFilter(
        filterFunc,
        {
            "Smeared + Suit scalers",
            "Smeared + Ancient Joker",
            "Smeared + The Idol",
            "Pareidolia + Face payoffs",
            "Four Fingers + Straight/Flush support",
            "Hack + Even/Odd payoffs",
            "Seeing Double + Onyx Agate",
            "Onyx Agate + The Moon",
            "Arrowhead + The World",
            "Bloodstone + The Sun",
            "Rough Gem + The Star",
            "Superposition + Straight enabler"
        },
        "Erratic Synergy Filter"
    );
}
