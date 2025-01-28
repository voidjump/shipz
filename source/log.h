#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include <termcolor/termcolor.hpp>

namespace log {
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
    }
    os << "]";
    return os;
}

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
