#pragma once
#include <stdint.h>
#include <string>

class DnsOptions
{
public:
    DnsOptions(int argc, char* const* argv);

    //! Get the listen port when acting as server.
    uint16_t Port() const          { return m_port; }

    //! Get the textual IP address to connect to.
    const char* ServerAddr() const { return m_serverAddr.c_str(); }

private:
    uint16_t     m_port;
    std::string  m_serverAddr;
};
