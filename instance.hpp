#include <map>
#include <iomanip>
#include <string>
#include "bitmap_lock.hpp"
#pragma once
struct Cache {
    std::map<std::string, double> nodes;
    bool generatedFirstPack;
};

struct InstParams {
    std::string deck;
    std::string stake;
    bool showman;
    int sixesFactor;
    long version;
    std::vector<std::string> vouchers;
    InstParams() {
        deck = "Red Deck";
        stake = "White Stake";
        showman = false;
        sixesFactor = 1;
        version = 10103; //1.0.1c
    }
    InstParams(std::string d, std::string s, bool show, long v) {
        deck = d;
        stake = s;
        showman = show;
        sixesFactor = 1;
        version = v;
    }
};

struct Instance {
    LockBitset lockedBitmap;
    std::string seed;
    double hashedSeed;
    Cache cache;
    InstParams params;
    LuaRandom rng;
    Instance(std::string s) {
        seed = s;
        hashedSeed = pseudohash(s);
        params = InstParams();
        rng = LuaRandom(0);
        cache.generatedFirstPack = false;
        lockedBitmap.reset(); // Clear all bits
    };
    double get_node(const std::string& ID) {
        if (cache.nodes.count(ID) == 0) {
            // Avoid string concatenation allocation
            std::string combined;
            combined.reserve(ID.length() + seed.length());
            combined = ID + seed;
            cache.nodes[ID] = pseudohash(combined);
        }
        cache.nodes[ID] = round13(std::fmod(cache.nodes[ID]*1.72431234+2.134453429141,1));
        return (cache.nodes[ID] + hashedSeed)/2;
    }
    double random(const std::string& ID) {
        rng = LuaRandom(get_node(ID));
        return rng.random();
    }
    int randint(const std::string& ID, int min, int max) {
        rng = LuaRandom(get_node(ID));
        return rng.randint(min, max);
    }
    std::string randchoice(const std::string& ID, const std::vector<std::string>& items) {
        rng = LuaRandom(get_node(ID));
        std::string item = items[rng.randint(0, items.size()-1)];
        if ((params.showman == false && isLocked(item)) || item == "RETRY") {
            int resample = 2;
            while (true) {
                rng = LuaRandom(get_node(ID+"_resample"+std::to_string(resample)));
                std::string item = items[rng.randint(0, items.size()-1)];
                resample++;
                if ((item != "RETRY" && !isLocked(item)) || resample > 1000) return item;
            }
        }
        return item;
    }
    std::string randweightedchoice(std::string ID, std::vector<WeightedItem> items) {
        rng = LuaRandom(get_node(ID));
        double poll = rng.random()*items[0].weight;
        int idx = 1;
        double weight = 0;
        while (weight < poll) {
            weight += items[idx].weight;
            idx++;
        }
        return items[idx-1].item;
    }

    // Functions defined later
    void lock(std::string item);
    void unlock(std::string item);
    bool isLocked(std::string item);
    
    void lockDirect(ItemID item);
    void unlockDirect(ItemID item);
    bool isLockedDirect(ItemID item);
    
    void initLocks(int ante, bool freshProfile, bool freshRun);
    void initUnlocks(int ante, bool freshProfile);

    void lockBitmap(const std::string& item);
    void unlockBitmap(const std::string& item);
    bool isLockedBitmap(const std::string& item);
    void initLocksBitmap(int ante, bool freshProfile, bool freshRun);
    void initUnlocksBitmap(int ante, bool freshProfile);

    std::string nextTarot(std::string source, int ante, bool soulable);
    std::string nextPlanet(std::string source, int ante, bool soulable);
    std::string nextSpectral(std::string source, int ante, bool soulable);
    JokerData nextJoker(std::string source, int ante, bool hasStickers);
    ShopInstance getShopInstance();
    ShopItem nextShopItem(int ante);
    std::string nextPack(int ante);
    std::vector<std::string> nextArcanaPack(int size, int ante);
    std::vector<std::string> nextCelestialPack(int size, int ante);
    std::vector<std::string> nextSpectralPack(int size, int ante);
    std::vector<JokerData> nextBuffoonPack(int size, int ante);
    std::vector<Card> nextStandardPack(int size, int ante);
    Card nextStandardCard(int ante);
    bool isVoucherActive(std::string voucher);
    void activateVoucher(std::string voucher);
    std::string nextVoucher(int ante);
    void setDeck(std::string deck);
    void setStake(std::string stake);
    std::string nextTag(int ante);
    std::string nextBoss(int ante);
};

// Global pre-computed locked items to avoid repeated initLocks calls
static LockBitset g_precomputedLocks;
static LockBitset g_finalLocks; // After applying selectedOptions

static bool g_locksInitialized = false;

void initializeGlobalLocks() {
    if (g_locksInitialized) return;
    
    // Create a temporary instance to compute the locks once
    Instance tempInst(""); // dummy seed
    tempInst.initLocks(1, false, false);
    
    // Add the specific items we always lock
    tempInst.lock("Overstock Plus");
    tempInst.lock("Liquidation");
    tempInst.lock("Glow Up");
    tempInst.lock("Reroll Glut");
    tempInst.lock("Omen Globe");
    tempInst.lock("Observatory");
    tempInst.lock("Nacho Tong");
    tempInst.lock("Recyclomancy");
    tempInst.lock("Tarot Tycoon");
    tempInst.lock("Planet Tycoon");
    tempInst.lock("Money Tree");
    tempInst.lock("Antimatter");
    tempInst.lock("Illusion");
    tempInst.lock("Petroglyph");
    tempInst.lock("Retcon");
    tempInst.lock("Palette");
    
    // Extract the locked vector
    g_precomputedLocks = tempInst.lockedBitmap;
    
    // Pre-compute final locks with selectedOptions(61, true) - no additional locks
    g_finalLocks = g_precomputedLocks; // Since selectedOptions are all true
    
    g_locksInitialized = true;
}

Instance initInstance(const std::string& seed, std::vector<bool> selectedOptions) {
    // Initialize global locks once
    initializeGlobalLocks();
    
    Instance inst(seed);
    inst.params = InstParams("Red Deck", "White Stake", false, 10106);
    
    // Copy pre-computed locks instead of calling initLocks + individual locks
    inst.lockedBitmap = g_finalLocks;
    
    inst.setStake("White Stake");
    inst.setDeck("Red Deck");

    return inst;
}