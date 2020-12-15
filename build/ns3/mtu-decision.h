#ifndef MTU_DECISION_H
#define MTU_DECISION_H

#include <vector>
#include <string>
#include "ns3/type-id.h"
#include "ns3/object.h"

namespace ns3
{
    /**
     * find the most reasonable MTU in different enviroment
     * the time precision is ms
    */
    class MtuDecision : public Object
    //the time is unified as ms
    {
    public:
        MtuDecision();

        ~MtuDecision();

        static TypeId GetTypeId();

        /**
         * delay at tx host
        */
        double ComputeHostDelay(double queuingDelay, double transDelay);

        /**
         * DC: LEAF_SPINE Delay_prop*8 + Delay_trans*3 +Delay_process*3
         * WAN: Num of switches * process_time + (NumOfSwitches + 1) * 2 * Delay_prop_wan
         * MIX: Delay_prop_dc * 4 + Delay_process_dc * 2 + NumOfSwitches * Delay_process_wan + (NumOfSwitches + 1) * 2 * Delay_prop_wan
        */
        double ComputeRTTInDC(double delay_prop, double delay_trans, double delay_process);
        double ComputeRTTInWAN(uint32_t numOfSwitches, double delay_prop, double delay_trans, double delay_process);
        double ComputeRTTInMix(double delay_prop_dc, double delay_prop_wan, double delay_process_wan, double delay_process_dc, double delay_trans_dc,
                               double delay_trans_wan, uint32_t numOfSwitches);

        /**
         * computeInitialRound use the initial set params to compute
         * trans_time for single packet = (Mtu+38)*8/bandwidth*1000
         * in the next part directly use the rtt as the round
        */
        double ComputeInitialRound(int mtu, int bytesInQueue, int numOfSwitches, int singleHopProp, int singleHopProcess, double rxDelay);
        // double ComputeRound(double delay_tx, double delay_rx, double RTT);

        /**
         * \param flowSize the size of flow
         * \param mtu
         * \param round the time from sending the packet until sending the next cwnd
         * \param cwnd current cwnd
        */
        double ComputeFCT(int flowSize, int mtu, double round, uint32_t cwnd);

        /**
         * find the best mtu with different RTT and flowsize
         * In findBestMtuInMix==> end_tx = wan or dc
        */
        int FindInitialBestMtu(int flowSize, int bytesInQueue, int numOfSwitches, int singleHopProp, int singleHopProcess, double rxDelay, uint32_t cwnd);
        int FindBestMtu(int flowSize, double round, uint32_t cwnd);
        // int FindBestMtuInDC(int flowsize, int bandwidth, double delay_prop, double delay_process, double delay_tx, double delay_rx);
        // int FindBestMtuInWAN(int flowsize, int numOfSwitches, int bandwidth, double delay_prop, double delay_process, double delay_tx, double delay_rx);
        // int FindBestMtuInMix(int flowsize, int numOfSwitches, int bandwidth_dc, int bandwidth_wan, double delay_prop_dc, double delay_prop_wan,
        //                      double delay_process_dc, double delay_process_wan, double delay_tx, double delay_rx, std::string src_tx);

        /**
         * number of packets transmitted in a round
        */
        int ComputePacketNum(double roundTime, int Mtu, int bandwidth);

        /**
         * continuous sending data from the n+1 round, compute the number n
        */
        int ComputeRoundNum(int packetNum, int segmentsNum);

        /**
         * the rounds that are unfilled
        */
        double ComputeFullTime(double roundTime, int roundNum);

        /**
         * set the MTU set
        */
        void SetMtu(std::vector<int> array_mtu);

        /**
         * get the MTU set
        */
        std::vector<int> GetMtu();

        // static std::vector<double> getEfficiency();

    private:
        std::vector<int> array_mtu;
        // static std::vector<double> array_efficiency;
    };

    //initialized --if not --id returned exit1 status
    // int mtus[] = {1500, 3000, 4500, 6000, 7500, 9000};
    // std::vector<int> mtu_vector(mtus, mtus + 6);
    // std::vector<int> MtuDecision::array_mtu = mtu_vector;
} // namespace ns3

#endif