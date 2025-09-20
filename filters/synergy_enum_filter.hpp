#pragma once

#include "filter_base.hpp"
// optional simulator helper
#include "../tools/describe_simulator.hpp"
#include <sstream>

// Helper: perform the same detection logic and return the synergy index (0 == none)
static int detect_synergy(const std::string& seed) {
    try {
        Instance::Instance inst(seed);
        // Apply global environment (deck/stake) if provided
        {
            EnvConfig e = getGlobalEnv();
            if (!e.deck.empty()) inst.setDeck(e.deck);
            if (!e.stake.empty()) inst.setStake(e.stake);
            inst.setShowman(e.showman);
            inst.setSixesFactor(e.sixesFactor);
            inst.setVersion(e.version);
            inst.setForceAllContent(e.forceAllContent);
        }
        // Initialize locks using env freshProfile/freshRun flags
        {
            EnvConfig e = getGlobalEnv();
            inst.initLocks(1, e.freshProfile, e.freshRun);
        }

        // Scan the first set of shop items for ante 1 to collect early Jokers and Tarots
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

        // Peek ante-1 voucher and tag for voucher/tag synergies
        auto ante1Voucher = inst.nextVoucher_enum(1);
        auto ante1Tag = inst.nextTag_enum(1);

        auto has = [&](Items::Joker j) {
            for (auto x : shopJokers) if (x == j) return true;
            return false;
        };

        auto hasTarot = [&](Items::Tarot t) {
            for (auto x : shopTarots) if (x == t) return true;
            return false;
        };

        // Evaluate synergies (copied from original lambda)
        bool pareidolia = has(Items::Joker::PAREIDOLIA);
        bool face_support = has(Items::Joker::SCARY_FACE) || has(Items::Joker::SMILEY_FACE)
             || has(Items::Joker::PHOTOGRAPH) || has(Items::Joker::SOCK_AND_BUSKIN)
             || has(Items::Joker::MIDAS_MASK) || has(Items::Joker::BUSINESS_CARD)
             || has(Items::Joker::RESERVED_PARKING);
        if (pareidolia && face_support) return 1;

        bool smeared = has(Items::Joker::SMEARED_JOKER);
        bool suit_scaler = has(Items::Joker::GREEDY_JOKER) || has(Items::Joker::LUSTY_JOKER)
                        || has(Items::Joker::WRATHFUL_JOKER) || has(Items::Joker::GLUTTONOUS_JOKER)
                        || has(Items::Joker::BLOODSTONE) || has(Items::Joker::ARROWHEAD)
                        || has(Items::Joker::ONYX_AGATE);
        if (smeared && suit_scaler) return 2;

        bool astronomer = has(Items::Joker::ASTRONOMER);
        bool constellation = has(Items::Joker::CONSTELLATION);
        bool satellite = has(Items::Joker::SATELLITE);
        if (astronomer && constellation && satellite) return 3;
        if (astronomer && constellation) return 4;
        if (astronomer && satellite) return 5;

        bool four_fingers = has(Items::Joker::FOUR_FINGERS);
        bool straight_flush_support = has(Items::Joker::CRAZY_JOKER) || has(Items::Joker::DROLL_JOKER)
                                   || has(Items::Joker::SHORTCUT) || has(Items::Joker::SPACE_JOKER);
        if (four_fingers && straight_flush_support) return 6;

        bool fortune_teller = has(Items::Joker::FORTUNE_TELLER);
        bool tarot_gen = has(Items::Joker::HALLUCINATION) || has(Items::Joker::CARTOMANCER)
                      || has(Items::Joker::VAGABOND);
        if (fortune_teller && tarot_gen) return 7;

        bool superposition = has(Items::Joker::SUPERPOSITION);
        if (superposition && (four_fingers || has(Items::Joker::SHORTCUT))) return 8;

        if (has(Items::Joker::RIFF_RAFF) && has(Items::Joker::ABSTRACT_JOKER)) return 9;

        bool copier = has(Items::Joker::BLUEPRINT) || has(Items::Joker::BRAINSTORM);
        bool good_target = has(Items::Joker::CONSTELLATION) || has(Items::Joker::BARON)
                        || has(Items::Joker::ASTRONOMER) || has(Items::Joker::FORTUNE_TELLER)
                        || has(Items::Joker::OBELISK) || has(Items::Joker::SATELLITE)
                        || has(Items::Joker::CAMPFIRE) || has(Items::Joker::HIKER)
                        || has(Items::Joker::BOOTSTRAPS);
        if (copier && good_target) return 10;

        if (has(Items::Joker::BARON) && has(Items::Joker::SHOOT_THE_MOON)) return 11;

        if (has(Items::Joker::HIKER) && (has(Items::Joker::DUSK) || has(Items::Joker::SELTZER)
            || has(Items::Joker::SOCK_AND_BUSKIN) || has(Items::Joker::HACK))) return 12;

        if (has(Items::Joker::VAMPIRE) && has(Items::Joker::MIDAS_MASK)) return 13;

        if (has(Items::Joker::GIFT_CARD) && has(Items::Joker::SWASHBUCKLER)) return 14;

        if (has(Items::Joker::HOLOGRAM) && (has(Items::Joker::DNA) || has(Items::Joker::CERTIFICATE))) return 15;

        if (has(Items::Joker::BOOTSTRAPS) && has(Items::Joker::BULL)) return 16;

        int face_payoffs = 0;
        face_payoffs += has(Items::Joker::SCARY_FACE) ? 1 : 0;
        face_payoffs += has(Items::Joker::SMILEY_FACE) ? 1 : 0;
        face_payoffs += has(Items::Joker::PHOTOGRAPH) ? 1 : 0;
        face_payoffs += has(Items::Joker::SOCK_AND_BUSKIN) ? 1 : 0;
        face_payoffs += has(Items::Joker::MIDAS_MASK) ? 1 : 0;
        face_payoffs += has(Items::Joker::BUSINESS_CARD) ? 1 : 0;
        face_payoffs += has(Items::Joker::RESERVED_PARKING) ? 1 : 0;
        if (face_payoffs >= 2) return 17;

        if (has(Items::Joker::CEREMONIAL_DAGGER) && (has(Items::Joker::EGG) || has(Items::Joker::GIFT_CARD))) return 18;
        if (has(Items::Joker::EGG) && has(Items::Joker::SWASHBUCKLER)) return 19;
        if (has(Items::Joker::CAMPFIRE) && has(Items::Joker::GIFT_CARD)) return 20;
        if (has(Items::Joker::BLACKBOARD) && (has(Items::Joker::ONYX_AGATE) || has(Items::Joker::WRATHFUL_JOKER) || has(Items::Joker::ARROWHEAD))) return 21;
        if (smeared && has(Items::Joker::ANCIENT_JOKER)) return 22;
        if (has(Items::Joker::HACK) && has(Items::Joker::WALKIE_TALKIE)) return 23;
        if (has(Items::Joker::HACK) && (has(Items::Joker::EVEN_STEVEN) || has(Items::Joker::ODD_TODD))) return 48;
        if (has(Items::Joker::BASEBALL_CARD) && (has(Items::Joker::HIKER) || has(Items::Joker::CONSTELLATION) || has(Items::Joker::SATELLITE))) return 25;
        if (has(Items::Joker::TO_THE_MOON) && (has(Items::Joker::BULL) || has(Items::Joker::BOOTSTRAPS))) return 26;
        if (has(Items::Joker::STEEL_JOKER) && hasTarot(Items::Tarot::THE_CHARIOT)) return 27;
        if (has(Items::Joker::STONE_JOKER) && hasTarot(Items::Tarot::THE_TOWER)) return 28;
        if (has(Items::Joker::GLASS_JOKER) && hasTarot(Items::Tarot::JUSTICE)) return 29;
        if (has(Items::Joker::GOLDEN_TICKET) && hasTarot(Items::Tarot::THE_DEVIL)) return 30;
        if (has(Items::Joker::ROUGH_GEM) && hasTarot(Items::Tarot::THE_STAR)) return 31;
        if (has(Items::Joker::BLOODSTONE) && hasTarot(Items::Tarot::THE_SUN)) return 32;
        if (has(Items::Joker::ARROWHEAD) && hasTarot(Items::Tarot::THE_WORLD)) return 33;
        if (has(Items::Joker::ONYX_AGATE) && hasTarot(Items::Tarot::THE_MOON)) return 34;
        if (has(Items::Joker::VAMPIRE) && (hasTarot(Items::Tarot::THE_HIEROPHANT) || hasTarot(Items::Tarot::THE_EMPRESS)
            || hasTarot(Items::Tarot::THE_DEVIL) || hasTarot(Items::Tarot::THE_CHARIOT))) return 35;
        if (has(Items::Joker::FORTUNE_TELLER) && hasTarot(Items::Tarot::THE_EMPEROR)) return 36;
        if (has(Items::Joker::CONSTELLATION) && hasTarot(Items::Tarot::THE_HIGH_PRIESTESS)) return 37;

        if (has(Items::Joker::FLASH_CARD) && (ante1Voucher == Items::Voucher::REROLL_SURPLUS || ante1Voucher == Items::Voucher::REROLL_GLUT
            || ante1Tag == Items::Tag::D6_TAG)) return 38;
        if (has(Items::Joker::TO_THE_MOON) && ante1Tag == Items::Tag::INVESTMENT_TAG) return 39;
        if (has(Items::Joker::THROWBACK) && ante1Tag == Items::Tag::SPEED_TAG) return 40;
        if ((has(Items::Joker::CONSTELLATION) || has(Items::Joker::ASTRONOMER)) &&
            (ante1Voucher == Items::Voucher::PLANET_MERCHANT || ante1Voucher == Items::Voucher::PLANET_TYCOON)) return 41;
        if ((has(Items::Joker::FORTUNE_TELLER) || has(Items::Joker::CARTOMANCER)) &&
            (ante1Voucher == Items::Voucher::TAROT_MERCHANT || ante1Voucher == Items::Voucher::TAROT_TYCOON)) return 42;

        if (has(Items::Joker::TRIBOULET) && (has(Items::Joker::BARON) || has(Items::Joker::SHOOT_THE_MOON) || has(Items::Joker::PHOTOGRAPH))) return 43;
        if (has(Items::Joker::YORICK) && (has(Items::Joker::MAIL_IN_REBATE) || has(Items::Joker::TRADING_CARD) || has(Items::Joker::HIT_THE_ROAD))) return 44;
        if (has(Items::Joker::RED_CARD) && has(Items::Joker::CAMPFIRE)) return 45;
        if (smeared && has(Items::Joker::THE_IDOL)) return 46;
        if (has(Items::Joker::SEEING_DOUBLE) && has(Items::Joker::ONYX_AGATE)) return 47;
        if (has(Items::Joker::HACK) && (has(Items::Joker::EVEN_STEVEN) || has(Items::Joker::ODD_TODD))) return 48;
        if (has(Items::Joker::ASTRONOMER) && has(Items::Joker::SATELLITE) && (has(Items::Joker::BOOTSTRAPS) || has(Items::Joker::BULL))) return 49;

        return 0;
    } catch (...) {
        return 0;
    }
}

