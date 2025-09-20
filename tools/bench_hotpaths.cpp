#include <iostream>
#include <chrono>
#include "instance.hpp"

using namespace std::chrono;

int main() {
    // Construct an instance with a sample seed
    Instance::Instance inst("ABCDEFGH");
    inst.initLocks(1, false, false);

    const int ITER = 100000;

    // Warm up
    for (int i = 0; i < 1000; ++i) {
        inst.nextTarot_enum("ar1", 1, true);
        inst.nextJoker_enum("sou", 1, false);
        inst.nextShopItem_enum(1);
    }

    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i) {
        inst.nextTarot_enum("ar1", 1, true);
    }
    auto t1 = high_resolution_clock::now();

    auto dt_tarot = duration_cast<milliseconds>(t1 - t0).count();

    t0 = high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i) {
        inst.nextJoker_enum("sou", 1, false);
    }
    t1 = high_resolution_clock::now();
    auto dt_joker = duration_cast<milliseconds>(t1 - t0).count();

    t0 = high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i) {
        inst.nextShopItem_enum(1);
    }
    t1 = high_resolution_clock::now();
    auto dt_shop = duration_cast<milliseconds>(t1 - t0).count();

    std::cout << "Benchmark iterations: " << ITER << "\n";
    std::cout << "Tarot time ms: " << dt_tarot << "\n";
    std::cout << "Joker time ms: " << dt_joker << "\n";
    std::cout << "ShopItem time ms: " << dt_shop << "\n";

    return 0;
}
