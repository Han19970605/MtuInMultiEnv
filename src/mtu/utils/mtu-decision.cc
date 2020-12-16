#include "mtu-decision.h"
#include "ns3/log.h"
#include <cmath>
#include <iostream>
extern uint64_t bandwidth;
extern uint32_t initialCwnd;

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("MtuDecision");
    NS_OBJECT_ENSURE_REGISTERED(MtuDecision);

    MtuDecision::MtuDecision()
    {
        int mtus[] = {1540, 3040, 4540, 9040};
        std::vector<int> mtu_vector(mtus, mtus + sizeof(mtus) / sizeof(mtus[0]));
        array_mtu = mtu_vector;
    }

    MtuDecision::~MtuDecision()
    {
        NS_LOG_INFO(this);
    }

    TypeId MtuDecision::GetTypeId()
    {
        TypeId tid = TypeId("ns3::MtuDecision")
                         .SetGroupName("mtu")
                         .AddConstructor<MtuDecision>();
        return tid;
    }

    double MtuDecision::ComputeHostDelay(double queuingDelay, double transDelay)
    {
        return queuingDelay + transDelay;
    }

    double MtuDecision::ComputeRTTInDC(double delay_prop, double delay_trans, double delay_process)
    {
        return delay_prop * 8 + delay_trans * 3 + delay_process * 3;
    }

    double MtuDecision::ComputeRTTInWAN(uint32_t numOfSwitches, double delay_prop, double delay_trans, double delay_process)
    {
        return numOfSwitches * (delay_process + delay_trans) + (numOfSwitches + 1) * 2 * delay_prop;
    }

    double MtuDecision::ComputeRTTInMix(double delay_prop_dc, double delay_prop_wan, double delay_process_wan, double delay_process_dc, double delay_trans_dc,
                                        double delay_trans_wan, uint32_t numOfSwitches)
    {
        double delay_dc = delay_prop_dc * 4 + (delay_process_dc + delay_trans_dc) * 2;
        double delay_wan = numOfSwitches * (delay_process_wan + delay_process_dc) + (numOfSwitches + 1) * 2 * delay_process_wan;
        return delay_dc + delay_wan;
    }

    /**
     * TIME FORMAT TO ms
     * applicable to mtu and wan 
    */
    double MtuDecision::ComputeInitialRound(int mtu, int bytesInQueue, int numOfSwitches, double singleHopProp, double singleHopProcess, double rxDelay)
    {
        double transDelay = double(mtu + 38) * 8 / double(bandwidth) * 1000;
        double queuingDelay = double(bytesInQueue) * 8 / double(bandwidth) * 1000;

        double round = queuingDelay + (numOfSwitches + 1) * 2 * singleHopProp + numOfSwitches * singleHopProcess +
                       numOfSwitches * transDelay + rxDelay;

        return round;
    }

    double MtuDecision::ComputeFCT(int flowSize, int mtu, double round, uint32_t cwnd)
    {
        double FCT;
        int mss = mtu - 40;
        double trans_time = double(mtu + 38) * 8 / double(bandwidth) * 1000;

        //can fulfill these packets
        int packets = flowSize / mss;
        int remainBytes = flowSize - packets * mss;

        int roundNum = ComputeRoundNum(ComputePacketNum(round, mtu, bandwidth), cwnd / mss);
        int packetNum = pow(2, roundNum) - 1;

        // end transmission before fulfill the bandwidth
        if (packets < packetNum)
        {
            int number_round = floor(log(packets + 1) / log(2));
            //packets sent in complete round
            int number_packet = pow(2, number_round) - 1;
            int remain_packets = packets - number_packet;
            // round * n + time for remaining packets + time for remaining bytes
            //unified to ms
            FCT = number_round * round + trans_time * remain_packets + double(remainBytes + 38) * 8 / double(bandwidth) * 1000 + round - trans_time;
        }
        else
        {
            int remain_packets = packets - packetNum;
            FCT = roundNum * round + remain_packets * trans_time + double(remainBytes + 38) * 8 / double(bandwidth) * 1000 + round - trans_time;
        }

        return FCT;
    }

    int MtuDecision::FindInitialBestMtu(int flowSize, int bytesInQueue, int numOfSwitches, double singleHopProp, double singleHopProcess, double rxDelay, uint32_t cwnd)
    {
        int bestMTU = array_mtu[0];
        double round = ComputeInitialRound(bestMTU, bytesInQueue, numOfSwitches, singleHopProp, singleHopProcess, rxDelay);
        double FCT = ComputeFCT(flowSize, bestMTU, round, cwnd);

        for (int i = 0; i < array_mtu.size(); i++)
        {
            // trans_delay = (array_mtu[i] + 38) * 8 / bandwidth * 1000;
            round = ComputeInitialRound(array_mtu[i], bytesInQueue, numOfSwitches, singleHopProp, singleHopProcess, rxDelay);
            if (ComputeFCT(flowSize, array_mtu[i], round, cwnd) < FCT)
            {
                bestMTU = array_mtu[i];
                FCT = ComputeFCT(flowSize, array_mtu[i], round, cwnd);
            }
            // std::cout << "the mtu is " << array_mtu[i] << "the fct is: " << ComputeFCT(flowSize, array_mtu[i], round, cwnd) << " " << round << std::endl;
        }
        // std::cout << "the best mtu is " << bestMTU << std::endl;
        return bestMTU;
    }

    int MtuDecision::FindBestMtu(int flowSize, double round, uint32_t cwnd)
    {
        int bestMTU = array_mtu[0];

        double FCT = ComputeFCT(flowSize, bestMTU, round, cwnd);

        for (int i = 0; i < array_mtu.size(); i++)
        {
            if (array_mtu[i] > cwnd)
                break;
            // trans_delay = (array_mtu[i] + 38) * 8 / bandwidth * 1000;
            if (ComputeFCT(flowSize, array_mtu[i], round, cwnd) < FCT)
            {
                bestMTU = array_mtu[i];
                FCT = ComputeFCT(flowSize, array_mtu[i], round, cwnd);
            }
        }
        return bestMTU;
    }

    int MtuDecision::ComputePacketNum(double roundTime, int Mtu, int bandwidth)
    {
        double trans_time = double(Mtu + 38) * 8 / double(bandwidth) * 1000;
        int packetNum = ceil(roundTime / trans_time);
        return packetNum;
    }

    int MtuDecision::ComputeRoundNum(int packetNum, int segmentsNum)
    {
        double roundNum = log((packetNum / segmentsNum) + 1) / log(2);
        // double roundNum = log(packetNum) / log(2) + 1;
        int number = floor(roundNum);
        return number;
    }

    double MtuDecision::ComputeFullTime(double roundTime, int roundNum)
    {
        return roundTime * roundNum;
    }

    void MtuDecision::SetMtu(std::vector<int> mtus)
    {
        array_mtu = mtus;
        // std::vector<double> efficiency;
        // for (int i = 0; i < mtus.size(); i++)
        // {
        //     double e = double(mtus[i] - 40) / double(mtus[i] + 38);
        //     efficiency.push_back(e);
        // }
        // array_efficiency = efficiency;
    }

    std::vector<int> MtuDecision::GetMtu()
    {
        return array_mtu;
    }

} // namespace ns3

