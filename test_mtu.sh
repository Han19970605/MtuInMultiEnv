#!/bin/bash

for delay in 
do 
    for loss_rate in 
    do
        for bandwidth in 
        do
            for load in 
            do
                ./waf --run "datacenter --DELAY=$delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$bandwidth --LOAD=$load"
            done
        done
    done
done

for delay in 
do 
    for loss_rate in 
    do
        for bandwidth in 
        do
            for load in 
            do
                ./waf --run "wan --DELAY=$delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$bandwidth --LOAD=$load"
            done
        done
    done
done

for dc_delay in 
do 
    for wan_delay in 
    do
        for loss_rate in 
            do
                for dc_bandwidth in 
                do
                    for wan_bandwidth in 
                    do 
                        for load in 
                        do
                            ./waf --run "mix --DELAY=$delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$bandwidth --LOAD=$load"
                        done
                    done
                done
            done
    done
done