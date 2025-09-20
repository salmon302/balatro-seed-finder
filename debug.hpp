// Simple header-only gated debug logging utility
#pragma once

#include "logger.hpp"

inline void set_debug(bool v) {
    if (v) set_log_level(LogLevel::DEBUG);
    else set_log_level(LogLevel::INFO);
}

template<typename... Args>
inline void debug_print(const Args&... args) {
    log_debug(args...);
}

template<typename... Args>
inline void debug_println(const Args&... args) {
    log_debug(args...);
}