class SynergyEnumFilter : public SearchFilter {
public:
    int apply(const std::string& seed, std::ostream& debugOut = std::cout) override {
        return detect_synergy(seed);
    }

    std::vector<std::string> getResultNames() const override {
        return {
            "Pareidolia + Face synergy",
            "Smeared + Suit synergy",
            "Astronomer + Constellation + Satellite",
            "Astronomer + Constellation",
            "Astronomer + Satellite",
            "Four Fingers + Straight/Flush support",
            "Fortune Teller + Tarot generation",
            "Superposition + Straight enabler",
            "Riff-Raff + Abstract Joker",
            "Blueprint/Brainstorm + copy target",
            "Baron + Shoot the Moon",
            "Hiker + Retriggerers",
            "Vampire + Midas Mask",
            "Gift Card + Swashbuckler",
            "Hologram + DNA/Certificate",
            "Bootstraps + Bull",
            "Multi-face payoff stack",
            "Ceremonial Dagger + Egg/Gift Card",
            "Egg + Swashbuckler",
            "Campfire + Gift Card",
            "Blackboard + Spade/Club scalers",
            "Smeared + Ancient Joker",
            "Hack + Walkie Talkie",
            "Hack + Fibonacci",
            "Baseball Card + key Uncommons",
            "To the Moon + Bull/Bootstraps",
            "Steel Joker + The Chariot",
            "Stone Joker + The Tower",
            "Glass Joker + Justice",
            "Golden Ticket + The Devil",
            "Rough Gem + The Star",
            "Bloodstone + The Sun",
            "Arrowhead + The World",
            "Onyx Agate + The Moon",
            "Vampire + Enhancing Tarots",
            "Fortune Teller + The Emperor",
            "Constellation + The High Priestess",
            "Flash Card + Reroll voucher/D6 tag",
            "To the Moon + Investment Tag",
            "Throwback + Speed Tag",
            "Constellation/Astronomer + Planet voucher",
            "Fortune Teller/Cartomancer + Tarot voucher",
            "Triboulet + Face multipliers",
            "Yorick + Discard economy",
            "Red Card + Campfire",
            "Smeared + The Idol",
            "Seeing Double + Onyx Agate",
            "Hack + Even/Odd payoffs",
            "Astronomer + Satellite + Bootstraps/Bull"
        };
    }

