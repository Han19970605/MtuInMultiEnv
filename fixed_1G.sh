#!/bin/bash

for loss in 0 0.01 0.05 0.1
do
    for mode in 0 1 2 3 4 5
    do
        ./waf --run "fixed_flow --DELAY='100us' --LOSS_RATE=$loss --BANDWIDTH_LINK='1Gbps' --LOAD=0.1 --MODE=$mode"
        ./waf --run "fixed_flow --DELAY='100us' --LOSS_RATE=$loss --BANDWIDTH_LINK='1Gbps' --LOAD=0.3 --MODE=$mode"
        ./waf --run "fixed_flow --DELAY='100us' --LOSS_RATE=$loss --BANDWIDTH_LINK='1Gbps' --LOAD=0.5 --MODE=$mode"
        ./waf --run "fixed_flow --DELAY='10us' --LOSS_RATE=$loss --BANDWIDTH_LINK='10Gbps' --LOAD=0.1 --MODE=$mode"
        ./waf --run "fixed_flow --DELAY='10us' --LOSS_RATE=$loss --BANDWIDTH_LINK='10Gbps' --LOAD=0.3 --MODE=$mode"
        ./waf --run "fixed_flow --DELAY='10us' --LOSS_RATE=$loss --BANDWIDTH_LINK='10Gbps' --LOAD=0.5 --MODE=$mode"
    done
done

