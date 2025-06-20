#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include <memory>
#include "instance.hpp"
#include "util.hpp"
#include "items.hpp"
#include "pack.hpp"
#include "functions.hpp"

void simulate(Instance inst)
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
        output += "Boss: " + inst.nextBoss(a) + "\n";
        auto voucher = inst.nextVoucher(a);
        output += "Voucher: " + voucher + "\n";
        inst.lock(voucher);
        // Unlock next level voucher
        for (int i = 0; i < VOUCHERS.size(); i += 2)
        {
            if (VOUCHERS[i] == voucher)
            {
                inst.unlock(VOUCHERS[i+1]);
            };
        };
        output += "Tags: " + inst.nextTag(a) + ", " + inst.nextTag(a) + "\n";

        output += "Shop Queue: \n";
        for (int q = 1; q <= cardsPerAnte[a - 1]; q++)
        {
            output += std::to_string(q) + ") ";
            auto item = inst.nextShopItem(a);
            if (item.type == "Joker")
            {
                if (item.jokerData.stickers.eternal)
                    output += "Eternal ";
                if (item.jokerData.stickers.perishable)
                    output += "Perishable ";
                if (item.jokerData.stickers.rental)
                    output += "Rental ";
                if (item.jokerData.edition != "No Edition")
                    output += item.jokerData.edition + " ";
            }
            output += item.item + "\n";
        }

        output += "\nPacks: \n";
        int numPacks = (a == 1) ? 4 : 6;
        for (int p = 1; p <= numPacks; p++)
        {
            std::string pack = inst.nextPack(a);
            auto packData = openPack(inst, a, packInfo(pack));
            output += anyPackToString(packData);
            // output += pack + " - ";
            // auto packData = packInfo(pack);
            // if (packData.type == "Celestial Pack")
            // {
            //     auto cards = inst.nextCelestialPack(packData.size, a);
            //     for (int c = 0; c < packData.size; c++)
            //     {
            //         output += cards[c];
            //         output += (c + 1 != packData.size) ? ", " : "";
            //     }
            // }
            // if (packData.type == "Arcana Pack")
            // {
            //     auto cards = inst.nextArcanaPack(packData.size, a);
            //     for (int c = 0; c < packData.size; c++)
            //     {
            //         output += cards[c];
            //         output += (c + 1 != packData.size) ? ", " : "";
            //     }
            // }
            // if (packData.type == "Spectral Pack")
            // {
            //     auto cards = inst.nextSpectralPack(packData.size, a);
            //     for (int c = 0; c < packData.size; c++)
            //     {
            //         output += cards[c];
            //         output += (c + 1 != packData.size) ? ", " : "";
            //     }
            // }
            // if (packData.type == "Buffoon Pack")
            // {
            //     auto cards = inst.nextBuffoonPack(packData.size, a);
            //     for (int c = 0; c < packData.size; c++)
            //     {
            //         auto joker = cards[c];
            //         if (joker.stickers.eternal)
            //             output += "Eternal ";
            //         if (joker.stickers.perishable)
            //             output += "Perishable ";
            //         if (joker.stickers.rental)
            //             output += "Rental ";
            //         if (joker.edition != "No Edition")
            //             output += joker.edition + " ";
            //         output += joker.joker;
            //         output += (c + 1 != packData.size) ? ", " : "";
            //     }
            // }
            // if (packData.type == "Standard Pack")
            // {
            //     auto cards = inst.nextStandardPack(packData.size, a);
            //     for (int c = 0; c < packData.size; c++)
            //     {
            //         auto card = cards[c];
            //         if (card.seal != "No Seal")
            //             output += card.seal + " ";
            //         if (card.edition != "No Edition")
            //             output += card.edition + " ";
            //         if (card.enhancement != "No Enhancement")
            //             output += card.enhancement + " ";
            //         auto rank = card.base[2];
            //         if (rank == 'T')
            //             output += "10";
            //         else if (rank == 'J')
            //             output += "Jack";
            //         else if (rank == 'Q')
            //             output += "Queen";
            //         else if (rank == 'K')
            //             output += "King";
            //         else if (rank == 'A')
            //             output += "Ace";
            //         else
            //             output += rank;
            //         output += " of ";
            //         auto suit = card.base[0];
            //         if (suit == 'C')
            //             output += "Clubs";
            //         else if (suit == 'S')
            //             output += "Spades";
            //         else if (suit == 'D')
            //             output += "Diamonds";
            //         else if (suit == 'H')
            //             output += "Hearts";
            //         output += (c + 1 != packData.size) ? ", " : "";
            //     }
            // }
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
        std::vector<bool> selectedOptions(61, true);
        Instance inst = initInstance(startSeedStr, selectedOptions);
        simulate(inst);
    }
}