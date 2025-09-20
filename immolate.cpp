#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <fstream>
#include <functional>
#include <memory>
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "rand_util.hpp"
#include "debug.hpp"
#include "logger.hpp"

#include "filters/filter_base.hpp"
// Conditional filter inclusion based on preprocessor definition
#ifdef SELECTED_FILTER
#include SELECTED_FILTER
#else
#endif

#define INLINE_FORCE __attribute__((always_inline)) inline

struct SearchStats {
    std::atomic<uint64_t> totalSeeds{0};
    std::atomic<uint64_t> currentSeedNumber{0};
    std::vector<FilterResult> results;
    
    void initializeResults(const std::vector<std::string>& resultNames) {
        results.clear();
        for (const auto& name : resultNames) {
            results.emplace_back(name);
        }
    }
    
    void updateResult(int resultIndex) {
        if (resultIndex > 0 && resultIndex <= static_cast<int>(results.size())) {
            results[resultIndex - 1].count.fetch_add(1);
        }
    }
};


static std::unique_ptr<SearchFilter> g_currentFilter;

SearchFilter* getCurrentFilter() {
    if (!g_currentFilter) {
        g_currentFilter = createFilter(); // Each filter file implements this
    }
    return g_currentFilter.get();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  -s, --seed SEED      Start from specific 8-character seed (A-Z, 1-9)\n";
    std::cout << "  -t, --threads NUM    Number of threads to use (default: auto-detect)\n";
    std::cout << "  -d, --debug          Enable debug mode (requires --seed)\n";
    std::cout << "  -l, --log-level LVL  Set log level (error,warn,info,debug)\n";
    std::cout << "  -v, --verbose        Shortcut for --log-level info\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << programName << " --seed AAAAAAAA --threads 8\n";
    std::cout << "  " << programName << " -s AAAAAAAA -d\n";
}

int applyCurrentFilter(const std::string& seed, std::ostream& debugOut) {
    return getCurrentFilter()->apply(seed, debugOut);
}

uint64_t seedToNumber(const std::string& seed) {
    const std::string chars = "ABCDEFGHIJKLMNPQRSTUVWXYZ123456789";
    uint64_t result = 0;
    uint64_t base = chars.length();
    
    for (char c : seed) {
        size_t pos = chars.find(c);
        if (pos == std::string::npos) return 0; // Invalid character
        result = result * base + pos;
    }
    return result;
}

std::string numberToSeed(uint64_t number) {
    const std::string chars = "ABCDEFGHIJKLMNPQRSTUVWXYZ123456789";
    uint64_t base = chars.length();
    
    if (number == 0) return "AAAAAAAA";
    
    std::string result;
    uint64_t temp = number;
    
    // Convert to base-33 representation
    while (temp > 0) {
        result = chars[temp % base] + result;
        temp /= base;
    }
    
    // Pad to 8 characters
    while (result.length() < 8) {
        result = chars[0] + result;
    }
    
    return result;
}

