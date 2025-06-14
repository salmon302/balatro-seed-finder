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
#include "immolate.hpp"

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

int applyCurrentFilter(const std::string& seed) {
    return getCurrentFilter()->apply(seed);
}

int hasInstPerkeoAndInstTribAndInstYorick(const std::string& seed) {
    std::vector<bool> selectedOptions(61, true);
    Instance inst = initInstance(seed, selectedOptions);
    bool cont = false;
    bool cont2 = false;

    if (inst.nextTag(1) == "Charm Tag") {
        std::vector<std::string> cards = inst.nextArcanaPack(5, 1);
        for (int c = 0; c < 5; c++) {
            if(cards[c] == "The Soul") {
                if(inst.nextJoker("sou", 1, false).joker == "Perkeo") {
                    cont = true;
                }
            }
        }
    }
    if (cont && inst.nextTag(1) == "Charm Tag") {
        std::vector<std::string> cards = inst.nextArcanaPack(5, 1);
        for (int c = 0; c < 5; c++) {
            if(cards[c] == "The Soul") {
                if(inst.nextJoker("sou", 1, false).joker == "Triboulet") {
                    cont2 = true;
                }
            }
        }
    }

    if (cont2 && inst.nextTag(2) == "Charm Tag") {
        std::vector<std::string> cards = inst.nextArcanaPack(5, 2);
        for (int c = 0; c < 5; c++) {
            if(cards[c] == "The Soul") {
                if(inst.nextJoker("sou", 1, false).joker == "Yorick") {
                    return 3; // Full match (Perkeo + Triboulet + Yorick)
                }
            }
        }
    }
    
    if (cont2) return 2; // Perkeo + Triboulet
    if (cont) return 1;  // Perkeo only
    return 0; // No match
}

