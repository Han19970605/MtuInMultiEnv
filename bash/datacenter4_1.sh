#!/bin/bash

for delay in '20us'
do 
    # corresponding loss_rate = 0 0.1% 1% 5%
    for loss_rate in 0 0.0005
    do
        for bandwidth in '1Gbps' '10Gbps' '40Gbps' 
        do
            for load in 0.3 0.5 0.8
            do
                ./waf --run "mtu_datacenter --DELAY=$delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$bandwidth --LOAD=$load"
            done
        done
    done
done