#pragma once
#include <unordered_map>
#include <string>
#include <bitset>
#include <iostream>
#include <cstdlib>

#ifndef INLINE_FORCE
#define INLINE_FORCE __attribute__((always_inline)) inline
#endif


// Bitmap positions for all lockable items (expandable)
enum ItemID : size_t {
    // Vouchers
    OVERSTOCK_PLUS = 0,
    LIQUIDATION = 1,
    GLOW_UP = 2,
    REROLL_GLUT = 3,
    OMEN_GLOBE = 4,
    OBSERVATORY = 5,
    NACHO_TONG = 6,
    RECYCLOMANCY = 7,
    TAROT_TYCOON = 8,
    PLANET_TYCOON = 9,
    MONEY_TREE = 10,
    ANTIMATTER = 11,
    ILLUSION = 12,
    PETROGLYPH = 13,
    RETCON = 14,
    PALETTE = 15,
    
    // Tags
    NEGATIVE_TAG = 16,
    FOIL_TAG = 17,
    HOLOGRAPHIC_TAG = 18,
    POLYCHROME_TAG = 19,
    RARE_TAG = 20,
    
    // Jokers (commonly locked ones)
    GOLDEN_TICKET = 21,
    MR_BONES = 22,
    ACROBAT = 23,
    SOCK_AND_BUSKIN = 24,
    SWASHBUCKLER = 25,
    TROUBADOUR = 26,
    CERTIFICATE = 27,
    SMEARED_JOKER = 28,
    THROWBACK = 29,
    HANGING_CHAD = 30,
    ROUGH_GEM = 31,
    BLOODSTONE = 32,
    ARROWHEAD = 33,
    ONYX_AGATE = 34,
    GLASS_JOKER = 35,
    SHOWMAN = 36,
    FLOWER_POT = 37,
    BLUEPRINT = 38,
    WEE_JOKER = 39,
    MERRY_ANDY = 40,
    OOPS_ALL_6S = 41,
    THE_IDOL = 42,
    SEEING_DOUBLE = 43,
    MATADOR = 44,
    HIT_THE_ROAD = 45,
    THE_DUO = 46,
    THE_TRIO = 47,
    THE_FAMILY = 48,
    THE_ORDER = 49,
    THE_TRIBE = 50,
    STUNTMAN = 51,
    INVISIBLE_JOKER = 52,
    BRAINSTORM = 53,
    SATELLITE = 54,
    SHOOT_THE_MOON = 55,
    DRIVERS_LICENSE = 56,
    CARTOMANCER = 57,
    ASTRONOMER = 58,
    BURNT_JOKER = 59,
    BOOTSTRAPS = 60,
    
    // Additional items from initLocks
    THE_MOUTH = 61,
    THE_FISH = 62,
    THE_WALL = 63,
    THE_HOUSE = 64,
    THE_MARK = 65,
    THE_WHEEL = 66,
    THE_ARM = 67,
    THE_WATER = 68,
    THE_NEEDLE = 69,
    THE_FLINT = 70,
    STANDARD_TAG = 71,
    METEOR_TAG = 72,
    BUFFOON_TAG = 73,
    HANDY_TAG = 74,
    GARBAGE_TAG = 75,
    ETHEREAL_TAG = 76,
    TOP_UP_TAG = 77,
    ORBITAL_TAG = 78,
    THE_TOOTH = 79,
    THE_EYE = 80,
    THE_PLANT = 81,
    THE_SERPENT = 82,
    THE_OX = 83,
    PLANET_X = 84,
    CERES = 85,
    ERIS = 86,
    FIVE_OF_A_KIND = 87,
    FLUSH_HOUSE = 88,
    FLUSH_FIVE = 89,
    STONE_JOKER = 90,
    STEEL_JOKER = 91,
    LUCKY_CAT = 92,
    CAVENDISH = 93,
    
    // Tarot cards that get locked/unlocked during pack generation
    THE_FOOL = 94,
    THE_MAGICIAN = 95,
    THE_HIGH_PRIESTESS = 96,
    THE_EMPRESS = 97,
    THE_EMPEROR = 98,
    THE_HIEROPHANT = 99,
    THE_LOVERS = 100,
    THE_CHARIOT = 101,
    JUSTICE = 102,
    THE_HERMIT = 103,
    THE_WHEEL_OF_FORTUNE = 104,
    STRENGTH_CARD = 105,
    THE_HANGED_MAN = 106,
    DEATH = 107,
    TEMPERANCE = 108,
    THE_DEVIL = 109,
    THE_TOWER = 110,
    THE_STAR_CARD = 111,
    THE_MOON = 112,
    THE_SUN_CARD = 113,
    JUDGEMENT = 114,
    THE_WORLD = 115,
    
    // Planet cards
    MERCURY = 116,
    VENUS = 117,
    EARTH = 118,
    MARS = 119,
    JUPITER = 120,
    SATURN = 121,
    URANUS = 122,
    NEPTUNE = 123,
    PLUTO = 124,
    PLANET_X_CARD = 125,
    CERES_CARD = 126,
    ERIS_CARD = 127,
    