    std::string getName() const override {
        return "Synergy Enum Filter";
    }

    std::string describeMatch(const std::string& seed) const override {
        int idx = detect_synergy(seed);
        if (idx == 0) return std::string();

        // Try the simulator output first and merge
        try {
            std::string sim = DescribeSim::describe_seed_timing(seed);
            if (!sim.empty()) {
                auto pos = sim.find("\"cards\"");
                if (pos != std::string::npos) {
                    auto cardsPos = sim.find('[', pos);
                    if (cardsPos != std::string::npos) {
                        std::string tail = sim.substr(cardsPos);
                        std::ostringstream oss;
                        auto names = getResultNames();
                        std::string rname = (idx <= (int)names.size() && idx > 0) ? names[idx-1] : getName();
                        oss << "{\"index\": " << idx << ", \"name\": \"" << rname << "\", \"cards\": " << tail;
                        return oss.str();
                    }
                }
            }
        } catch(...) {}

        // Fallback: use the simulator to at least produce cards, then set index/name
        try {
            std::string sim = DescribeSim::describe_seed_timing(seed);
            if (!sim.empty()) {
                // Best-effort: place index/name at top by simple string replace if possible
                auto brace = sim.find('{');
                auto cardsPos = sim.find("\"cards\"");
                if (cardsPos != std::string::npos) {
                    auto cardsStart = sim.find('[', cardsPos);
                    if (cardsStart != std::string::npos) {
                        std::ostringstream oss;
                        auto names = getResultNames();
                        std::string rname = (idx <= (int)names.size() && idx > 0) ? names[idx-1] : getName();
                        std::string tail = sim.substr(cardsStart);
                        oss << "{\"index\": " << idx << ", \"name\": \"" << rname << "\", \"cards\": " << tail;
                        return oss.str();
                    }
                }
            }
        } catch(...) {}

        return std::string();
    }
};

std::unique_ptr<SearchFilter> createFilter() {
    return std::make_unique<SynergyEnumFilter>();
}