std::string runSimulation(const std::string& seed) {
    std::vector<bool> selectedOptions(61, true);
    Instance inst = initInstance(seed, selectedOptions);
    
    std::string output = "";
    int ante = 1; // Maximum ante to simulate
    
    // Cards per ante array (based on typical Balatro progression)
    std::vector<int> cardsPerAnte = {15,50,50,50,50,50,50,50};
    
    for (int a = 1; a <= ante; a++) {
        inst.initUnlocks(a, false);
        output += "==ANTE " + std::to_string(a) + "==\n";
        output += "Boss: " + inst.nextBoss(a) + "\n";
        
        std::string voucher = inst.nextVoucher(a);
        output += "Voucher: " + voucher + "\n";
        inst.lock(voucher);
        
        // Unlock next level voucher
        for (int i = 0; i < VOUCHERS.size(); i += 2) {
            if (VOUCHERS[i] == voucher) {
                // Only unlock it if it's unlockable
                auto it = std::find(VOUCHERS.begin(), VOUCHERS.end(), VOUCHERS[i+1]);
                if (it != VOUCHERS.end()) {
                    int optionIndex = std::distance(VOUCHERS.begin(), it);
                    if (optionIndex < selectedOptions.size() && selectedOptions[optionIndex]) {
                        inst.unlock(VOUCHERS[i+1]);
                    }
                }
            }
        }
        
        output += "Tags: " + inst.nextTag(a) + ", " + inst.nextTag(a) + "\n";
        
        output += "Shop Queue: \n";
        int cardsThisAnte = (a <= cardsPerAnte.size()) ? cardsPerAnte[a-1] : 6;
        for (int q = 1; q <= cardsThisAnte; q++) {
            output += std::to_string(q) + ") ";
            ShopItem item = inst.nextShopItem(a);
            if (item.type == "Joker") {
                if (item.jokerData.stickers.eternal) output += "Eternal ";
                if (item.jokerData.stickers.perishable) output += "Perishable ";
                if (item.jokerData.stickers.rental) output += "Rental ";
                if (item.jokerData.edition != "No Edition") output += item.jokerData.edition + " ";
            }
            output += item.item + "\n";
        }
        
        output += "\nPacks: \n";
        int numPacks = (a == 1) ? 4 : 6;
        for (int p = 1; p <= numPacks; p++) {
            std::string pack = inst.nextPack(a);
            output += pack + " - ";
            Pack packData = packInfo(pack);
            
            if (packData.type == "Celestial Pack") {
                std::vector<std::string> cards = inst.nextCelestialPack(packData.size, a);
                for (int c = 0; c < packData.size; c++) {
                    output += cards[c];
                    output += (c + 1 != packData.size) ? ", " : "";
                }
            }
            if (packData.type == "Arcana Pack") {
                std::vector<std::string> cards = inst.nextArcanaPack(packData.size, a);
                for (int c = 0; c < packData.size; c++) {
                    output += cards[c];
                    output += (c + 1 != packData.size) ? ", " : "";
                }
            }
            if (packData.type == "Spectral Pack") {
                std::vector<std::string> cards = inst.nextSpectralPack(packData.size, a);
                for (int c = 0; c < packData.size; c++) {
                    output += cards[c];
                    output += (c + 1 != packData.size) ? ", " : "";
                }
            }
            if (packData.type == "Buffoon Pack") {
                std::vector<JokerData> cards = inst.nextBuffoonPack(packData.size, a);
                for (int c = 0; c < packData.size; c++) {
                    JokerData joker = cards[c];
                    if (joker.stickers.eternal) output += "Eternal ";
                    if (joker.stickers.perishable) output += "Perishable ";
                    if (joker.stickers.rental) output += "Rental ";
                    if (joker.edition != "No Edition") output += joker.edition + " ";
                    output += joker.joker;
                    output += (c + 1 != packData.size) ? ", " : "";
                }
            }
            if (packData.type == "Standard Pack") {
                std::vector<Card> cards = inst.nextStandardPack(packData.size, a);
                for (int c = 0; c < packData.size; c++) {
                    Card card = cards[c];
                    if (card.seal != "No Seal") output += card.seal + " ";
                    if (card.edition != "No Edition") output += card.edition + " ";
                    if (card.enhancement != "No Enhancement") output += card.enhancement + " ";
                    
                    char rank = card.base[2];
                    if (rank == 'T') output += "10";
                    else if (rank == 'J') output += "Jack";
                    else if (rank == 'Q') output += "Queen";
                    else if (rank == 'K') output += "King";
                    else if (rank == 'A') output += "Ace";
                    else output += rank;
                    
                    output += " of ";
                    
                    char suit = card.base[0];
                    if (suit == 'C') output += "Clubs";
                    else if (suit == 'S') output += "Spades";
                    else if (suit == 'D') output += "Diamonds";
                    else if (suit == 'H') output += "Hearts";
                    
                    output += (c + 1 != packData.size) ? ", " : "";
                }
            }
            output += "\n";
        }
        
        output += "\n";
    }
    
    return output;
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

std::string generateRandomSeed() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Characters: A-Z, 1-9 (excluding 0)
    const std::string chars = "ABCDEFGHIJKLMNPQRSTUVWXYZ123456789";
    std::uniform_int_distribution<> dis(0, chars.length() - 1);
    
    std::string seed;
    for (int i = 0; i < 8; i++) {
        seed += chars[dis(gen)];
    }
    return seed;
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

void logMatch(const std::string& seed, int matchLevel, std::ostream& csvFile, std::mutex& csvMutex) {
    std::lock_guard<std::mutex> lock(csvMutex);
    csvFile << seed << "," << matchLevel << std::endl;
    csvFile.flush();
}

void searchWorker(std::atomic<bool>& found, std::string& result, std::mutex& resultMutex, SearchStats& stats, uint64_t startSeed, int threadId, std::ostream& csvFile, std::mutex& csvMutex) {
    const unsigned int numThreads = std::thread::hardware_concurrency();
    uint64_t currentNumber = startSeed + threadId;
    
    while (!found.load()) {
        std::string seed = numberToSeed(currentNumber);
        stats.currentSeedNumber.store(currentNumber);
        
        stats.totalSeeds++;
        int matchLevel = applyCurrentFilter(seed);
        
        if (matchLevel > 0) {
            // Update configurable results
            stats.updateResult(matchLevel);
            logMatch(seed, matchLevel, csvFile, csvMutex);
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
    
    // Parse command line arguments
    uint64_t startSeedNumber = 0;
    if (argc > 1) {
        std::string startSeedStr = argv[1];
        if (startSeedStr.length() == 8) {
            startSeedNumber = seedToNumber(startSeedStr);
            std::cout << "Starting from seed: " << startSeedStr << " (" << startSeedNumber << ")" << std::endl;
        } else {
            std::cout << "Invalid seed format. Expected 8 characters (A-Z, 1-9). Starting from AAAAAAAA." << std::endl;
        }
    }
    
    // Create CSV file with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

    std::string csvFilename = "matches_" + ss.str() + ".csv";
    
    #ifdef ENABLE_LOGS

    std::ofstream csvFile(csvFilename);
    if (!csvFile.is_open()) {
        std::cerr << "Error: Could not create CSV file: " << csvFilename << std::endl;
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
    
    // Initialize configurable results with default filter
    SearchFilter* currentFilter = getCurrentFilter();
    stats.initializeResults(currentFilter->getResultNames());
    
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    
    auto startTime = std::chrono::steady_clock::now();
    
    std::cout << "Starting search with " << numThreads << " threads..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; i++) {
        threads.emplace_back(searchWorker, std::ref(found), std::ref(result), std::ref(resultMutex), std::ref(stats), startSeedNumber, i, std::ref(csvFile), std::ref(csvMutex));
    }
    
    // Stats display thread
    std::thread statsThread([&]() {
        while (!found.load()) {
            displayStats(stats, startTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
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
