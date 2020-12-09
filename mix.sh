#!/bin/bash

for dc_delay in '1us' '5us' '10us' '20us'
do 
    for wan_delay in '1ms' '2.5ms' '6.25ms' '12.5ms'
    do
        for loss_rate in 0 0.0005 0.005 0.025
            do
                for dc_bandwidth in '1Gbps' '10Gbps' '40Gbps' 
                do
                    for wan_bandwidth in '10Mbps' '100Mbps' '1000Mbps'
                    do 
                        for load in 0.3 0.5 0.8
                        do
                            ./waf --run "mix --DC_DELAY=$dc_delay --WAN_DELAY=$wan_delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$dc_bandwidth --ES_BANDWIDTH=$wan_bandwidth --SS_BANDWIDTH=$wan_bandwidth --LOAD=$load"
                        done
                    done
                done
            done
    done
done