// double MtuDecision::ComputeRound(double delay_tx, double delay_rx, double RTT)
// {
//     return delay_tx + delay_rx + RTT;
// }

// double MtuDecision::ComputeFCT(int flowsize, int bandwidth, int mtu, double round)
// {
//     double FCT;
//     int mss = mtu - 40;
//     double trans_time = double(mtu + 38) * 8 / double(bandwidth) * 1000;

//     //can fulfill these packets
//     int packets = flowsize / mss;
//     int remainBytes = flowsize - packets * mss;

//     /**
//      * delay_tx should contain the tx delay in NIC
//      * if delay_tx = 0
//      * we set delay_tx to tx_delay in NIC
//     */
//     double tx_delay = delay_tx;
//     if (delay_tx == 0)
//     {
//         tx_delay = trans_time;
//     }

//     double round = ComputeRound(tx_delay, delay_rx, RTT);

//     int roundNum = ComputeRoundNum(ComputePacketNum(round, mtu, bandwidth));
//     int packetNum = pow(2, roundNum) - 1;

//     // end transmission before fulfill the bandwidth
//     if (packets < packetNum)
//     {
//         int number_round = floor(log(packets + 1) / log(2));
//         //packets sent in complete round
//         int number_packet = pow(2, number_round) - 1;
//         int remain_packets = packets - number_packet;
//         // round * n + time for remaining packets + time for remaining bytes
//         //unified to ms
//         FCT = number_round * round + trans_time * remain_packets + double(remainBytes + 38) * 8 / double(bandwidth) * 1000 + RTT;
//     }
//     else
//     {
//         int remain_packets = packets - packetNum;
//         FCT = roundNum * round + remain_packets * trans_time + double(remainBytes + 38) * 8 / double(bandwidth) * 1000 + RTT;
//     }

