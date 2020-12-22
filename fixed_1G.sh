#!/bin/bash

for mode in 0 1 2 3 4 5
do
    ./waf --run "fixed_flow --DELAY='100us' --LOSS_RATE=0 --BANDWIDTH_LINK='1Gbps' --LOAD=0.3 --MODE=$mode --END_TIME=1000"
done
# for delay in 
# do 
#     for loss_rate in 
#     do
#         for bandwidth in 
#         do
#             for load in     
#             do
#                 ./waf --run "fixed_flow --DELAY=$delay --LOSS_RATE=$loss_rate --BANDWIDTH_LINK=$bandwidth --LOAD=$load"
#             done
#         done
#     done
# done