void displayStats(const SearchStats& stats, std::chrono::steady_clock::time_point startTime) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    double elapsedMin = elapsed.count() / 60.0;
    
    uint64_t total = stats.totalSeeds.load();
    uint64_t currentSeed = stats.currentSeedNumber.load();
    
    double rate = (elapsedMin > 0) ? total / elapsedMin : 0;
    
    // Total possible seeds in Balatro
    const uint64_t TOTAL_SEEDS = 2821109907456ULL;
    
    // Calculate progress percentage
    double progressPercent = (double)currentSeed / TOTAL_SEEDS * 100.0;
    
    // Calculate ETA
    uint64_t remainingSeeds = TOTAL_SEEDS - currentSeed;
    double etaMinutes = (rate > 0) ? remainingSeeds / rate : 0;
    uint64_t etaDays = (uint64_t)(etaMinutes / (60 * 24));
    uint64_t etaHours = (uint64_t)((etaMinutes - etaDays * 60 * 24) / 60);
    uint64_t etaMinutesRemainder = (uint64_t)(etaMinutes - etaDays * 60 * 24 - etaHours * 60);
    
    // Clear screen and move cursor to top
    std::cout << "\033[2J\033[H";
    
    std::cout << "=== SEED SEARCH STATISTICS ===" << std::endl;
    std::cout << "Filter:         " << getCurrentFilter()->getName() << std::endl;
    std::cout << "Runtime:        " << elapsed.count() << "s (" << std::fixed << std::setprecision(1) << elapsedMin << " min)" << std::endl;
    std::cout << "Current seed:   " << numberToSeed(currentSeed) << " (" << currentSeed << ")" << std::endl;
    std::cout << "Progress:       " << std::fixed << std::setprecision(6) << progressPercent << "%" << std::endl;
    std::cout << "Seeds explored: " << total << std::endl;
    std::cout << "Search rate:    " << std::fixed << std::setprecision(0) << rate << " seeds/min" << std::endl;
    
    if (rate > 0) {
        std::cout << "ETA:            " << etaDays << " days, " << etaHours << " hours, " << etaMinutesRemainder << " minutes" << std::endl;
    } else {
        std::cout << "ETA:            Calculating..." << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Matches found:" << std::endl;
    
    // Display configurable results
    auto resultNames = getCurrentFilter()->getResultNames();
    for (size_t i = 0; i < stats.results.size() && i < resultNames.size(); i++) {
        uint64_t count = stats.results[i].count.load();
        std::cout << "  " << std::left << std::setw(25) << (resultNames[i] + ":") << count << std::endl;
    }
    
    std::cout << std::endl;
    
    if (total > 0) {
        std::cout << "Match rates:" << std::endl;
        
        // Display configurable match rates
        for (size_t i = 0; i < stats.results.size() && i < resultNames.size(); i++) {
            uint64_t count = stats.results[i].count.load();
            if (count > 0) {
                std::cout << "  " << std::left << std::setw(25) << (resultNames[i] + ":") << "1 in " << (total / count) << std::endl;
            }
        }
    }
    
    std::cout << std::flush;
}

// Escape CSV field (very small helper)
static std::string csvEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\"\""; // double the quote
        else out += c;
    }
    return out;
}

void logMatch(const std::string& seed, int matchLevel, const std::string& matchName, std::ostream& csvFile, std::mutex& csvMutex) {
    std::lock_guard<std::mutex> lock(csvMutex);
    // CSV: seed,level,name (name quoted if necessary)
    if (matchName.empty()) {
        csvFile << seed << "," << matchLevel << std::endl;
    } else {
        csvFile << seed << "," << matchLevel << ",\"" << csvEscape(matchName) << "\"" << std::endl;
    }
    csvFile.flush();
}

void writeProgressFile(const std::string& filterKey, uint64_t currentNumber) {
    try {
        std::string progDir = "dist";
        std::string progFile = progDir + "/progress_" + filterKey + ".txt";
        std::string tmpFile = progFile + ".tmp";

        // Write to temporary file then atomically replace (best-effort)
        std::ofstream pf(tmpFile, std::ios::trunc);
        if (pf.is_open()) {
            pf << currentNumber << std::endl;
            pf.flush();
            pf.close();

            // Remove existing file then rename
            std::remove(progFile.c_str());
            std::rename(tmpFile.c_str(), progFile.c_str());
        }
    } catch (...) {
        // best-effort only
    }
}

void searchWorker(std::atomic<bool>& found, std::string& result, std::mutex& resultMutex, SearchStats& stats, uint64_t startSeed, int threadId, std::ostream& csvFile, std::mutex& csvMutex, std::ostream& debugOut) {
    const unsigned int numThreads = std::thread::hardware_concurrency();
    uint64_t currentNumber = startSeed + threadId;
    
    while (!found.load()) {
        std::string seed = numberToSeed(currentNumber);
        stats.currentSeedNumber.store(currentNumber);
        
        stats.totalSeeds++;
        int matchLevel = applyCurrentFilter(seed, debugOut);
        
        if (matchLevel > 0) {
            // Update configurable results
            stats.updateResult(matchLevel);
            std::string matchName = "";
            auto names = getCurrentFilter()->getResultNames();
            if (matchLevel > 0 && matchLevel <= static_cast<int>(names.size())) matchName = names[matchLevel - 1];
            logMatch(seed, matchLevel, matchName, csvFile, csvMutex);
        }
        
        // Each thread takes every Nth seed where N is number of threads
        currentNumber += numThreads;
    }
}