//     return FCT;
// }

// int MtuDecision::FindBestMtu(int flowsize, int bandwidth, double RTT, double delay_tx, double delay_rx)
// {
//     int bestMTU = array_mtu[0];
//     double FCT = ComputeFCT(flowsize, bandwidth, array_mtu[0], RTT, delay_tx, delay_rx);
//     for (int i = 0; i < array_mtu.size(); i++)
//     {
//         if (ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx) < FCT)
//         {
//             bestMTU = array_mtu[i];
//             FCT = ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx);
//         }
//     }
//     return bestMTU;
// }
// int MtuDecision::FindBestMtuInDC(int flowsize, int bandwidth, double delay_prop, double delay_process, double delay_tx, double delay_rx)
// {
//     int bestMTU = array_mtu[0];
//     double trans_delay = (bestMTU + 38) * 8 / bandwidth * 1000;
//     double RTT = ComputeRTTInDC(delay_prop, trans_delay, delay_process);
//     double FCT = ComputeFCT(flowsize, bandwidth, bestMTU, RTT, delay_tx, delay_rx);
//     for (int i = 0; i < array_mtu.size(); i++)
//     {
//         trans_delay = (array_mtu[i] + 38) * 8 / bandwidth * 1000;
//         RTT = ComputeRTTInDC(delay_prop, trans_delay, delay_process);
//         if (ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx) < FCT)
//         {
//             bestMTU = array_mtu[i];
//             FCT = ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx);
//         }
//     }
//     return bestMTU;
// }

// int MtuDecision::FindBestMtuInWAN(int flowsize, int numOfSwitches, int bandwidth, double delay_prop, double delay_process, double delay_tx, double delay_rx)
// {
//     int bestMTU = array_mtu[0];
//     double trans_delay = (bestMTU + 38) * 8 / bandwidth * 1000;
//     double RTT = ComputeRTTInWAN(numOfSwitches, delay_prop, trans_delay, delay_process);
//     double FCT = ComputeFCT(flowsize, bandwidth, bestMTU, RTT, delay_tx, delay_rx);
//     for (int i = 0; i < array_mtu.size(); i++)
//     {
//         trans_delay = (array_mtu[i] + 38) * 8 / bandwidth * 1000;
//         RTT = ComputeRTTInWAN(numOfSwitches, delay_prop, trans_delay, delay_process);
//         if (ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx) < FCT)
//         {
//             bestMTU = array_mtu[i];
//             FCT = ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx);
//         }
//     }

//     return bestMTU;
// }

// int MtuDecision::FindBestMtuInMix(int flowsize, int numOfSwitches, int bandwidth_dc, int bandwidth_wan, double delay_prop_dc, double delay_prop_wan,
//                                   double delay_process_dc, double delay_process_wan, double delay_tx, double delay_rx, std::string src_tx)
// {
//     int bestMTU = array_mtu[0];
//     double trans_delay_dc = (bestMTU + 38) * 8 / bandwidth_dc * 1000;
//     double trans_delay_wan = (bestMTU + 38) * 8 / bandwidth_wan * 1000;
//     double RTT = ComputeRTTInMix(delay_prop_dc, delay_prop_wan, delay_process_wan, delay_process_dc, trans_delay_dc, trans_delay_wan, numOfSwitches);
//     int bandwidth;
//     if (src_tx == "wan")
//     {
//         bandwidth = bandwidth_wan;
//     }
//     else if (src_tx == "dc")
//     {
//         bandwidth = bandwidth_dc;
//     }
//     double FCT = ComputeFCT(flowsize, bandwidth, bestMTU, RTT, delay_tx, delay_rx);
//     for (int i = 0; i < array_mtu.size(); i++)
//     {
//         trans_delay_dc = (array_mtu[i] + 38) * 8 / bandwidth_dc * 1000;
//         trans_delay_wan = (array_mtu[i] + 38) * 8 / bandwidth_wan * 1000;
//         RTT = ComputeRTTInMix(delay_prop_dc, delay_prop_wan, delay_process_wan, delay_process_dc, trans_delay_dc, trans_delay_wan, numOfSwitches);
//         if (ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx) < FCT)
//         {
//             bestMTU = array_mtu[i];
//             FCT = ComputeFCT(flowsize, bandwidth, array_mtu[i], RTT, delay_tx, delay_rx);
//         }
//     }
//     return bestMTU;
// }