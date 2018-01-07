#pragma once
#include <iostream>
#include <iomanip>

inline void PacketDump(std::ostream& o, const uint8_t* buf, unsigned length)
{
    o << std::hex;
    const uint8_t* end = buf + length;
    unsigned col = 0;
    for( ; buf != end; ++buf)
    {
        o << std::setw(2) << std::setfill('0') << unsigned(*buf) << " ";
        if (++col == 16)
        {
            o << std::endl;
            col = 0;
        }
    }
    if (col != 0)
    {
        o << std::endl;
    }
    o << std::dec;
}

