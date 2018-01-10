#include "Options.h"
#include <stdlib.h>
#include <iostream>
#include <getopt.h>

namespace
{
    void Usage(const char* progname)
    {
        std::cerr << "Usage:" << std::endl
                  << "    " << progname << " [-p <port>] [--port <port>]" << std::endl;
    }

    uint16_t ParsePort(const char* opt)
    {
       char* endptr;
       unsigned long int i = strtoul(optarg, &endptr, /*base*/ 10);
       if(*endptr != '\0')
       {
           std::cerr << "Non-integer passed to port option." << std::endl;
           exit(1);
       }
       if(i > 0xffff)
       {
           std::cerr << "Port number exceeds maximum 16-bit integer value of 65535." << std::endl;
           exit(1);
       }
       return i;
    }
}


DnsOptions::DnsOptions(int argc, char* const* argv):
    m_port(53)
{
    bool found_server = false;
    while (1)
    {
        static struct option long_options[] = {
            {"port",    required_argument, nullptr,  'p' },
            {"server",  required_argument, nullptr,  's' },
            {0,         0,                 nullptr,  0 }
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "p:s:h", long_options, &option_index);
        if (c == -1)
        {
            if(optind < argc)
            {
                std::cerr << "Unexpected parameter '" << argv[optind] << "' after flags." << std::endl;
                Usage(argv[0]);
                exit(1);
            }
            break;
        }

        switch (c)
        {
        case 'p':
            m_port = ParsePort(optarg);
            break;

        case 's':
            m_serverAddr = optarg;
            found_server = true;
            break;
        case 'h':
            Usage(argv[0]);
            exit(0);

        case '?':
            Usage(argv[0]);
            exit(1);
        }
    }
    if(!found_server)
    {
        std::cerr << "Required argument --server is missing" << std::endl;
        Usage(argv[0]);
        exit(1);
    }
}

