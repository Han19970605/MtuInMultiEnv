#include "ns3/mtu-net-device.h"
#include "ns3/priority-tag.h"
#include "ns3/multi-queue.h"
#include "ns3/simulator.h"
#include <fstream>
#include <iostream>

// extern std::map<int, int> netdeviceQ_length;
extern std::string TCP_PROTOCOL;
extern double LOAD;
extern double LOSS_RATE;
extern std::string PROPOGATION_DELAY;

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("MtuNetDevice");
    NS_OBJECT_ENSURE_REGISTERED(MtuNetDevice);

    TypeId MtuNetDevice::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::MtuNetDevice")
                                .SetParent<PointToPointNetDevice>()
                                .SetGroupName("mtu")
                                .AddConstructor<MtuNetDevice>();
        return tid;
    }

    MtuNetDevice::MtuNetDevice()
        : loss_rate(0.0)
    {
        NS_LOG_FUNCTION(this);
    }

    MtuNetDevice::~MtuNetDevice()
    {
        NS_LOG_FUNCTION(this);
    }

    void MtuNetDevice::setLossRate(double lossRate)
    {
        loss_rate = lossRate;
    }

    double MtuNetDevice::getLossRate() const
    {
        return loss_rate;
    }

    bool MtuNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
    {
        // PriorityTag tag;
        // bool found = packet->PeekPacketTag(tag);

        // if (found)
        // {
        //     // std::cout << "packet sending" << std::endl;
        //     Ptr<MultiQueue> q = DynamicCast<MultiQueue>(this->GetQueue());
        //     uint64_t bytesInQueue = q->GetTotalNumber();

        //     if ((tag.GetTotalTag() + packet->GetSize()) >= tag.GetFlowsizeTag())
        //     {
        //         uint64_t fct = Simulator::Now().GetNanoSeconds() - tag.GetTimeStamp() + rtt;
        //         std::string fileName = (std::string("./data/fct/")).append(data_fileName).append(std::string(".csv"));
        //         // std::cout << fileName << std::endl;
        //         std::ofstream file(fileName, std::ios::app);
        //         uint32_t node_id = this->GetNode()->GetId();
        //         file << node_id << "," << tag.GetSeqTag() << "," << TCP_PROTOCOL << "," << loss_rate << "," << LOAD << "," << PROPOGATION_DELAY << ","
        //              << tag.GetFlowsizeTag() << "," << fct
        //              << "\n";
        //         file.close();

        //         //netdevice at the endhost
        //         // if (node_id < 32)
        //         // {
        //         //     netdeviceQ_length[node_id] = bytesInQueue;
        //         // }

        //         // std::string buffer_file = (std::string("./data/buffer/")).append(data_fileName).append(std::string("_buffer.csv"));
        //         // std::ofstream bufferFile(buffer_file, std::ios::app);
        //         // bufferFile << bytesInQueue << "\n";
        //         // bufferFile.close();
        //     }
        // }
        if (packet == 0)
        {
            std::cout << "packet is null " << std::endl;
        }

        if (loss_rate == 0)
        {
            return PointToPointNetDevice::Send(packet, dest, protocolNumber);
        }
        else
        {
            srand(time(0));
            double loss = double(rand()) / double(RAND_MAX);
            if (loss < loss_rate)
                return true;
            else
                return PointToPointNetDevice::Send(packet, dest, protocolNumber);
        }
    }

} // namespace ns3