#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include "env.hpp"

// Minimal copy of the lightweight parser logic from immolate.cpp for testing
EnvConfig parseEnvLightweight(const std::string &txt) {
    EnvConfig e;
    auto find_str = [&](const std::string& key) -> std::string {
        auto pos = txt.find('"' + key + '"');
        if (pos == std::string::npos) return std::string();
        auto colon = txt.find(':', pos);
        if (colon == std::string::npos) return std::string();
        auto start = txt.find_first_not_of(" \t\n\r", colon+1);
        if (start == std::string::npos) return std::string();
        if (txt[start] == '"') {
            auto end = txt.find('"', start+1);
            if (end == std::string::npos) return std::string();
            return txt.substr(start+1, end-start-1);
        } else {
            auto end = txt.find_first_of(",}\n\r", start);
            if (end == std::string::npos) end = txt.size();
            return txt.substr(start, end-start);
        }
    };

    std::string v;
    v = find_str("deck"); if (!v.empty()) e.deck = v;
    v = find_str("stake"); if (!v.empty()) e.stake = v;
    v = find_str("showman"); if (!v.empty()) e.showman = (v.find("true") != std::string::npos);
    v = find_str("sixesFactor"); if (!v.empty()) e.sixesFactor = std::stoi(v);
    v = find_str("version"); if (!v.empty()) e.version = std::stol(v);
    v = find_str("forceAllContent"); if (!v.empty()) e.forceAllContent = (v.find("true") != std::string::npos);
    v = find_str("freshProfile"); if (!v.empty()) e.freshProfile = (v.find("true") != std::string::npos);
    v = find_str("freshRun"); if (!v.empty()) e.freshRun = (v.find("true") != std::string::npos);

    auto posSo = txt.find("\"selectedOptions\"");
    if (posSo != std::string::npos) {
        auto bracket = txt.find('[', posSo);
        if (bracket != std::string::npos) {
            auto endb = txt.find(']', bracket);
            if (endb != std::string::npos && endb > bracket) {
                std::string body = txt.substr(bracket+1, endb - bracket - 1);
                std::vector<std::string> parts;
                size_t p = 0;
                while (p < body.size()) {
                    auto comma = body.find(',', p);
                    if (comma == std::string::npos) comma = body.size();
                    std::string token = body.substr(p, comma - p);
                    auto l = token.find_first_not_of(" \t\n\r");
                    auto r = token.find_last_not_of(" \t\n\r");
                    if (l != std::string::npos && r != std::string::npos) token = token.substr(l, r - l + 1);
                    else token = "";
                    if (!token.empty()) parts.push_back(token);
                    p = comma + 1;
                }
                bool allBool = true; bool allInt = true;
                for (auto &tkn : parts) {
                    std::string tl = tkn;
                    for (auto &c : tl) c = (char)std::tolower(c);
                    if (!(tl == "true" || tl == "false")) allBool = false;
                    try { std::size_t idx; std::stoul(tkn); } catch(...) { allInt = false; }
                }
                if (allBool && parts.size() == 61) {
                    e.selectedOptions.clear(); e.selectedOptions.resize(61);
                    for (size_t i = 0; i < 61 && i < parts.size(); ++i) {
                        auto tl = parts[i]; for (auto &c : tl) c = (char)std::tolower(c);
                        e.selectedOptions[i] = (tl == "true");
                    }
                    e.selectedOptionsSet = true;
                } else if (allInt) {
                    e.selectedOptions = std::vector<bool>(61, false);
                    for (auto &tkn : parts) {
                        try {
                            int idx = std::stoi(tkn);
                            if (idx >= 0 && idx < 61) e.selectedOptions[idx] = true;
                        } catch (...) { }
                    }
                    e.selectedOptionsSet = true;
                }
            }
        }
    }
    return e;
}

void printSelected(const EnvConfig &e) {
    std::cout << "selectedOptionsSet=" << (e.selectedOptionsSet?"true":"false") << ", count=" << e.selectedOptions.size() << '\n';
    for (size_t i = 0; i < e.selectedOptions.size(); ++i) {
        if (e.selectedOptions[i]) std::cout << i << ",";
    }
    std::cout << '\n';
}

int main() {
    // Build two example env JSON strings
    std::ostringstream btrue;
    btrue << "{\n  \"deck\": \"Red Deck\",\n  \"selectedOptions\": [";
    for (int i = 0; i < 61; ++i) { if (i) btrue << ", "; btrue << (i%2==0?"true":"false"); }
    btrue << "]\n}\n";

    std::ostringstream idxs;
    idxs << "{\n  \"deck\": \"Red Deck\",\n  \"selectedOptions\": [";
    // include indices 0,2,4,60
    idxs << "0,2,4,60";
    idxs << "]\n}\n";

    std::string s1 = btrue.str();
    std::string s2 = idxs.str();

    std::cout << "Parsing boolean array sample:\n" << s1 << std::endl;
    EnvConfig e1 = parseEnvLightweight(s1);
    printSelected(e1);

    std::cout << "Parsing indices array sample:\n" << s2 << std::endl;
    EnvConfig e2 = parseEnvLightweight(s2);
    printSelected(e2);

    return 0;
}
