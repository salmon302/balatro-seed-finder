#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include <memory>
#include "../instance.hpp"
#include "../util.hpp"
#include "../items.hpp"
#include "../pack.hpp"

struct FilterResult {
    std::string name;
    std::atomic<uint64_t> count{0};

    FilterResult() = default;
    FilterResult(const std::string& n) : name(n) {}
    FilterResult(const FilterResult& other) : name(other.name), count(other.count.load()) {}
    FilterResult& operator=(const FilterResult& other) {
        if (this != &other) {
            name = other.name;
            count.store(other.count.load());
        }
        return *this;
    }
};

// Abstract base class for search filters
class SearchFilter {
public:
    virtual ~SearchFilter() = default;
    virtual int apply(const std::string& seed, std::ostream& debugOut = std::cout) = 0;
    virtual std::vector<std::string> getResultNames() const = 0;
    virtual std::string getName() const = 0;
};

// Generic function pointer filter for custom filters
class CustomFilter : public SearchFilter {
private:
    std::function<int(const std::string&, std::ostream&)> filterFunc;
    std::vector<std::string> resultNames;
    std::string filterName;

public:
    CustomFilter(std::function<int(const std::string&, std::ostream&)> func,
                 const std::vector<std::string>& names,
                 const std::string& name = "Custom Filter")
        : filterFunc(func), resultNames(names), filterName(name) {}

    int apply(const std::string& seed, std::ostream& debugOut = std::cout) override {
        return filterFunc(seed, debugOut);
    }

    std::vector<std::string> getResultNames() const override {
        return resultNames;
    }

    std::string getName() const override {
        return filterName;
    }
};

// Utility function to create a custom filter with lambda
std::unique_ptr<SearchFilter> createCustomFilter(
    std::function<int(const std::string&, std::ostream&)> filterFunc,
    const std::vector<std::string>& resultNames,
    const std::string& name = "Custom Filter") {
    return std::make_unique<CustomFilter>(filterFunc, resultNames, name);
}

// Function that each filter file must implement
std::unique_ptr<SearchFilter> createFilter();