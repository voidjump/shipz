#pragma once
#include <iostream>

namespace log
{
    template <typename... Args>
    inline void info(Args &&...args)
    {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        std::cout << "[I] " << oss.str() << std::endl;
    }

    template <typename... Args>
    inline void error(Args &&...args)
    {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        std::cout << "[E] " << oss.str() << std::endl;
    }

#ifdef DEBUG_BUILD
    template <typename... Args>
    inline void debug(Args &&...args)
    {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        std::cout << "[D] " << oss.str() << std::endl;
    }
#else
    template <typename... Args>
    inline void debug(Args &&...args) {}
#endif
}
