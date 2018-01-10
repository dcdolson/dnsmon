#pragma once
#include <stdint.h>

class DnsOptions
{
public:
    DnsOptions(int argc, char* const* argv);

    uint16_t Port() const { return m_port; }

private:
    uint16_t  m_port;
};