    // Spectral cards
    FAMILIAR = 128,
    GRIM = 129,
    INCANTATION = 130,
    TALISMAN = 131,
    AURA = 132,
    WRAITH = 133,
    SIGIL = 134,
    OUIJA = 135,
    ECTOPLASM = 136,
    IMMOLATE_CARD = 137,
    ANKH = 138,
    DEJA_VU = 139,
    HEX = 140,
    TRANCE = 141,
    MEDIUM = 142,
    CRYPTID = 143,
    
    // Special cards
    THE_SOUL_CARD = 144,
    BLACK_HOLE_CARD = 145
    // Can expand up to 256 or more as needed
};

// Global lookup table for string -> bit position
static const std::unordered_map<std::string, ItemID> ITEM_TO_ID = {
    {"Overstock Plus", OVERSTOCK_PLUS},
    {"Liquidation", LIQUIDATION},
    {"Glow Up", GLOW_UP},
    {"Reroll Glut", REROLL_GLUT},
    {"Omen Globe", OMEN_GLOBE},
    {"Observatory", OBSERVATORY},
    {"Nacho Tong", NACHO_TONG},
    {"Recyclomancy", RECYCLOMANCY},
    {"Tarot Tycoon", TAROT_TYCOON},
    {"Planet Tycoon", PLANET_TYCOON},
    {"Money Tree", MONEY_TREE},
    {"Antimatter", ANTIMATTER},
    {"Illusion", ILLUSION},
    {"Petroglyph", PETROGLYPH},
    {"Retcon", RETCON},
    {"Palette", PALETTE},
    {"Negative Tag", NEGATIVE_TAG},
    {"Foil Tag", FOIL_TAG},
    {"Holographic Tag", HOLOGRAPHIC_TAG},
    {"Polychrome Tag", POLYCHROME_TAG},
    {"Rare Tag", RARE_TAG},
    {"Golden Ticket", GOLDEN_TICKET},
    {"Mr. Bones", MR_BONES},
    {"Acrobat", ACROBAT},
    {"Sock and Buskin", SOCK_AND_BUSKIN},
    {"Swashbuckler", SWASHBUCKLER},
    {"Troubadour", TROUBADOUR},
    {"Certificate", CERTIFICATE},
    {"Smeared Joker", SMEARED_JOKER},
    {"Throwback", THROWBACK},
    {"Hanging Chad", HANGING_CHAD},
    {"Rough Gem", ROUGH_GEM},
    {"Bloodstone", BLOODSTONE},
    {"Arrowhead", ARROWHEAD},
    {"Onyx Agate", ONYX_AGATE},
    {"Glass Joker", GLASS_JOKER},
    {"Showman", SHOWMAN},
    {"Flower Pot", FLOWER_POT},
    {"Blueprint", BLUEPRINT},
    {"Wee Joker", WEE_JOKER},
    {"Merry Andy", MERRY_ANDY},
    {"Oops! All 6s", OOPS_ALL_6S},
    {"The Idol", THE_IDOL},
    {"Seeing Double", SEEING_DOUBLE},
    {"Matador", MATADOR},
    {"Hit the Road", HIT_THE_ROAD},
    {"The Duo", THE_DUO},
    {"The Trio", THE_TRIO},
    {"The Family", THE_FAMILY},
    {"The Order", THE_ORDER},
    {"The Tribe", THE_TRIBE},
    {"Stuntman", STUNTMAN},
    {"Invisible Joker", INVISIBLE_JOKER},
    {"Brainstorm", BRAINSTORM},
    {"Satellite", SATELLITE},
    {"Shoot the Moon", SHOOT_THE_MOON},
    {"Driver's License", DRIVERS_LICENSE},
    {"Cartomancer", CARTOMANCER},
    {"Astronomer", ASTRONOMER},
    {"Burnt Joker", BURNT_JOKER},
    {"Bootstraps", BOOTSTRAPS},
    {"The Mouth", THE_MOUTH},
    {"The Fish", THE_FISH},
    {"The Wall", THE_WALL},
    {"The House", THE_HOUSE},
    {"The Mark", THE_MARK},
    {"The Wheel", THE_WHEEL},
    {"The Arm", THE_ARM},
    {"The Water", THE_WATER},
    {"The Needle", THE_NEEDLE},
    {"The Flint", THE_FLINT},
    {"Standard Tag", STANDARD_TAG},
    {"Meteor Tag", METEOR_TAG},
    {"Buffoon Tag", BUFFOON_TAG},
    {"Handy Tag", HANDY_TAG},
    {"Garbage Tag", GARBAGE_TAG},
    {"Ethereal Tag", ETHEREAL_TAG},
    {"Top-up Tag", TOP_UP_TAG},
    {"Orbital Tag", ORBITAL_TAG},
    {"The Tooth", THE_TOOTH},
    {"The Eye", THE_EYE},
    {"The Plant", THE_PLANT},
    {"The Serpent", THE_SERPENT},
    {"The Ox", THE_OX},
    {"Planet X", PLANET_X},
    {"Ceres", CERES},
    {"Eris", ERIS},
    {"Five of a Kind", FIVE_OF_A_KIND},
    {"Flush House", FLUSH_HOUSE},
    {"Flush Five", FLUSH_FIVE},
    {"Stone Joker", STONE_JOKER},
    {"Steel Joker", STEEL_JOKER},
    {"Lucky Cat", LUCKY_CAT},
    {"Cavendish", CAVENDISH},
    
    // Tarot cards
    {"The Fool", THE_FOOL},
    {"The Magician", THE_MAGICIAN},
    {"The High Priestess", THE_HIGH_PRIESTESS},
    {"The Empress", THE_EMPRESS},
    {"The Emperor", THE_EMPEROR},
    {"The Hierophant", THE_HIEROPHANT},
    {"The Lovers", THE_LOVERS},
    {"The Chariot", THE_CHARIOT},
    {"Justice", JUSTICE},
    {"The Hermit", THE_HERMIT},
    {"The Wheel of Fortune", THE_WHEEL_OF_FORTUNE},
    {"Strength", STRENGTH_CARD},
    {"The Hanged Man", THE_HANGED_MAN},
    {"Death", DEATH},
    {"Temperance", TEMPERANCE},
    {"The Devil", THE_DEVIL},
    {"The Tower", THE_TOWER},
    {"The Star", THE_STAR_CARD},
    {"The Moon", THE_MOON},
    {"The Sun", THE_SUN_CARD},
    {"Judgement", JUDGEMENT},
    {"The World", THE_WORLD},
    
    // Planet cards
    {"Mercury", MERCURY},
    {"Venus", VENUS},
    {"Earth", EARTH},
    {"Mars", MARS},
    {"Jupiter", JUPITER},
    {"Saturn", SATURN},
    {"Uranus", URANUS},
    {"Neptune", NEPTUNE},
    {"Pluto", PLUTO},
    {"Planet X", PLANET_X_CARD},
    {"Ceres", CERES_CARD},
    {"Eris", ERIS_CARD},
    
    // Spectral cards
    {"Familiar", FAMILIAR},
    {"Grim", GRIM},
    {"Incantation", INCANTATION},
    {"Talisman", TALISMAN},
    {"Aura", AURA},
    {"Wraith", WRAITH},
    {"Sigil", SIGIL},
    {"Ouija", OUIJA},
    {"Ectoplasm", ECTOPLASM},
    {"Immolate", IMMOLATE_CARD},
    {"Ankh", ANKH},
    {"Deja Vu", DEJA_VU},
    {"Hex", HEX},
    {"Trance", TRANCE},
    {"Medium", MEDIUM},
    {"Cryptid", CRYPTID},
    
    // Special cards
    {"The Soul", THE_SOUL_CARD},
    {"Black Hole", BLACK_HOLE_CARD}
};

