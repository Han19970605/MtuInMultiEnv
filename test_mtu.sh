#!/bin/bash

./waf --run "fixed_flow --DELAY='5us' --LOSS_RATE=0 --BANDWIDTH_LINK='10Gbps' --LOAD=0.3"

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