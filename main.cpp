#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "PacketDump.h"
#include "GetTimestamp.h"
#include "Options.h"

bool dns_isquery(const uint8_t* dns, unsigned length)
{
    return length >= 4 && ((dns[2] & 0x80) == 0);
}

inline const char* uc_to_c(const uint8_t* uc)
{
    return reinterpret_cast<const char*>(uc);
}

unsigned dns_numQuestions(const uint8_t* dns, unsigned length)
{
    if(length >= 12)
    {
        return ntohs(*reinterpret_cast<const uint16_t*>(dns+4));
    }
    return 0;
}

class DnsQuestion
{
public:
    DnsQuestion(const std::string& name):
        m_name(name)
    {
    }
    std::string Name() const
    {
        return m_name;
    }
private:
    std::string m_name;
};

using DnsQuestions = std::vector<DnsQuestion>;

std::string next_label(const uint8_t*& p, unsigned& length)
{
    if(length == 0)
    {
        return std::string();
    }

    uint8_t len = *p++;
    --length;
    if(len > length)
    {
        // invalid packet
        syslog(LOG_LOCAL0|LOG_WARNING, "Invalid question format");
        return std::string();
    }
    std::string label(uc_to_c(p), len);
    p += len;
    length -= len;
    return label;
}

DnsQuestions ParseQuestions(const uint8_t* dns, unsigned length)
{
    DnsQuestions result;
    if(dns_isquery(dns, length))
    {
        unsigned nq = dns_numQuestions(dns, length);
        dns += 12;
        length -= 12;
        for(unsigned i=0; i < nq; ++i)
        {
            std::string name;
            while(true)
            {
                std::string label = next_label(dns, length); // updates dns, length to new values.
                if(label.empty())
                    break;
                name += label;
                name += ".";
            }
            DnsQuestion q(name);
            result.push_back(std::move(q));
        }
    } else printf("Not query\n");
    return result;
}

std::string to_string(const struct in_addr& a)
{
    char dst[INET_ADDRSTRLEN];
    const char *p = inet_ntop(AF_INET, &a, dst, sizeof(dst));
    if (!p)
    {
        throw std::invalid_argument("could not convert address");
    }
    return std::string(p);
}

void Dump(const struct sockaddr_in& from, const uint8_t* message, unsigned length)
{
    DnsQuestions info = ParseQuestions(message, length);
    for(const DnsQuestion& q: info)
    {
        syslog(LOG_LOCAL0|LOG_INFO, "dns-query %s %s", to_string(from.sin_addr).c_str(), q.Name().c_str());
    }
}

int main(int argc, char* const* argv)
{
    DnsOptions options(argc, argv);

    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        syslog(LOG_LOCAL0|LOG_ERR, "Failure to open server socket: %m");
        exit(1);
    }
    
    // Listen port from internal clients.
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(options.Port()); // DNS
    sa.sin_addr.s_addr = INADDR_ANY;
    if (0 != bind(s, (struct sockaddr*)&sa, sizeof(sa)))
    {
        syslog(LOG_LOCAL0|LOG_ERR, "Failure to bind socket: %m");
        exit(1);
    }

    int client_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        syslog(LOG_LOCAL0|LOG_ERR, "Failure to open client socket: %m");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(53);
    if (1 != inet_pton(AF_INET, options.ServerAddr(), &server.sin_addr))
    {
        syslog(LOG_LOCAL0|LOG_ERR, "Cannot parse server address '%s' as IPv4", options.ServerAddr());
        std::cerr << "Problem parsing server address '" << options.ServerAddr() << "' as IPv4" << std::endl;
        exit(1);
    }
    if (0 != connect(client_socket, (struct sockaddr*)&server, sizeof(server)))
    {
        syslog(LOG_LOCAL0|LOG_ERR, "Failure connecting to server: %m");
        exit(1);
    }

    if(options.IsDaemon())
    {
        daemon(0, 0);
    }

    uint16_t g_txn = 1;
    using tmap = std::unordered_map<uint16_t, std::pair<struct sockaddr_in, uint16_t> >;
    tmap transaction_map;

    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);
        FD_SET(client_socket, &readfds);

        int n = select(std::max(s,client_socket)+1, &readfds, /*writefds*/ nullptr, /*exceptfds*/nullptr, /*timeout*/nullptr);
        if(n < 0)
        {
            syslog(LOG_LOCAL0|LOG_ERR, "Select() error: %m");
            exit(1);
        }

        uint8_t message[8192];

        if (FD_ISSET(s, &readfds))
        {
            // from internal host
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            memset(&from, 0, fromlen);
            ssize_t bytes = recvfrom(s, message, sizeof(message), /*flags*/ 0, (struct sockaddr*)&from, &fromlen);
            if(bytes > 0)
            {
                // PacketDump(std::cout, message, bytes);
                if (fromlen == sizeof(from))
                {
                    uint16_t txn = ntohs(*(uint16_t*)message);
                    // std::cout << "Received " << bytes << " bytes from " << to_string(from.sin_addr)
                    //           << " : " << ntohs(from.sin_port) << " txn: " << txn << std::endl;
                    Dump(from, message, bytes);

                    // new transaction that makes sense to us, as a key for response.
                    uint16_t new_txn = ++g_txn;
                    transaction_map[new_txn] = std::make_pair(from, txn);
                    *(uint16_t*)message = htons(new_txn);
                    if(bytes != send(client_socket, message, bytes, /*flags*/ 0))
                    {
                        syslog(LOG_LOCAL0|LOG_ERR, "Trouble sending to server: %m");
                    }
                }
            }
            else
            {
                syslog(LOG_LOCAL0|LOG_ERR, "Error on server socket recvfrom: %m");
                exit(1);
            }
        }

        if(FD_ISSET(client_socket, &readfds))
        {
            // response from server
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            memset(&from, 0, fromlen);
            ssize_t bytes = recvfrom(client_socket, message, sizeof(message), /*flags*/ 0, (struct sockaddr*)&from, &fromlen);
            if(bytes > 2 && fromlen == sizeof(from))
            {
                // PacketDump(std::cout, message, bytes);
                uint16_t txn = ntohs(*(uint16_t*)message);
                // std::cout << "Received " << bytes << " bytes from " << to_string(from.sin_addr)
                //           << " : " << ntohs(from.sin_port) << " txn: " << txn << std::endl;
                tmap::iterator i = transaction_map.find(txn);
                if(i != transaction_map.end())
                {
                    *(uint16_t*)message = htons(i->second.second);
                    ssize_t sent = sendto(s, message, bytes, /*flags*/ 0,
                                          (struct sockaddr*)&i->second.first, sizeof(struct sockaddr_in));
                    if(sent < 0)
                    {
                        syslog(LOG_LOCAL0|LOG_ERR, "Error sending back to client: %m");
                    }
                    transaction_map.erase(i);
                }
                else
                {
                    syslog(LOG_LOCAL0|LOG_WARNING, "Response transaction not found");
                }
            }
            else
            {
                syslog(LOG_LOCAL0|LOG_ERR, "Error on client socket recvfrom: %m");
                exit(1);
            }
        }
    }

}