// Use a bitset with enough bits for all items (256 should be plenty)
using LockBitset = std::bitset<256>;

// Fast bitset operations
INLINE_FORCE void lockItem(LockBitset& lockBitmap, ItemID itemId) {
    lockBitmap.set(itemId);
}

INLINE_FORCE void lockItem(LockBitset& lockBitmap, const std::string& itemName) {
    auto it = ITEM_TO_ID.find(itemName);
    if (it != ITEM_TO_ID.end()) {
        lockBitmap.set(it->second);
    } else {
        std::cerr << "ERROR: Trying to lock unknown item: '" << itemName << "'" << std::endl;
        std::cerr << "This item needs to be added to ITEM_TO_ID mapping in bitmap_lock.hpp" << std::endl;
        std::abort();
    }
}

INLINE_FORCE void unlockItem(LockBitset& lockBitmap, ItemID itemId) {
    lockBitmap.reset(itemId);
}

INLINE_FORCE void unlockItem(LockBitset& lockBitmap, const std::string& itemName) {
    auto it = ITEM_TO_ID.find(itemName);
    if (it != ITEM_TO_ID.end()) {
        lockBitmap.reset(it->second);
    } else {
        std::cerr << "ERROR: Trying to unlock unknown item: '" << itemName << "'" << std::endl;
        std::cerr << "This item needs to be added to ITEM_TO_ID mapping in bitmap_lock.hpp" << std::endl;
        std::abort();
    }
}

INLINE_FORCE bool isItemLocked(const LockBitset& lockBitmap, ItemID itemId) {
    return lockBitmap.test(itemId);
}

INLINE_FORCE bool isItemLocked(const LockBitset& lockBitmap, const std::string& itemName) {
    auto it = ITEM_TO_ID.find(itemName);
    if (it != ITEM_TO_ID.end()) {
        return lockBitmap.test(it->second);
    }
    return false; // Item not found, assume not locked
}