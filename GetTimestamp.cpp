#include "GetTimestamp.h"
#include <chrono>
#include <sstream>

std::string GetTimestamp()
{
    using namespace std::chrono;
    auto now = system_clock::to_time_t(system_clock::now());
    std::stringstream s;
    s << now;
    return s.str();
}

