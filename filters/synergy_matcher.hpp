#pragma once

#include "filter_base.hpp"
#include "../instance.hpp"
#include "../items.hpp"
#include "../debug.hpp"
#include <vector>
#include <unordered_set>
#include <functional>
#include <optional>

// Small, header-only utility to define and evaluate "synergy" rules
// Rules are expressed programmatically (no external JSON dependency).
// The matcher scans the early shop/tarot/voucher/tag outputs and tests
// rules efficiently using unordered_sets of observed enums.

namespace Synergy {

struct Rule {
    std::string name;
    // All of these jokers must be present
    std::vector<Items::Joker> requireAll;
    // At least one of these jokers must be present (if non-empty)
    std::vector<Items::Joker> requireAny;
    // None of these jokers may be present (if non-empty)
    std::vector<Items::Joker> exclude;

    // Optional tarot requirements: all of these tarots must be present
    std::vector<Items::Tarot> requireTarots;

    // Optional voucher/tag checks (peek at ante-1)
    // Use int sentinel values for voucher/tag comparisons. -1 = don't check / not present
    // otherwise compare against static_cast<int>(Items::Voucher::...)
    int voucherEquals = -1;
    int tagEquals = -1;

    // Custom predicate for things too specific for the simple shape.
    // It is given the Instance (already initialized) and the observed sets.
    // voucher/tag are passed as int where -1 means absent.
    std::function<bool(Instance::Instance&, const std::unordered_set<int>& jokers,
                       const std::unordered_set<int>& tarots,
                       int voucher, int tag)> predicate;

    Rule(const std::string& n) : name(n) {}
};

class Matcher {
public:
    explicit Matcher(std::vector<Rule> r, int shopScan = 28) : rules(std::move(r)), scanCount(shopScan) {}

    // Evaluate rules for a seed. Returns index+1 of first matching rule, or 0
    int matchFirst(const std::string& seed, std::ostream& debugOut = std::cout) const {
        Instance::Instance inst(seed);
        // Apply global env like the other filters do
        {
            EnvConfig e = getGlobalEnv();
            if (!e.deck.empty()) inst.setDeck(e.deck);
            if (!e.stake.empty()) inst.setStake(e.stake);
            inst.setShowman(e.showman);
            inst.setSixesFactor(e.sixesFactor);
            inst.setVersion(e.version);
            inst.setForceAllContent(e.forceAllContent);
        }
        {
            EnvConfig e = getGlobalEnv();
            inst.initLocks(1, e.freshProfile, e.freshRun);
        }

        // Collect early shop jokers and tarots
        std::unordered_set<int> jokers;
        std::unordered_set<int> tarots;
        jokers.reserve(scanCount);

        for (int i = 0; i < scanCount; ++i) {
            auto item = inst.nextShopItem_enum(1);
            if (item.type == Items::OptimizedShopItem::Type::JOKER) {
                jokers.insert(static_cast<int>(item.item.joker));
            } else if (item.type == Items::OptimizedShopItem::Type::TAROT) {
                tarots.insert(static_cast<int>(item.item.tarot));
            }
        }

        // Debug: print observed jokers/tarots (gated)
        try {
            std::string line = "[matcher] observed jokers:";
            for (int j : jokers) { line += ' '; line += Items::toString(static_cast<Items::Joker>(j)); }
            line += " | tarots:";
            for (int t : tarots) { line += ' '; line += Items::toString(static_cast<Items::Tarot>(t)); }
            debug_println(line);
        } catch(...) {}

        auto anteVoucher = inst.nextVoucher_enum(1);
        auto anteTag = inst.nextTag_enum(1);

    int voucherOpt = -1;
    int tagOpt = -1;
    voucherOpt = (static_cast<int>(anteVoucher) == static_cast<int>(Items::Voucher::INVALID)) ? -1 : static_cast<int>(anteVoucher);
    tagOpt = (static_cast<int>(anteTag) == static_cast<int>(Items::Tag::INVALID)) ? -1 : static_cast<int>(anteTag);

        // Evaluate rules in order; first match wins. Rules may use predicate for advanced checks.
        for (size_t ri = 0; ri < rules.size(); ++ri) {
            const Rule& r = rules[ri];

            bool fail = false;
            // requireAll
            for (auto j : r.requireAll) {
                if (jokers.find(static_cast<int>(j)) == jokers.end()) { fail = true; break; }
            }
            if (fail) continue;

            // requireAny
            if (!r.requireAny.empty()) {
                bool any = false;
                for (auto j : r.requireAny) if (jokers.find(static_cast<int>(j)) != jokers.end()) { any = true; break; }
                if (!any) continue;
            }

            // exclude
            bool excluded = false;
            for (auto j : r.exclude) if (jokers.find(static_cast<int>(j)) != jokers.end()) { excluded = true; break; }
            if (excluded) continue;

            // tarots
            bool tarotFail = false;
            for (auto t : r.requireTarots) if (tarots.find(static_cast<int>(t)) == tarots.end()) { tarotFail = true; break; }
            if (tarotFail) continue;

            // voucher/tag
            if (r.voucherEquals != -1) {
                if (voucherOpt == -1 || voucherOpt != r.voucherEquals) continue;
            }
            if (r.tagEquals != -1) {
                if (tagOpt == -1 || tagOpt != r.tagEquals) continue;
            }

            // custom predicate
            if (r.predicate) {
                Instance::Instance instForPred(seed);
                // apply env and locks for predicate instance to be safe
                {
                    EnvConfig e = getGlobalEnv();
                    if (!e.deck.empty()) instForPred.setDeck(e.deck);
                    if (!e.stake.empty()) instForPred.setStake(e.stake);
                    instForPred.setShowman(e.showman);
                    instForPred.setSixesFactor(e.sixesFactor);
                    instForPred.setVersion(e.version);
                    instForPred.setForceAllContent(e.forceAllContent);
                }
                {
                    EnvConfig e = getGlobalEnv();
                    instForPred.initLocks(1, e.freshProfile, e.freshRun);
                }
                if (!r.predicate(instForPred, jokers, tarots, voucherOpt, tagOpt)) continue;
            }

            // matched
            return static_cast<int>(ri) + 1;
        }

        return 0;
    }

    std::vector<Rule> rules;
    int scanCount{28};
};

} // namespace Synergy
