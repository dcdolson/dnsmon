#!/bin/sh

while [ 1 ]
do
    logger local.notice "Starting dnsmon"
    /usr/local/dnsmon/bin/dnsmon >> /var/log/dnsmon.log 2>> /var/log/dnsmon.err
    logger local.notice "dnsmon exited with code $?"
done

