#include "describe_simulator.hpp"
#include <sstream>
#include <json/json.h>

namespace DescribeSim {

static std::string escape_json(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string describe_seed_timing(const std::string& seed) {
    try {
        Instance::Instance inst(seed);
        EnvConfig e = getGlobalEnv();
        if (!e.deck.empty()) inst.setDeck(e.deck);
        if (!e.stake.empty()) inst.setStake(e.stake);
        inst.setShowman(e.showman);
        inst.setSixesFactor(e.sixesFactor);
        inst.setVersion(e.version);
        inst.setForceAllContent(e.forceAllContent);
        inst.initLocks(1, e.freshProfile, e.freshRun);

        // Basic structure
        Json::Value root;
        root["index"] = 1; // placeholder; filters may override
        root["name"] = "describe_simulator_prediction";
        Json::Value cards(Json::arrayValue);

        // Predict tag
        auto tag = inst.nextTag_enum(1);
        Json::Value tagnode;
        tagnode["name"] = Items::toString(tag);
        tagnode["slot"] = "tag";
        tagnode["position"] = -1;
        tagnode["count"] = 1;
        tagnode["turn"] = 0;
        tagnode["when"] = "pack_open";
        cards.append(tagnode);

        // Arcana pack
        auto pack = inst.nextArcanaPack_enum(5,1);
        for (int i = 0; i < (int)pack.tarots.size(); ++i) {
            std::string cname;
            if (pack.isSpectral[i]) cname = Items::toString(pack.spectrals[i]);
            else cname = Items::toString(pack.tarots[i]);
            Json::Value n;
            n["name"] = cname;
            n["slot"] = "tarot";
            n["position"] = i;
            n["count"] = 1;
            // Conservative prediction: played on turn 1
            n["turn"] = 1;
            n["when"] = "predicted_play";
            cards.append(n);
        }

        // Joker (next)
        auto joker = inst.nextJoker_enum("sou",1,false);
        Json::Value jn;
        jn["name"] = Items::toString(joker.joker);
        jn["slot"] = "joker";
        jn["position"] = -1;
        jn["count"] = 1;
        jn["turn"] = 1;
        jn["when"] = "shop_joker";
        cards.append(jn);

        root["cards"] = cards;

        Json::StreamWriterBuilder b;
        b["indentation"] = "  ";
        std::string out = Json::writeString(b, root);
        return out;
    } catch (...) {
        return std::string();
    }
}

} // namespace DescribeSim
