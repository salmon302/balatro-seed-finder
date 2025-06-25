#include <string>
#include <vector>
#include "instance.hpp"
#include "items_utils.hpp"
#include "items.hpp"
#include "items_to_string.hpp"

// Enum-based pack handling structures
struct EnumAnyPack {
    Items::Pack packType;
    Items::MixedArcanaPack tarots;
    std::vector<Items::Planet> planets;
    std::vector<Items::Spectral> spectrals;
    std::vector<Items::OptimizedJokerData> jokers;
    std::vector<Items::CardEnum> cards;  // Keep Card as-is for now
};

std::string enumAnyPackToString(EnumAnyPack p) {
    std::string out = "";
    out += Items::toString(p.packType);
    if(p.packType == Items::Pack::BUFFOON_PACK) {
        for(int i = 0; i < p.jokers.size(); i++) {
            auto j = p.jokers[i];
            out += "\t";
            // O(1) enum-to-string conversion!
            out += Items::toString(j.edition);
            out += " ";
            out += Items::toString(j.joker);
            if(j.eternal) {
                out += " Eternal";
            }
            if(j.perishable) {
                out += " Perishable";
            }
            if(j.rental) {
                out += " Rental";
            }
        }
    } else if(p.packType == Items::Pack::STANDARD_PACK) {
        for(int i = 0; i < p.cards.size(); i++) {
            auto j = p.cards[i];
            auto en = Items::toString(j.enhancement);
            auto ed = Items::toString(j.edition);
            auto s = Items::toString(j.seal);
            out += std::string("\t") + en + " " + j.base + " " + ed + " " + s;
        }
    } else if(p.packType == Items::Pack::ARCANA_PACK) {
        for(int i = 0; i < p.tarots.tarots.size(); i++) {
            out += "\t";

            if(p.tarots.isSpectral[i]) {
                out += Items::toString(p.tarots.spectrals[i]);
            } else {
                out += Items::toString(p.tarots.tarots[i]);
            }            
        }
    } else if(p.packType == Items::Pack::CELESTIAL_PACK) {
        for(int i = 0; i < p.planets.size(); i++) {
            out += "\t";
            out += Items::toString(p.planets[i]);
        }
    } else if(p.packType == Items::Pack::SPECTRAL_PACK) {
        for(int i = 0; i < p.spectrals.size(); i++) {
            out += "\t";
            out += Items::toString(p.spectrals[i]);  // O(1) conversion!
        }
    }
    return out;
}

EnumAnyPack openEnumPack(Instance::Instance& inst, int ante, Items::NextPackData packInfo) {
    EnumAnyPack p;
    p.packType = packInfo.type;
    if(packInfo.type == Items::Pack::ARCANA_PACK) {
        p.tarots = inst.nextArcanaPack_enum(packInfo.size, ante);
        p.planets = {};
        p.spectrals = {};
        p.cards = {};
        p.jokers = {};
    }
    else if(packInfo.type == Items::Pack::BUFFOON_PACK) {
        p.jokers = inst.nextBuffoonPack_enum(packInfo.size, ante);
        p.tarots = {};
        p.planets = {};
        p.spectrals = {};
        p.cards = {};
    }
    else if(packInfo.type == Items::Pack::SPECTRAL_PACK) {
        p.spectrals = inst.nextSpectralPack_enum(packInfo.size, ante);
        p.tarots = {};
        p.planets = {};
        p.cards = {};
        p.jokers = {};
    }
    else if(packInfo.type == Items::Pack::STANDARD_PACK) {
        p.cards = inst.nextStandardPack_enum(packInfo.size, ante);
        p.tarots = {};
        p.planets = {};
        p.spectrals = {};
        p.jokers = {};
    }
    else if(packInfo.type == Items::Pack::CELESTIAL_PACK) {
        p.planets = inst.nextCelestialPack_enum(packInfo.size, ante);
        p.tarots = {};
        p.spectrals = {};
        p.cards = {};
        p.jokers = {};
    } else {
        std::cout << "NO SUCH PACK " << static_cast<uint8_t>(packInfo.type) << "!! !!" << std::endl;
        std::abort();
    }
    return p;
}

void simulate_enum(Instance::Instance inst)
{
    std::string output;
    std::vector<int> cardsPerAnte = {15,50,50,50,50,50,50,50};
    inst.setStake("White Stake");
    inst.setDeck("Red Deck");
    int ante = 8;
    for (int a = 1; a <= ante; a++)
    {
        inst.initUnlocks(a, false);
        output += "==ANTE " + std::to_string(a) + "==\n";
        
        // Boss generation with enum - O(1) conversion!
        auto boss = inst.nextBoss_enum(a);
        output += "Boss: ";
        output += Items::toString(boss);
        output += "\n";
        
        // Voucher generation with enum - O(1) conversion!
        auto voucher = inst.nextVoucher_enum(a);
        output += "Voucher: ";
        output += Items::toString(voucher);
        output += "\n";
        inst.activateVoucher_enum(voucher);
        
        // Tags with enum - O(1) conversion!
        auto tag1 = inst.nextTag_enum(a);
        auto tag2 = inst.nextTag_enum(a);
        output += "Tags: ";
        output += Items::toString(tag1);
        output += ", ";
        output += Items::toString(tag2);
        output += "\n";

        output += "Shop Queue: \n";
        for (int q = 1; q <= cardsPerAnte[a - 1]; q++)
        {
            output += std::to_string(q) + ") ";
            auto item = inst.nextShopItem_enum(a);
            
            // Handle different shop item types using the correct interface
            if (item.type == Items::OptimizedShopItem::Type::JOKER) {
                auto& jokerData = item.joker_data;
                if (jokerData.eternal)
                    output += "Eternal ";
                if (jokerData.perishable)
                    output += "Perishable ";
                if (jokerData.rental)
                    output += "Rental ";
                if (jokerData.edition != Items::Edition::NO_EDITION) {
                    output += Items::toString(jokerData.edition);  // O(1) conversion!
                    output += " ";
                }
                output += Items::toString(item.item.joker);  // O(1) conversion!
            } else if (item.type == Items::OptimizedShopItem::Type::TAROT) {
                output += Items::toString(item.item.tarot);  // O(1) conversion!
            } else if (item.type == Items::OptimizedShopItem::Type::PLANET) {
                output += Items::toString(item.item.planet);  // O(1) conversion!
            } else if (item.type == Items::OptimizedShopItem::Type::SPECTRAL) {
                output += Items::toString(item.item.spectral);  // O(1) conversion!
            }
            output += "\n";
        }

        output += "\nPacks: \n";
        int numPacks = (a == 1) ? 4 : 6;
        for (int p = 1; p <= numPacks; p++)
        {
            auto pack = inst.nextPack_enum(a);
            auto packData = Items::convertPackData(pack);
            auto enumPackData = openEnumPack(inst, a, packData);
            output += enumAnyPackToString(enumPackData);
            output += "\n";
        }

        output += "\n";
    };
    std::cout << output << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        std::string startSeedStr = argv[1];
        Instance::Instance inst(startSeedStr);
        inst.initLocks(1, false, false);
        simulate_enum(inst);
    }
    return 0;
}