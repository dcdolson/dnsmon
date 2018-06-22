# dnsmon
Experimental DNS proxy

The idea is to receive DNS traffic as a server and proxy it to another server,
with some logging.

This is not yet suitable for use, since many aspects of the DNS protocol are
unsupported, and the service might not restart if it fails.

The scripts are for running as a service in a FreeNAS jail.

You can get traffic to this proxy by getting your home router to provide
this proxy as the DNS server in DHCP messages to clients. In this way
you can get logs of what devices on your network are doing.