int main(int argc, char* argv[]) {
    std::atomic<bool> found(false);
    std::string result;
    std::mutex resultMutex;
    std::mutex csvMutex;
    SearchStats stats;
    
    // Parse command line arguments using getopt
    uint64_t startSeedNumber = 0;
    bool debugMode = false;
    std::string debugSeed;
    unsigned int numThreads = 0; // 0 means auto-detect
    bool resumeMode = false;
    uint64_t resumeOffset = 0;
    uint64_t resumeMargin = 0;
    std::string envFilePath;
    bool listResults = false;
    bool describeMatch = false;
    
    static struct option long_options[] = {
        {"seed", required_argument, 0, 's'},
    {"env", required_argument, 0, 'e'},
    {"list-results", no_argument, 0, 'L'},
    {"describe-match", no_argument, 0, 'D'},
    {"log-level", required_argument, 0, 'l'},
    {"verbose", no_argument, 0, 'v'},
        {"resume", no_argument, 0, 'r'},
        {"resume-offset", required_argument, 0, 'o'},
    {"resume-margin", required_argument, 0, 'm'},
        {"threads", required_argument, 0, 't'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "s:t:dh", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                if (strlen(optarg) == 8) {
                    debugSeed = optarg;
                    startSeedNumber = seedToNumber(debugSeed);
                } else {
                    log_error("Invalid seed format. Expected 8 characters (A-Z, 1-9).");
                    return 1;
                }
                break;
            case 'e':
                envFilePath = optarg;
                break;
            case 'L':
                listResults = true;
                break;
            case 'D':
                describeMatch = true;
                break;
            case 'r':
                resumeMode = true;
                break;
            case 'o':
                resumeOffset = std::stoull(optarg);
                break;
            case 'm':
                resumeMargin = std::stoull(optarg);
                break;
            case 't':
                numThreads = std::stoul(optarg);
                if (numThreads == 0) {
                    log_error("Number of threads must be greater than 0.");
                    return 1;
                }
                break;
            case 'l': {
                std::string lvl = optarg;
                for (auto &ch : lvl) ch = (char)std::tolower((unsigned char)ch);
                if (lvl == "error") set_log_level(LogLevel::ERROR);
                else if (lvl == "warn" || lvl == "warning") set_log_level(LogLevel::WARN);
                else if (lvl == "info") set_log_level(LogLevel::INFO);
                else if (lvl == "debug") set_log_level(LogLevel::DEBUG);
                else { log_warn("Unknown log level: ", optarg); }
            } break;
            case 'v':
                set_log_level(LogLevel::INFO);
                break;
            case 'd':
                debugMode = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case '?':
                // getopt_long already printed an error message
                printUsage(argv[0]);
                return 1;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    // Validate arguments
    if (debugMode && debugSeed.empty()) {
    log_error("Debug mode requires a seed. Use --seed option.");
        printUsage(argv[0]);
        return 1;
    }

    // If user provided a start seed on the command line, print it for interactive runs
    // but suppress this when we're running describe-match which expects only JSON.
    if (!describeMatch && !debugMode && !debugSeed.empty()) {
        std::cout << "Starting from seed: " << debugSeed << " (" << startSeedNumber << ")" << std::endl;
    }
    
    // Handle debug mode
    if (debugMode) {
        // enable gated debug prints
        set_debug(true);
        // Create debug output file
        std::string debugFilename = "debug_" + debugSeed + ".txt";
        std::ofstream debugFile(debugFilename);
        if (!debugFile.is_open()) {
            log_error("Could not create debug file: ", debugFilename);
            return 1;
        }
        
        std::cout << "Running debug mode for seed: " << debugSeed << std::endl;
        std::cout << "Debug output will be written to: " << debugFilename << std::endl;
        
        // Run filter on the single seed (filter will write debug info to debugFile)
        int matchLevel = applyCurrentFilter(debugSeed, debugFile);
        
        debugFile.close();
        
        // Print results to console
        std::cout << "Debug complete!" << std::endl;
        std::cout << "Seed: " << debugSeed << std::endl;
        std::cout << "Match Level: " << matchLevel << std::endl;
        // If matched, print the human-readable result name so GUI can display it
        if (matchLevel > 0) {
            auto names = getCurrentFilter()->getResultNames();
            if (matchLevel <= static_cast<int>(names.size())) {
                std::cout << "Match Name: " << names[matchLevel - 1] << std::endl;
            }
        }
        std::cout << "Debug output written to: " << debugFilename << std::endl;
        
        return 0;
    }

    // If requested, print filter result names as JSON and exit. This allows GUI or tools
    // to query the compiled filter for the meaning of match indices.
    if (listResults) {
        auto names = getCurrentFilter()->getResultNames();
        std::cout << "[";
        for (size_t i = 0; i < names.size(); ++i) {
            std::string s = names[i];
            std::string esc;
            for (char c : s) {
                if (c == '\\') esc += "\\\\";
                else if (c == '"') esc += "\\\"";
                else if (c == '\n') esc += "\\n";
                else if (c == '\r') esc += "\\r";
                else if (c == '\t') esc += "\\t";
                else esc += c;
            }
            std::cout << '"' << esc << '"';
            if (i + 1 < names.size()) std::cout << ',';
        }
        std::cout << "]\n";
        return 0;
    }

    // Describe-match: for a given --seed print a small JSON object with matched index and name
    if (describeMatch) {
        if (debugSeed.empty()) {
            log_error("--describe-match requires --seed <SEED>");
            return 1;
        }
        // If an env file was provided with --env, parse minimal fields now so describe-match respects them
        if (!envFilePath.empty()) {
            try {
                std::ifstream ef(envFilePath);
                if (ef.is_open()) {
                    std::stringstream ssin; ssin << ef.rdbuf();
                    std::string txt = ssin.str();
                    EnvConfig e;
                    // lightweight parse for unlockedJokers
                    auto posUJ = txt.find("\"unlockedJokers\"");
                    if (posUJ != std::string::npos) {
                        auto bracket = txt.find('[', posUJ);
                        if (bracket != std::string::npos) {
                            auto endb = txt.find(']', bracket);
                            if (endb != std::string::npos && endb > bracket) {
                                std::string body = txt.substr(bracket+1, endb - bracket - 1);
                                size_t p = 0;
                                while (p < body.size()) {
                                    auto q1 = body.find('"', p);
                                    if (q1 == std::string::npos) break;
                                    auto q2 = body.find('"', q1+1);
                                    if (q2 == std::string::npos) break;
                                    std::string jname = body.substr(q1+1, q2 - q1 - 1);
                                    if (!jname.empty()) e.unlockedJokers.push_back(jname);
                                    p = q2 + 1;
                                }
                            }
                        }
                    }
                    // also parse unlockedTags for parity
                    auto posUT = txt.find("\"unlockedTags\"");
                    if (posUT != std::string::npos) {
                        auto bracket = txt.find('[', posUT);
                        if (bracket != std::string::npos) {
                            auto endb = txt.find(']', bracket);
                            if (endb != std::string::npos && endb > bracket) {
                                std::string body = txt.substr(bracket+1, endb - bracket - 1);
                                size_t p = 0;
                                while (p < body.size()) {
                                    auto q1 = body.find('"', p);
                                    if (q1 == std::string::npos) break;
                                    auto q2 = body.find('"', q1+1);
                                    if (q2 == std::string::npos) break;
                                    std::string tname = body.substr(q1+1, q2 - q1 - 1);
                                    if (!tname.empty()) e.unlockedTags.push_back(tname);
                                    p = q2 + 1;
                                }
                            }
                        }
                    }
                    setGlobalEnv(e);
                }
            } catch(...) {}
        }
        int matchLevel = applyCurrentFilter(debugSeed, std::cout);
        int idx = matchLevel;
        std::string name = "";
        if (matchLevel > 0) {
            auto names = getCurrentFilter()->getResultNames();
            if (matchLevel <= static_cast<int>(names.size())) name = names[matchLevel - 1];
        }
        // Attempt to get a structured description from the filter
        try {
            auto filter = getCurrentFilter();
            if (filter) {
                std::string detail = filter->describeMatch(debugSeed);
                if (!detail.empty()) {
                    // If the filter returned a full JSON object, try to merge index/name if not present
                    // We trust filters to emit valid JSON. If index or name missing, still print detail.
                    std::cout << detail << std::endl;
                    return 0;
                }
            }
        } catch (...) {
            // ignore and fall back to simple output
        }

        // Fallback: Print simple JSON object with index/name (basic behavior)
        // Note: simple escaping for quotes/backslashes
        std::string esc;
        for (char c : name) {
            if (c == '\\') esc += "\\\\";
            else if (c == '"') esc += "\\\"";
            else if (c == '\n') esc += "\\n";
            else if (c == '\r') esc += "\\r";
            else if (c == '\t') esc += "\\t";
            else esc += c;
        }
        std::cout << "{\"index\": " << idx << ", \"name\": \"" << esc << "\"}\n";
        return 0;
    }
    
    // Create CSV file with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

    std::string csvFilename = "dist/matches_" + ss.str() + ".csv";
    
    #ifdef ENABLE_LOGS

    std::ofstream csvFile(csvFilename);
    if (!csvFile.is_open()) {
    log_error("Could not create CSV file: ", csvFilename);
        return 1;
    }

    #else

    std::streambuf * buf;
    buf = std::cout.rdbuf();
    std::ostream csvFile(buf);

    #endif

     
    // Write CSV header
    csvFile << "seed,match_level" << std::endl;
    std::cout << "Logging matches to: " << csvFilename << std::endl;
    
    stats.currentSeedNumber.store(startSeedNumber);

    // If env file provided, read it and apply global env
    if (!envFilePath.empty()) {
        // Try to use nlohmann/json if compiled with USE_NLOHMANN_JSON; otherwise fall back to the lightweight parser
#ifdef USE_NLOHMANN_JSON
        try {
            #include <nlohmann/json.hpp>
            std::ifstream ef(envFilePath);
            if (ef.is_open()) {
                try {
                    nlohmann::json j;
                    ef >> j;
                    EnvConfig e;
                    if (j.contains("deck") && j["deck"].is_string()) e.deck = j["deck"].get<std::string>();
                    if (j.contains("stake") && j["stake"].is_string()) e.stake = j["stake"].get<std::string>();
                    if (j.contains("tag") && j["tag"].is_string()) e.tag = j["tag"].get<std::string>();
                    if (j.contains("unlockedTags") && j["unlockedTags"].is_array()) {
                        for (const auto &vtag : j["unlockedTags"]) if (vtag.is_string()) e.unlockedTags.push_back(vtag.get<std::string>());
                    }
                    if (j.contains("unlockedJokers") && j["unlockedJokers"].is_array()) {
                        for (const auto &vj : j["unlockedJokers"]) if (vj.is_string()) e.unlockedJokers.push_back(vj.get<std::string>());
                    }
                    if (j.contains("showman") && j["showman"].is_boolean()) e.showman = j["showman"].get<bool>();
                    if (j.contains("freshProfile") && j["freshProfile"].is_boolean()) e.freshProfile = j["freshProfile"].get<bool>();
                    if (j.contains("freshRun") && j["freshRun"].is_boolean()) e.freshRun = j["freshRun"].get<bool>();
                    if (j.contains("sixesFactor") && j["sixesFactor"].is_number_integer()) e.sixesFactor = j["sixesFactor"].get<int>();
                    if (j.contains("version") && j["version"].is_number_integer()) e.version = j["version"].get<long>();
                    if (j.contains("forceAllContent") && j["forceAllContent"].is_boolean()) e.forceAllContent = j["forceAllContent"].get<bool>();
                    if (j.contains("selectedOptions")) {
                        auto so = j["selectedOptions"];
                        if (so.is_array()) {
                            bool allBool = true; bool allInt = true;
                            for (const auto &x : so) {
                                if (!x.is_boolean()) allBool = false;
                                if (!x.is_number_integer()) allInt = false;
                            }
                            if (allBool && so.size() == 61) {
                                e.selectedOptions.resize(61);
                                for (size_t i = 0; i < 61; ++i) e.selectedOptions[i] = so[i].get<bool>();
                                e.selectedOptionsSet = true;
                            } else if (allInt) {
                                e.selectedOptions = std::vector<bool>(61, false);
                                for (const auto &idx : so) {
                                    int v = idx.get<int>(); if (v >= 0 && v < 61) e.selectedOptions[v] = true;
                                }
                                e.selectedOptionsSet = true;
                            } else {
                                log_warn("selectedOptions not in expected format: array of 61 booleans or integer index list");
                            }
                        }
                    }
                    setGlobalEnv(e);
                    log_info("Applied env (json): deck=", e.deck, ", stake=", e.stake, ", tag=", e.tag, ", showman=", (e.showman?"true":"false"), ", unlockedTags=", e.unlockedTags.size(), ", unlockedJokers=", e.unlockedJokers.size());
                } catch (const std::exception &ex) {
                    log_error("Failed to parse env JSON: ", ex.what());
                }
            } else {
                log_warn("Could not open env file: ", envFilePath, ", falling back to lightweight parser");
            }
        } catch (...) {
            // fall back to naive parser below
        }
#endif

    try {
            std::ifstream ef(envFilePath);
            if (ef.is_open()) {
        log_warn("Using lightweight env parser (nlohmann::json not available or failed). Consider compiling with USE_NLOHMANN_JSON for robust parsing.");
                std::stringstream ssin;
                ssin << ef.rdbuf();
                std::string txt = ssin.str();
                EnvConfig e;
                // lightweight parsing: look for keys and extract simple values (robust to spacing)
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
                        // read until comma or brace
                        auto end = txt.find_first_of(",}\n\r", start);
                        if (end == std::string::npos) end = txt.size();
                        return txt.substr(start, end-start);
                    }
                };

                std::string v;
                v = find_str("deck"); if (!v.empty()) e.deck = v;
                v = find_str("stake"); if (!v.empty()) e.stake = v;
                v = find_str("tag"); if (!v.empty()) e.tag = v;
                // Lightweight parse for unlockedTags: look for "unlockedTags" and extract a simple array of strings
                auto posUT = txt.find("\"unlockedTags\"");
                if (posUT != std::string::npos) {
                    auto bracket = txt.find('[', posUT);
                    if (bracket != std::string::npos) {
                        auto endb = txt.find(']', bracket);
                        if (endb != std::string::npos && endb > bracket) {
                            std::string body = txt.substr(bracket+1, endb - bracket - 1);
                            size_t p = 0;
                            while (p < body.size()) {
                                // find next quote
                                auto q1 = body.find('"', p);
                                if (q1 == std::string::npos) break;
                                auto q2 = body.find('"', q1+1);
                                if (q2 == std::string::npos) break;
                                std::string tagname = body.substr(q1+1, q2 - q1 - 1);
                                if (!tagname.empty()) e.unlockedTags.push_back(tagname);
                                p = q2 + 1;
                            }
                        }
                    }
                }
                // Lightweight parse for unlockedJokers (fallback)
                auto posUJ = txt.find("\"unlockedJokers\"");
                if (posUJ != std::string::npos) {
                    auto bracket = txt.find('[', posUJ);
                    if (bracket != std::string::npos) {
                        auto endb = txt.find(']', bracket);
                        if (endb != std::string::npos && endb > bracket) {
                            std::string body = txt.substr(bracket+1, endb - bracket - 1);
                            size_t p = 0;
                            while (p < body.size()) {
                                auto q1 = body.find('"', p);
                                if (q1 == std::string::npos) break;
                                auto q2 = body.find('"', q1+1);
                                if (q2 == std::string::npos) break;
                                std::string jname = body.substr(q1+1, q2 - q1 - 1);
                                if (!jname.empty()) e.unlockedJokers.push_back(jname);
                                p = q2 + 1;
                            }
                        }
                    }
                }
                v = find_str("showman"); if (!v.empty()) e.showman = (v.find("true") != std::string::npos);
                v = find_str("sixesFactor"); if (!v.empty()) e.sixesFactor = std::stoi(v);
                v = find_str("version"); if (!v.empty()) e.version = std::stol(v);
                v = find_str("forceAllContent"); if (!v.empty()) e.forceAllContent = (v.find("true") != std::string::npos);
                v = find_str("freshProfile"); if (!v.empty()) e.freshProfile = (v.find("true") != std::string::npos);
                v = find_str("freshRun"); if (!v.empty()) e.freshRun = (v.find("true") != std::string::npos);

                // Lightweight attempt to parse selectedOptions: look for "selectedOptions" and extract a simple array
                auto posSo = txt.find("\"selectedOptions\"");
                if (posSo != std::string::npos) {
                    auto bracket = txt.find('[', posSo);
                    if (bracket != std::string::npos) {
                        auto endb = txt.find(']', bracket);
                        if (endb != std::string::npos && endb > bracket) {
                            std::string body = txt.substr(bracket+1, endb - bracket - 1);
                            // Split by commas and trim
                            std::vector<std::string> parts;
                            size_t p = 0;
                            while (p < body.size()) {
                                auto comma = body.find(',', p);
                                if (comma == std::string::npos) comma = body.size();
                                std::string token = body.substr(p, comma - p);
                                // trim
                                auto l = token.find_first_not_of(" \t\n\r");
                                auto r = token.find_last_not_of(" \t\n\r");
                                if (l != std::string::npos && r != std::string::npos) token = token.substr(l, r - l + 1);
                                else token = "";
                                if (!token.empty()) parts.push_back(token);
                                p = comma + 1;
                            }
                            // Determine if parts are booleans or indices
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

                setGlobalEnv(e);
                std::cout << "Applied env: deck=" << e.deck << ", stake=" << e.stake << ", tag=" << e.tag << ", showman=" << (e.showman?"true":"false")
                          << ", unlockedTags=" << e.unlockedTags.size() << ", unlockedJokers=" << e.unlockedJokers.size() << std::endl;
            }
        } catch (...) {
            // ignore parsing errors
        }
    }

    // If resume mode requested, try to read existing progress file for this filter
    // Determine filter key (use simplified filter name from getCurrentFilter()->getName())
    std::string filterKeyRaw = getCurrentFilter()->getName();
    // Sanitize filter key to file-friendly short name (replace spaces with underscore)
    std::string filterKey;
    for (char ch : filterKeyRaw) {
        if (std::isalnum((unsigned char)ch) || ch == '_' ) filterKey.push_back(ch);
        else if (std::isspace((unsigned char)ch)) filterKey.push_back('_');
    }
    if (filterKey.empty()) filterKey = "filter";

    if (resumeMode) {
        try {
            std::string progFile = std::string("dist/progress_") + filterKey + ".txt";
            std::ifstream pf(progFile);
            if (pf.is_open()) {
                uint64_t stored = 0;
                pf >> stored;
                if (stored > 0) {
                    std::cout << "Resuming from stored seed number: " << stored << " -> " << numberToSeed(stored) << std::endl;
                    // Apply margin (subtract) then offset (add)
                    uint64_t applied = stored;
                    if (resumeMargin > 0) {
                        if (applied > resumeMargin) applied = applied - resumeMargin;
                        else applied = 0;
                    }
                    if (resumeOffset > 0) applied = applied + resumeOffset;
                    std::cout << "Applied resume margin: " << resumeMargin << ", offset: " << resumeOffset << " -> starting at: " << applied << " (" << numberToSeed(applied) << ")" << std::endl;
                    stats.currentSeedNumber.store(applied);
                    startSeedNumber = applied; // threads will start from this base
                }
                pf.close();
            }
        } catch (...) {
            // ignore
        }
    }
    
    // Initialize configurable results with default filter
    SearchFilter* currentFilter = getCurrentFilter();
    stats.initializeResults(currentFilter->getResultNames());
    
    // Create null stream for filter debug output (since debug mode is disabled in normal search)
    // cross-platform null stream
    std::ofstream nullOfs;
    #ifdef _WIN32
    nullOfs.open("NUL");
    #else
    nullOfs.open("/dev/null");
    #endif
    std::ostream nullStream((nullOfs.is_open() ? nullOfs.rdbuf() : std::cout.rdbuf()));

    // Use specified thread count or auto-detect
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // Fallback if auto-detection fails
    }
    // Progress write throttle (ms)
    const uint64_t PROGRESS_THROTTLE_MS = 5000;
    
    auto startTime = std::chrono::steady_clock::now();
    
    std::cout << "Starting search with " << numThreads << " threads..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; i++) {
        threads.emplace_back(searchWorker, std::ref(found), std::ref(result), std::ref(resultMutex), std::ref(stats), startSeedNumber, i, std::ref(csvFile), std::ref(csvMutex), std::ref(nullStream));
    }
    
    // Stats display thread
    std::thread statsThread([&]() {
        auto lastWrite = std::chrono::steady_clock::now() - std::chrono::milliseconds(PROGRESS_THROTTLE_MS);
        while (!found.load()) {
            displayStats(stats, startTime);
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWrite).count() >= (long)PROGRESS_THROTTLE_MS) {
                writeProgressFile(filterKey, stats.currentSeedNumber.load());
                lastWrite = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        // Final write
        writeProgressFile(filterKey, stats.currentSeedNumber.load());
    });
    
    // Handle Ctrl+C gracefully
    std::signal(SIGINT, [](int) {
        std::cout << "\n\nInterrupted by user." << std::endl;
        std::exit(1);
    });
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    statsThread.join();

    #ifdef ENABLE_LOGS

    csvFile.close();

    #endif

    
    // Final stats display
    displayStats(stats, startTime);
    std::cout << "\n*** SEARCH COMPLETE ***" << std::endl;
    std::cout << "Found seed: " << result << std::endl;
    std::cout << "Last processed seed: " << numberToSeed(stats.currentSeedNumber.load()) << std::endl;
    std::cout << "Matches logged to: " << csvFilename << std::endl;
    
    return 0;
}
