#!/bin/bash

if [ "$1" == memory ]; then
    for i in `seq 0 10`;
    do
        ./memory/memory_access 10485760 100 > mem_testing
        python ks.py mem_base mem_testing
        sleep 3
    done
fi

if [ "$1" == disk ]; then
    for i in `seq 0 10`;
    do
        ./disk/disk_stream 1048576 100 testing > disk_testing
        python ks.py disk_base disk_testing
        sleep 3
    done
fi

if [ "$1" == network ]; then
    for i in `seq 0 10`;
    do
        ./network/network_connect 100 10 192.168.1.12/ > network_testing
        python ks.py network_base network_testing
        sleep 3
    done
fi
