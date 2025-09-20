// Simple header-only logging with levels
#pragma once

#include <atomic>
#include <string>
#include <sstream>
#include <iostream>

enum class LogLevel { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 };

inline std::atomic<LogLevel>& get_log_level_ref() {
    static std::atomic<LogLevel> lvl{LogLevel::INFO};
    return lvl;
}

inline void set_log_level(LogLevel l) { get_log_level_ref().store(l); }

inline LogLevel get_log_level() { return get_log_level_ref().load(); }

template<typename... Args>
inline void log_with_stream(std::ostream& os, const Args&... args) {
    try {
        std::ostringstream ss;
        using expander = int[];
        (void)expander{0, (void(ss << args), 0)... };
        os << ss.str();
    } catch(...) {}
}

template<typename... Args>
inline void log_error(const Args&... args) {
    log_with_stream(std::cerr, "[ERROR] ", args..., "\n");
}

template<typename... Args>
inline void log_warn(const Args&... args) {
    if (get_log_level() >= LogLevel::WARN) log_with_stream(std::cerr, "[WARN] ", args..., "\n");
}

template<typename... Args>
inline void log_info(const Args&... args) {
    if (get_log_level() >= LogLevel::INFO) log_with_stream(std::cerr, "[INFO] ", args..., "\n");
}

template<typename... Args>
inline void log_debug(const Args&... args) {
    if (get_log_level() >= LogLevel::DEBUG) log_with_stream(std::cerr, "[DEBUG] ", args..., "\n");
}
