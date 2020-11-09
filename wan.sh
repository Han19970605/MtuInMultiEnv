#!/bin/bash

for delay in '1ms' '2.5ms' '6.25ms' '12.5ms' 
do 
    for loss_rate in 0 0.0005 0.005 0.025
    do
        for bandwidth in '10Mbps' '100Mbps' '1000Mbps'
        do
            for load in 0.3 0.8 1 2
            do
                ./waf --run "wan --PROPOGATION_DELAY=$delay --LOSS_RATE=$loss_rate --ES_BANDWIDTH=$bandwidth --SS_BANDWIDTH=$bandwidth --LOAD=$load"
            done
        done
    done
done