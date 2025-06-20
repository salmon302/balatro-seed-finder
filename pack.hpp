#pragma once
#include "instance.hpp"
#include "util.hpp"
#include "items.hpp"

struct AnyPack {
    std::string packType;
    std::vector<std::string> others;
    std::vector<JokerData> jokers;
    std::vector<Card> cards;
};

std::string anyPackToString(AnyPack p) {
    std::string out = "";
    out += p.packType;
    if(p.packType == "Buffoon Pack") {
        for(int i = 0; i < p.jokers.size(); i++) {
            auto j = p.jokers[i];
            out += "\t";
            out += j.edition + " " + j.joker;
            if(j.stickers.eternal) {
                out += " Eternal";
            }
            if(j.stickers.perishable) {
                out += " Perishable";
            }
            if(j.stickers.rental) {
                out += " Rental";
            }
        }
    } else if(p.packType == "Standard Pack") {
        for(int i = 0; i < p.cards.size(); i++) {
            auto j = p.cards[i];
            out += "\t" + j.enhancement + " " + j.base + " " + j.edition + " " + j.seal;
        }
    } else {
        for(int i = 0; i < p.others.size(); i++) {
            auto j = p.others[i];
            out += "\t" + j;
        }
    }
    return out;
}

bool packHasJoker(AnyPack p, std::string jokerName, bool negative = false) {
    for(int i = 0; i < p.jokers.size(); i++) {
        return p.jokers[i].joker == jokerName && (p.jokers[i].edition == "Negative" || !negative);
    }
    return false;
}

AnyPack openPack(Instance& inst, int ante, Pack packInfo) {
    AnyPack p;
    p.packType = packInfo.type;
    if(packInfo.type == "Arcana Pack") {
        p.others = inst.nextArcanaPack(packInfo.size, ante);
        p.cards = {};
        p.jokers = {};
    }
    else if(packInfo.type == "Buffoon Pack") {
        p.jokers = inst.nextBuffoonPack(packInfo.size, ante);
        p.cards = {};
        p.others = {};
    }
    else if(packInfo.type == "Spectral Pack") {
        p.others = inst.nextSpectralPack(packInfo.size, ante);
        p.cards = {};
        p.jokers = {};
    }
    else if(packInfo.type == "Standard Pack") {
        p.cards = inst.nextStandardPack(packInfo.size, ante);
        p.others = {};
        p.jokers = {};
    }
    else if(packInfo.type == "Celestial Pack") {
        p.others = inst.nextCelestialPack(packInfo.size, ante);
        p.cards = {};
        p.jokers = {};
    } else {
        std::cout << "NO SUCH PACK " << packInfo.type << std::endl;
        std::abort();
    }
    return p;
}
