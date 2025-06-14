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
