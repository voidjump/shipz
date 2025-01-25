#pragma once
#include <iostream>
#include <sstream>
#include <termcolor/termcolor.hpp>

namespace log {
template <typename... Args>
inline void info(Args &&...args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    std::cout << termcolor::white << "[I] " << oss.str() << termcolor::reset
              << std::endl;
}

template <typename... Args>
inline void error(Args &&...args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    std::cout << termcolor::red << "[E] " << oss.str() << termcolor::reset
              << std::endl;
}

#ifdef DEBUG_BUILD
template <typename... Args>
inline void debug(Args &&...args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    std::cout << termcolor::white << "[D] " << oss.str() << termcolor::reset
              << std::endl;
}
#else
template <typename... Args>
inline void debug(Args &&...args) {}
#endif
}  // namespace log
