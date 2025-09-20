#pragma once

#include "synergy_matcher.hpp"
#include "filter_base.hpp"

std::unique_ptr<SearchFilter> createFilter() {
    using namespace Synergy;

    std::vector<Rule> rules;

    // 1) Pareidolia + face card support
    Rule r1("Pareidolia + Face synergy");
    r1.requireAll = {Items::Joker::PAREIDOLIA};
    r1.requireAny = {Items::Joker::SCARY_FACE, Items::Joker::SMILEY_FACE, Items::Joker::PHOTOGRAPH,
                     Items::Joker::SOCK_AND_BUSKIN, Items::Joker::MIDAS_MASK, Items::Joker::BUSINESS_CARD,
                     Items::Joker::RESERVED_PARKING};
    rules.push_back(std::move(r1));

    // 2) Smeared + suit scalers
    Rule r2("Smeared + Suit synergy");
    r2.requireAll = {Items::Joker::SMEARED_JOKER};
    r2.requireAny = {Items::Joker::GREEDY_JOKER, Items::Joker::LUSTY_JOKER, Items::Joker::WRATHFUL_JOKER,
                     Items::Joker::GLUTTONOUS_JOKER, Items::Joker::BLOODSTONE, Items::Joker::ARROWHEAD,
                     Items::Joker::ONYX_AGATE};
    rules.push_back(std::move(r2));

    // 3) Astronomer + constellation/satellite combos
    Rule r3("Astronomer + Constellation + Satellite");
    r3.requireAll = {Items::Joker::ASTRONOMER, Items::Joker::CONSTELLATION, Items::Joker::SATELLITE};
    rules.push_back(std::move(r3));

    Rule r4("Astronomer + Constellation");
    r4.requireAll = {Items::Joker::ASTRONOMER, Items::Joker::CONSTELLATION};
    rules.push_back(std::move(r4));

    Rule r5("Astronomer + Satellite");
    r5.requireAll = {Items::Joker::ASTRONOMER, Items::Joker::SATELLITE};
    rules.push_back(std::move(r5));

    // 27) Steel Joker + The Chariot tarot
    Rule r27("Steel Joker + The Chariot");
    r27.requireAll = {Items::Joker::STEEL_JOKER};
    r27.requireTarots = {Items::Tarot::THE_CHARIOT};
    rules.push_back(std::move(r27));

    // Example of a predicate: Fortune Teller + at least one tarot-generating joker in shop
    Rule fortune("Fortune Teller + Tarot generation");
    fortune.requireAll = {Items::Joker::FORTUNE_TELLER};
    fortune.predicate = [](Instance::Instance& inst, const std::unordered_set<int>& jokers,
                           const std::unordered_set<int>& tarots,
                           int /*voucher*/, int /*tag*/) -> bool {
        // scan next few joker draws to see if any tarot-generating jokers appear
        for (int i = 0; i < 12; ++i) {
            auto j = inst.nextJoker_enum("pred", 1, false).joker;
            if (j == Items::Joker::HALLUCINATION || j == Items::Joker::CARTOMANCER || j == Items::Joker::VAGABOND) return true;
        }
        return false;
    };
    rules.push_back(std::move(fortune));

    // 6) Four Fingers + straight/flush supports
    Rule r6("Four Fingers + Straight/Flush support");
    r6.requireAll = {Items::Joker::FOUR_FINGERS};
    r6.requireAny = {Items::Joker::CRAZY_JOKER, Items::Joker::DROLL_JOKER, Items::Joker::SHORTCUT, Items::Joker::SPACE_JOKER};
    rules.push_back(std::move(r6));

    // 7) Superposition + straight enablers
    Rule r7("Superposition + Straight enabler");
    r7.requireAll = {Items::Joker::SUPERPOSITION};
    r7.requireAny = {Items::Joker::FOUR_FINGERS, Items::Joker::SHORTCUT};
    rules.push_back(std::move(r7));

    // 8) Riff-Raff + Abstract Joker
    Rule r8("Riff-Raff + Abstract Joker");
    r8.requireAll = {Items::Joker::RIFF_RAFF, Items::Joker::ABSTRACT_JOKER};
    rules.push_back(std::move(r8));

    // 9) Blueprint/Brainstorm + high-value copy targets
    Rule r9("Blueprint/Brainstorm + Copy targets");
    r9.requireAny = {Items::Joker::BLUEPRINT, Items::Joker::BRAINSTORM};
    r9.requireAny.insert(r9.requireAny.end(), {Items::Joker::CONSTELLATION, Items::Joker::ASTRONOMER, Items::Joker::BARON, Items::Joker::FORTUNE_TELLER, Items::Joker::OBELISK, Items::Joker::SATELLITE, Items::Joker::CAMPFIRE, Items::Joker::HIKER, Items::Joker::BOOTSTRAPS});
    // The above uses requireAny twice; matcher will treat only the later requireAny — prefer predicate instead
    r9.predicate = [](Instance::Instance& /*inst*/, const std::unordered_set<int>& jokers,
                      const std::unordered_set<int>& /*tarots*/, int /*voucher*/, int /*tag*/) -> bool {
        // has blueprint/brainstorm and at least one high-value target
        bool hasSource = (jokers.count(static_cast<int>(Items::Joker::BLUEPRINT)) || jokers.count(static_cast<int>(Items::Joker::BRAINSTORM)));
        if (!hasSource) return false;
        static const int targets[] = { static_cast<int>(Items::Joker::CONSTELLATION), static_cast<int>(Items::Joker::ASTRONOMER), static_cast<int>(Items::Joker::BARON), static_cast<int>(Items::Joker::FORTUNE_TELLER), static_cast<int>(Items::Joker::OBELISK), static_cast<int>(Items::Joker::SATELLITE), static_cast<int>(Items::Joker::CAMPFIRE), static_cast<int>(Items::Joker::HIKER), static_cast<int>(Items::Joker::BOOTSTRAPS) };
        for (int t : targets) if (jokers.count(t)) return true;
        return false;
    };
    rules.push_back(std::move(r9));

    // 10) Baron + Shoot the Moon
    Rule r10("Baron + Shoot the Moon");
    r10.requireAll = {Items::Joker::BARON, Items::Joker::SHOOT_THE_MOON};
    rules.push_back(std::move(r10));

    // 11) Hiker + Retriggerers
    Rule r11("Hiker + Retriggerers");
    r11.requireAll = {Items::Joker::HIKER};
    r11.requireAny = {Items::Joker::DUSK, Items::Joker::SELTZER, Items::Joker::SOCK_AND_BUSKIN, Items::Joker::HACK};
    rules.push_back(std::move(r11));

    // 12) Vampire + Midas Mask
    Rule r12("Vampire + Midas Mask");
    r12.requireAll = {Items::Joker::VAMPIRE, Items::Joker::MIDAS_MASK};
    rules.push_back(std::move(r12));

    // 13) Gift Card + Swashbuckler
    Rule r13("Gift Card + Swashbuckler");
    r13.requireAll = {Items::Joker::GIFT_CARD, Items::Joker::SWASHBUCKLER};
    rules.push_back(std::move(r13));

    // 14) Hologram + DNA/Certificate
    Rule r14("Hologram + DNA/Certificate");
    r14.requireAll = {Items::Joker::HOLOGRAM};
    r14.requireAny = {Items::Joker::DNA, Items::Joker::CERTIFICATE};
    rules.push_back(std::move(r14));

    // 15) Bootstraps + Bull
    Rule r15("Bootstraps + Bull");
    r15.requireAll = {Items::Joker::BOOTSTRAPS, Items::Joker::BULL};
    rules.push_back(std::move(r15));

    // 16) Multi-face payoff stack (2+ face payoffs without Pareidolia)
    Rule r16("Multi-face payoffs (2+) without Pareidolia");
    r16.predicate = [](Instance::Instance& /*inst*/, const std::unordered_set<int>& jokers,
                       const std::unordered_set<int>& /*tarots*/, int /*voucher*/, int /*tag*/) -> bool {
        if (jokers.count(static_cast<int>(Items::Joker::PAREIDOLIA))) return false;
        static const int faces[] = { static_cast<int>(Items::Joker::SCARY_FACE), static_cast<int>(Items::Joker::SMILEY_FACE), static_cast<int>(Items::Joker::PHOTOGRAPH), static_cast<int>(Items::Joker::SOCK_AND_BUSKIN), static_cast<int>(Items::Joker::MIDAS_MASK), static_cast<int>(Items::Joker::BUSINESS_CARD), static_cast<int>(Items::Joker::RESERVED_PARKING) };
        int cnt = 0;
        for (int f : faces) if (jokers.count(f)) { if (++cnt >= 2) return true; }
        return false;
    };
    rules.push_back(std::move(r16));

    // 17) Ceremonial Dagger + Egg/Gift Card
    Rule r17("Ceremonial Dagger + Egg/Gift Card");
    r17.requireAll = {Items::Joker::CEREMONIAL_DAGGER};
    r17.requireAny = {Items::Joker::EGG, Items::Joker::GIFT_CARD};
    rules.push_back(std::move(r17));

    // 18) Egg + Swashbuckler
    Rule r18("Egg + Swashbuckler");
    r18.requireAll = {Items::Joker::EGG, Items::Joker::SWASHBUCKLER};
    rules.push_back(std::move(r18));

    // 19) Campfire + Gift Card
    Rule r19("Campfire + Gift Card");
    r19.requireAll = {Items::Joker::CAMPFIRE, Items::Joker::GIFT_CARD};
    rules.push_back(std::move(r19));

    // 20) Blackboard + suit scalers
    Rule r20("Blackboard + Suit scalers");
    r20.requireAll = {Items::Joker::BLACKBOARD};
    r20.requireAny = {Items::Joker::ONYX_AGATE, Items::Joker::WRATHFUL_JOKER, Items::Joker::ARROWHEAD};
    rules.push_back(std::move(r20));

    // 21) Smeared + Ancient Joker
    Rule r21("Smeared + Ancient Joker");
    r21.requireAll = {Items::Joker::SMEARED_JOKER, Items::Joker::ANCIENT_JOKER};
    rules.push_back(std::move(r21));

    // 22) Hack + Walkie Talkie
    Rule r22("Hack + Walkie Talkie");
    r22.requireAll = {Items::Joker::HACK, Items::Joker::WALKIE_TALKIE};
    rules.push_back(std::move(r22));

    // 23) Hack + Fibonacci
    Rule r23("Hack + Fibonacci");
    r23.requireAll = {Items::Joker::HACK, Items::Joker::FIBONACCI};
    rules.push_back(std::move(r23));

    // 24) Baseball Card + key uncommons
    Rule r24("Baseball Card + Key Uncommons");
    r24.requireAll = {Items::Joker::BASEBALL_CARD};
    r24.requireAny = {Items::Joker::HIKER, Items::Joker::CONSTELLATION, Items::Joker::SATELLITE};
    rules.push_back(std::move(r24));

    // 25) To the Moon + Bull/Bootstraps
    Rule r25("To the Moon + Bull/Bootstraps");
    r25.requireAll = {Items::Joker::TO_THE_MOON};
    r25.requireAny = {Items::Joker::BULL, Items::Joker::BOOTSTRAPS};
    rules.push_back(std::move(r25));

    // 26) Stone Joker + The Tower tarot
    Rule r26("Stone Joker + The Tower");
    r26.requireAll = {Items::Joker::STONE_JOKER};
    r26.requireTarots = {Items::Tarot::THE_TOWER};
    rules.push_back(std::move(r26));

    // 28) Glass Joker + Justice (keep numbering flexible)
    Rule r28("Glass Joker + Justice");
    r28.requireAll = {Items::Joker::GLASS_JOKER};
    r28.requireTarots = {Items::Tarot::JUSTICE};
    rules.push_back(std::move(r28));

    // 29) Golden Ticket + The Devil
    Rule r29("Golden Ticket + The Devil");
    r29.requireAll = {Items::Joker::GOLDEN_TICKET};
    r29.requireTarots = {Items::Tarot::THE_DEVIL};
    rules.push_back(std::move(r29));

    // 30) Rough Gem + The Star
    Rule r30("Rough Gem + The Star");
    r30.requireAll = {Items::Joker::ROUGH_GEM};
    r30.requireTarots = {Items::Tarot::THE_STAR};
    rules.push_back(std::move(r30));

    // 31) Bloodstone + The Sun
    Rule r31("Bloodstone + The Sun");
    r31.requireAll = {Items::Joker::BLOODSTONE};
    r31.requireTarots = {Items::Tarot::THE_SUN};
    rules.push_back(std::move(r31));

    // 32) Arrowhead + The World
    Rule r32("Arrowhead + The World");
    r32.requireAll = {Items::Joker::ARROWHEAD};
    r32.requireTarots = {Items::Tarot::THE_WORLD};
    rules.push_back(std::move(r32));

    // 33) Onyx Agate + The Moon
    Rule r33("Onyx Agate + The Moon");
    r33.requireAll = {Items::Joker::ONYX_AGATE};
    r33.requireTarots = {Items::Tarot::THE_MOON};
    rules.push_back(std::move(r33));

    // 34) Fortune Teller + The Emperor
    Rule r34("Fortune Teller + The Emperor");
    r34.requireAll = {Items::Joker::FORTUNE_TELLER};
    r34.requireTarots = {Items::Tarot::THE_EMPEROR};
    rules.push_back(std::move(r34));

    // 35) Constellation + The High Priestess
    Rule r35("Constellation + The High Priestess");
    r35.requireAll = {Items::Joker::CONSTELLATION};
    r35.requireTarots = {Items::Tarot::THE_HIGH_PRIESTESS};
    rules.push_back(std::move(r35));

    // 36) Flash Card + Reroll vouchers or D6 Tag
    Rule r36("Flash Card + Reroll or D6 Tag");
    r36.requireAll = {Items::Joker::FLASH_CARD};
    r36.predicate = [](Instance::Instance& /*inst*/, const std::unordered_set<int>& /*jokers*/, const std::unordered_set<int>& /*tarots*/, int voucher, int tag) -> bool {
        if (voucher == static_cast<int>(Items::Voucher::REROLL_SURPLUS) || voucher == static_cast<int>(Items::Voucher::REROLL_GLUT)) return true;
        if (tag == static_cast<int>(Items::Tag::D6_TAG)) return true;
        return false;
    };
    rules.push_back(std::move(r36));

    // 37) To the Moon + Investment Tag
    Rule r37("To the Moon + Investment Tag");
    r37.requireAll = {Items::Joker::TO_THE_MOON};
    r37.tagEquals = static_cast<int>(Items::Tag::INVESTMENT_TAG);
    rules.push_back(std::move(r37));

    // 38) Throwback + Speed Tag
    Rule r38("Throwback + Speed Tag");
    r38.requireAll = {Items::Joker::THROWBACK};
    r38.tagEquals = static_cast<int>(Items::Tag::SPEED_TAG);
    rules.push_back(std::move(r38));

    // 39) Constellation/Astronomer + Planet Merchant/Tycoon vouchers
    Rule r39("Constellation/Astronomer + Planet Merchant/Tycoon");
    r39.requireAny = {Items::Joker::CONSTELLATION, Items::Joker::ASTRONOMER};
    r39.predicate = [](Instance::Instance& /*inst*/, const std::unordered_set<int>& jokers, const std::unordered_set<int>& /*tarots*/, int voucher, int /*tag*/) -> bool {
        if (!jokers.count(static_cast<int>(Items::Joker::CONSTELLATION)) && !jokers.count(static_cast<int>(Items::Joker::ASTRONOMER))) return false;
        if (voucher == static_cast<int>(Items::Voucher::PLANET_MERCHANT) || voucher == static_cast<int>(Items::Voucher::PLANET_TYCOON)) return true;
        return false;
    };
    rules.push_back(std::move(r39));

    // 40) Fortune Teller/Cartomancer + Tarot Merchant/Tycoon vouchers
    Rule r40("Fortune Teller/Cartomancer + Tarot Merchant/Tycoon");
    r40.requireAny = {Items::Joker::FORTUNE_TELLER, Items::Joker::CARTOMANCER};
    r40.predicate = [](Instance::Instance& /*inst*/, const std::unordered_set<int>& jokers, const std::unordered_set<int>& /*tarots*/, int voucher, int /*tag*/) -> bool {
        if (!jokers.count(static_cast<int>(Items::Joker::FORTUNE_TELLER)) && !jokers.count(static_cast<int>(Items::Joker::CARTOMANCER))) return false;
        if (voucher == static_cast<int>(Items::Voucher::TAROT_MERCHANT) || voucher == static_cast<int>(Items::Voucher::TAROT_TYCOON)) return true;
        return false;
    };
    rules.push_back(std::move(r40));

    // 41) Triboulet + face multipliers
    Rule r41("Triboulet + face multipliers");
    r41.requireAll = {Items::Joker::TRIBOULET};
    r41.requireAny = {Items::Joker::BARON, Items::Joker::SHOOT_THE_MOON, Items::Joker::PHOTOGRAPH};
    rules.push_back(std::move(r41));

    // 42) Yorick + discard economy
    Rule r42("Yorick + discard economy");
    r42.requireAll = {Items::Joker::YORICK};
    r42.requireAny = {Items::Joker::MAIL_IN_REBATE, Items::Joker::TRADING_CARD, Items::Joker::HIT_THE_ROAD};
    rules.push_back(std::move(r42));

    // 43) Red Card + Campfire
    Rule r43("Red Card + Campfire");
    r43.requireAll = {Items::Joker::RED_CARD, Items::Joker::CAMPFIRE};
    rules.push_back(std::move(r43));

    // 44) Smeared + The Idol
    Rule r44("Smeared + The Idol");
    r44.requireAll = {Items::Joker::SMEARED_JOKER, Items::Joker::THE_IDOL};
    rules.push_back(std::move(r44));

    // 45) Seeing Double + Onyx Agate
    Rule r45("Seeing Double + Onyx Agate");
    r45.requireAll = {Items::Joker::SEEING_DOUBLE, Items::Joker::ONYX_AGATE};
    rules.push_back(std::move(r45));

    // 46) Hack + Even Steven/Odd Todd
    Rule r46("Hack + Even Steven/Odd Todd");
    r46.requireAll = {Items::Joker::HACK};
    r46.requireAny = {Items::Joker::EVEN_STEVEN, Items::Joker::ODD_TODD};
    rules.push_back(std::move(r46));

    // 47) Astronomer + Satellite + Bootstraps/Bull
    Rule r47("Astronomer + Satellite + Bootstraps/Bull");
    r47.requireAll = {Items::Joker::ASTRONOMER, Items::Joker::SATELLITE};
    r47.requireAny = {Items::Joker::BOOTSTRAPS, Items::Joker::BULL};
    rules.push_back(std::move(r47));

    Matcher m(std::move(rules), 28);

    // Build human readable names from rules BEFORE moving matcher (moving invalidates m)
    std::vector<std::string> names;
    for (const auto& r : m.rules) names.push_back(r.name);

    auto filterFunc = [m = std::move(m)](const std::string& seed, std::ostream& debugOut) -> int {
        return m.matchFirst(seed, debugOut);
    };

    return createCustomFilter(filterFunc, names, "Synergy Config Filter");
}
