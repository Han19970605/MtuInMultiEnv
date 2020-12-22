#include "mtu-net-helper.h"
#include "ns3/mtu-bulksend-application.h"
#include "ns3/mtu-utility.h"

#include "ns3/application-container.h"
#include "ns3/string.h"
#include "ns3/mtu-decision.h"
#include "ns3/mtu-net-device.h"
#include "ns3/multi-queue.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"
#include "ns3/point-to-point-module.h"
#include "ns3/packet-sink.h"

#include <iostream>
#include <fstream>

/**
 * \brief params needed in current mechanism
 * 
 * \param mode define whther adjust the mtu or priority
 *        --- 0 open the priority, open the mtu adjustment
 *        --- 1 open the priority, only choose the mtu at first
 *        --- 2 open the priority, use 1500 as the MTU
 *        --- 3 close the priority, open the mtu adjustment
 *        --- 4 close the priority, only choose the mtu at first
 *        --- 5 close the priority, use 1500 as the MTU
 * \param adjust_interval check whether adjust the sending size every x interval
 * \param bandwidth externed by mtu-net-helper and tcp-socket-base
 * \param netdeviceQ_length queue length in netdevice, one device for each host, the id can be get by GetId()
 * \param flowInfo use the given flowInfo obtained from the csv files
*/
uint32_t mode = 1;
int adjust_interval = 6;
std::string BANDWIDTH_LINK = "10Gbps";
uint64_t bandwidth = DataRate(BANDWIDTH_LINK).GetBitRate();
std::map<int, int> netdeviceQ_length;
uint32_t initialCwnd = 9000;
std::string TCP_PROTOCOL = "ns3::TcpNewReno";
double LOSS_RATE = 0;
double LOAD = 0.3;
std::string PROPOGATION_DELAY = "100us";
int numOfSwitches = 3;

struct flow
{
    double startTime;
    uint64_t flowSize;
};
std::vector<flow> flowInfo;

// #define PROCESS_DELAY()

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("MtuNetHelper");
    NS_OBJECT_ENSURE_REGISTERED(MtuNetHelper);

    MtuNetHelper::MtuNetHelper()
    {
        m_sendApplicationFactory.SetTypeId("ns3::MtuBulkSendApplication");
        m_sinkApplicationFactory.SetTypeId("ns3::PacketSink");

        m_mtuDeviceFactory.SetTypeId("ns3::MtuNetDevice");
        m_pointFactory.SetTypeId("ns3::PointToPointNetDevice");

        m_channelFactory.SetTypeId("ns3::PointToPointChannel");
        m_remoteChannelFactory.SetTypeId("ns3::PointToPointRemoteChannel");

        m_normalQueueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
        m_queueFactory.SetTypeId("ns3::MultiQueue");
    }

    MtuNetHelper::~MtuNetHelper() {}

    ApplicationContainer MtuNetHelper::InstallApplication(Ptr<Node> sendNode, Ptr<Node> sinkNode, Ipv4Address remoteAddress,
                                                          uint16_t port, uint32_t maxBytes, uint32_t sendSize, uint32_t priority,
                                                          uint32_t flowCount, uint64_t bandwidth)
    {
        ApplicationContainer applications;
        m_sendApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_sendApplicationFactory.Set("Remote", AddressValue(InetSocketAddress(remoteAddress, port)));
        m_sendApplicationFactory.Set("MaxBytes", UintegerValue(maxBytes));
        m_sendApplicationFactory.Set("SendSize", UintegerValue(sendSize));

        Ptr<MtuBulkSendApplication> sendApplication = m_sendApplicationFactory.Create<MtuBulkSendApplication>();
        sendApplication->SetPriority(priority);
        sendApplication->SetSequenceNumber(flowCount);
        sendApplication->SetNdBandwidth(bandwidth);
        // sendApplication->SetSegmentSize(sendSize);
        sendNode->AddApplication(sendApplication);

        m_sinkApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_sinkApplicationFactory.Set("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), port)));

        Ptr<PacketSink> sinkApplication = m_sinkApplicationFactory.Create<PacketSink>();
        sinkNode->AddApplication(sinkApplication);

        applications.Add(sendApplication);
        applications.Add(sinkApplication);

        return applications;
    }

    // read the info of flows from the csv file
    // adapt to wan and datacenter
    void MtuNetHelper::InstallAllApplications(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                              std::vector<Ipv4Address> destAddresses, uint32_t &flowCount, int port_start, int port_end,
                                              double timesim_start, double timesim_end, double time_flow_launch_end, double delay_prop,
                                              double delay_process)
    {
        int src_leaf_node_count = fromServers.GetN();
        int dst_leaf_node_count = destServers.GetN();
        uint16_t port = port_start;
        flowCount = flowInfo.size();
        std::cout << "begin install applications" << std::endl;

        for (int i = 0; i < flowInfo.size(); i++)
        {
            port = port_start + i;

            if (port >= port_end)
            {
                // std::cout << "the port is bigger than port end" << port << std::endl;
                break;
            }

            flow f = flowInfo[i];
            double startTime = f.startTime;
            uint64_t flowSize = f.flowSize;
            uint32_t priority = 0;
            if (mode == 0 || mode == 1 || mode == 2)
                priority = MtuUtility::gen_priority(flowSize);

            // select the src and dst
            int srcIndex = MtuUtility::rand_range(0, src_leaf_node_count - 1);
            int dstIndex = srcIndex;
            while (dstIndex == srcIndex)
            {
                dstIndex = MtuUtility::rand_range(0, dst_leaf_node_count - 1);
            }

            if (port == 49153)
            {
                std::cout << src_leaf_node_count << " " << dst_leaf_node_count << " " << srcIndex << " " << dstIndex << std::endl;
            }

            int size = 1460;
            if ((mode != 2) && (mode != 5))
            {
                // choose the best mtu
                MtuDecision *md = new MtuDecision();
                int bytesInQueue = DynamicCast<MultiQueue>(DynamicCast<MtuNetDevice>(fromServers.Get(srcIndex)->GetDevice(1))->GetQueue())->GetPktsAhead(priority);
                int mtu = md->FindInitialBestMtu(flowSize, bytesInQueue, 3, delay_prop, delay_process, 0, initialCwnd);
                // int mtu = md->FindBestMtuInDC(flowSize, bandwidth, delay_prop, 0, tx_delay, 0);
                // std::cout << "the best mtu is " << mtu << std::endl;
                size = mtu - 40;
            }

            ApplicationContainer applications;
            applications = InstallApplication(fromServers.Get(srcIndex), destServers.Get(dstIndex), destAddresses[dstIndex], port, flowSize, size, priority,
                                              i, bandwidth);
            applications.Start(Seconds(startTime));
            applications.Stop(Seconds(timesim_end));
        }
    }

    //install applications in different enviroments
    void MtuNetHelper::InstallAllApplicationsInDC(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                                  std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                                  double timesim_end, double time_flow_launch_end, uint64_t bandwidth, double delay_prop, double delay_process,
                                                  double delay_tx, double delay_rx)
    {
        int src_leaf_node_count = fromServers.GetN();
        int dst_leaf_node_count = destServers.GetN();
        uint16_t port = port_start;

        for (int i = 0; i < src_leaf_node_count; i++)
        {
            double startTime = timesim_start + MtuUtility::poission_gen_interval(requestRate); //加泊松分布
            while (startTime < time_flow_launch_end)
            {
                flowCount++;
                if (port > port_end)
                    break;
                int destIndex = MtuUtility::rand_range(0, dst_leaf_node_count - 1);
                // generate flowsize base on the cdf
                uint32_t flowSize = MtuUtility::gen_random_cdf(cdfTable);
                // generate priority base on the flowsize
                uint32_t priority = 0;
                priority = MtuUtility::gen_priority(flowSize);

                // select the best sendsize
                int size = 1460;
                MtuDecision *md = new MtuDecision();
                // int bestMtu = md->FindBestMtuInDC(flowSize, bandwidth, delay_prop, delay_process, delay_tx, delay_rx);
                // size = bestMtu - 40;

                ApplicationContainer applications;
                applications = InstallApplication(fromServers.Get(i), destServers.Get(destIndex), destAddress[destIndex],
                                                  port, flowSize, size, priority, flowCount, bandwidth);

                applications.Start(Seconds(startTime));
                applications.Stop(Seconds(timesim_end));
                // applications.Get(0)->SetStartTime(Seconds(startTime));
                // applications.Get(0)->SetStopTime(Seconds(timesim_end));
                // applications.Get(1)->SetStartTime(Seconds(timesim_start));
                // applications.Get(1)->SetStopTime(Seconds(timesim_end));

                startTime += MtuUtility::poission_gen_interval(requestRate);
                port++;
            }
        }
        NS_LOG_INFO("Finish installing applications!");
    }

    void MtuNetHelper::InstallAllApplicationsInWAN(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                                   std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                                   double timesim_end, double time_flow_launch_end, uint64_t bandwidth, int numOfSwitches, double delay_prop,
                                                   double delay_process, double delay_tx, double delay_rx)
    {
        int src_leaf_node_count = fromServers.GetN();
        int dst_leaf_node_count = destServers.GetN();
        uint16_t port = port_start;
        for (int i = 0; i < src_leaf_node_count; i++)
        {
            double startTime = timesim_start + MtuUtility::poission_gen_interval(requestRate); //加泊松分布
            while (startTime < time_flow_launch_end)
            {
                flowCount++;
                if (port > port_end)
                    break;
                int destIndex = MtuUtility::rand_range(0, dst_leaf_node_count - 1);
                // generate flowsize base on the cdf
                uint32_t flowSize = MtuUtility::gen_random_cdf(cdfTable);
                // generate priority base on the flowsize
                uint32_t priority = 0;
                priority = MtuUtility::gen_priority(flowSize);

                std::string file_path = "./data/wangen_flow_";
                file_path = file_path.append(std::to_string(bandwidth / 1000000));
                file_path = file_path.append(std::string("Mbps.csv"));
                std::ofstream file(file_path, std::ios::app);
                file << startTime << "," << flowSize << "\n";

                // select the best sendsize
                int size = 1460;
                MtuDecision *md = new MtuDecision();
                // int bestMtu = md->FindBestMtuInWAN(flowSize, numOfSwitches, bandwidth, delay_prop, delay_process, delay_tx, delay_rx);
                // size = bestMtu - 40;

                ApplicationContainer applications;
                applications = InstallApplication(fromServers.Get(i), destServers.Get(destIndex), destAddress[destIndex],
                                                  port, flowSize, size, priority, flowCount, bandwidth);

                applications.Get(0)->SetStartTime(Seconds(startTime));
                applications.Get(0)->SetStopTime(Seconds(timesim_end));
                applications.Get(1)->SetStartTime(Seconds(timesim_start));
                applications.Get(1)->SetStopTime(Seconds(timesim_end));

                startTime += MtuUtility::poission_gen_interval(requestRate);
                port++;
            }
        }
        NS_LOG_INFO("Finish installing applications!");
    }

    void MtuNetHelper::InstallAllApplicationsInMix(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                                   std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                                   double timesim_end, double time_flow_launch_end, uint64_t bandwidth_dc, uint64_t bandwidth_wan, double delay_prop_dc, double delay_prop_wan, double delay_process_wan,
                                                   double delay_process_dc, int numOfSwitches, double delay_tx, double delay_rx, std::string src_type)
    {
        int src_leaf_node_count = fromServers.GetN();
        int dst_leaf_node_count = destServers.GetN();
        uint16_t port = port_start;
        for (int i = 0; i < src_leaf_node_count; i++)
        {
            double startTime = timesim_start + MtuUtility::poission_gen_interval(requestRate); //加泊松分布
            while (startTime < time_flow_launch_end)
            {
                flowCount++;
                if (port > port_end)
                    break;
                int destIndex = MtuUtility::rand_range(0, dst_leaf_node_count - 1);
                // generate flowsize base on the cdf
                uint32_t flowSize = MtuUtility::gen_random_cdf(cdfTable);
                // generate priority base on the flowsize
                uint32_t priority = 0;
                priority = MtuUtility::gen_priority(flowSize);

                // if the dst server in WAN select the best sendsize
                int size = 1460;
                MtuDecision *md = new MtuDecision();
                // int bestMtu = md->FindBestMtuInMix(flowSize, numOfSwitches, bandwidth_dc, bandwidth_wan, delay_prop_dc,
                //    delay_prop_wan, delay_process_dc, delay_process_wan, delay_tx, delay_rx, src_type);
                // size = bestMtu - 40;

                ApplicationContainer applications;
                applications = InstallApplication(fromServers.Get(i), destServers.Get(destIndex), destAddress[destIndex],
                                                  port, flowSize, size, priority, flowCount, bandwidth_dc);

                applications.Get(0)->SetStartTime(Seconds(startTime));
                applications.Get(0)->SetStopTime(Seconds(timesim_end));
                applications.Get(1)->SetStartTime(Seconds(timesim_start));
                applications.Get(1)->SetStopTime(Seconds(timesim_end));

                startTime += MtuUtility::poission_gen_interval(requestRate);
                port++;
            }
        }
        NS_LOG_INFO("Finish installing applications!");
    }

    /**
     * set different attributes for queue, device and channel
    */
    void MtuNetHelper::SetQueue(std::string type,
                                std::string n1, const AttributeValue &v1,
                                std::string n2, const AttributeValue &v2,
                                std::string n3, const AttributeValue &v3,
                                std::string n4, const AttributeValue &v4)
    {
        QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

        m_queueFactory.SetTypeId(type);
        m_queueFactory.Set(n1, v1);
        m_queueFactory.Set(n2, v2);
        m_queueFactory.Set(n3, v3);
        m_queueFactory.Set(n4, v4);
    }

    void MtuNetHelper::SetDeviceAttribute(std::string n1, const AttributeValue &v1)
    {

        m_mtuDeviceFactory.Set(n1, v1);
        m_pointFactory.Set(n1, v1);
    }

    void MtuNetHelper::SetChannelAttribute(std::string n1, const AttributeValue &v1)
    {
        m_channelFactory.Set(n1, v1);
        m_remoteChannelFactory.Set(n1, v1);
    }

    NetDeviceContainer MtuNetHelper::InstallNormalNetDevices(Ptr<Node> a, Ptr<Node> b)
    {
        NetDeviceContainer container;
        Ptr<MtuNetDevice> dev_a = m_mtuDeviceFactory.Create<MtuNetDevice>();
        dev_a->setLossRate(0);
        dev_a->SetAddress(Mac48Address::Allocate());
        dev_a->data_fileName = data_fileName;
        dev_a->rtt = rtt;
        Ptr<MultiQueue> queueA = m_queueFactory.Create<MultiQueue>();
        // queueA->SetMaxPackets(100);
        dev_a->SetQueue(queueA);

        Ptr<MtuNetDevice> dev_b = m_mtuDeviceFactory.Create<MtuNetDevice>();
        dev_b->setLossRate(0);
        dev_b->SetAddress(Mac48Address::Allocate());
        dev_b->data_fileName = data_fileName;
        dev_b->rtt = rtt;
        Ptr<MultiQueue> queueB = m_queueFactory.Create<MultiQueue>();
        // queueB->SetMaxPackets(100);
        dev_b->SetQueue(queueB);

        bool useNormalChannel = true;
        Ptr<PointToPointChannel> channel = 0;

        if (MpiInterface::IsEnabled())
        {
            uint32_t n1SystemId = a->GetSystemId();
            uint32_t n2SystemId = b->GetSystemId();
            uint32_t currSystemId = MpiInterface::GetSystemId();
            if (n1SystemId != currSystemId || n2SystemId != currSystemId)
            {
                useNormalChannel = false;
            }
        }
        if (useNormalChannel)
        {
            channel = m_channelFactory.Create<PointToPointChannel>();
        }
        else
        {
            channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel>();
            Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver>();
            Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver>();
            mpiRecA->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_a));
            mpiRecB->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_b));
            dev_a->AggregateObject(mpiRecA);
            dev_b->AggregateObject(mpiRecB);
        }

        a->AddDevice(dev_a);
        b->AddDevice(dev_b);

        dev_a->Attach(channel);
        dev_b->Attach(channel);

        container.Add(dev_a);
        container.Add(dev_b);

        return container;
    }

    NetDeviceContainer MtuNetHelper::InstallMtuNetDevices(Ptr<Node> loss_node, Ptr<Node> normal_node, double loss_rate)
    {
        NetDeviceContainer container;
        Ptr<MtuNetDevice> dev_a = m_mtuDeviceFactory.Create<MtuNetDevice>();
        dev_a->SetAddress(Mac48Address::Allocate());
        dev_a->setLossRate(loss_rate);
        dev_a->data_fileName = data_fileName;
        dev_a->rtt = rtt;
        // Ptr<MultiQueue> queueA = m_queueFactory.Create<MultiQueue>();
        // queueA->SetMaxPackets(100);
        Ptr<MultiQueue> queueA = m_queueFactory.Create<MultiQueue>();
        queueA->SetMaxPackets(100);
        dev_a->SetQueue(queueA);
        loss_node->AddDevice(dev_a);

        Ptr<MtuNetDevice> dev_b = m_mtuDeviceFactory.Create<MtuNetDevice>();
        dev_b->setLossRate(0);
        dev_b->SetAddress(Mac48Address::Allocate());
        dev_b->data_fileName = data_fileName;
        dev_b->rtt = rtt;
        // Ptr<MultiQueue> queueB = m_queueFactory.Create<MultiQueue>();
        // queueB->SetMaxPackets(100);
        Ptr<MultiQueue> queueB = m_queueFactory.Create<MultiQueue>();
        queueB->SetMaxPackets(100);
        dev_b->SetQueue(queueB);
        normal_node->AddDevice(dev_b);

        bool useNormalChannel = true;
        Ptr<PointToPointChannel> channel = 0;

        if (MpiInterface::IsEnabled())
        {
            uint32_t n1SystemId = loss_node->GetSystemId();
            uint32_t n2SystemId = normal_node->GetSystemId();
            uint32_t currSystemId = MpiInterface::GetSystemId();
            if (n1SystemId != currSystemId || n2SystemId != currSystemId)
            {
                useNormalChannel = false;
            }
        }
        if (useNormalChannel)
        {
            channel = m_channelFactory.Create<PointToPointChannel>();
        }
        else
        {
            channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel>();
            Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver>();
            Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver>();
            mpiRecA->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_a));
            mpiRecB->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_b));
            dev_a->AggregateObject(mpiRecA);
            dev_b->AggregateObject(mpiRecB);
        }

        dev_a->Attach(channel);
        dev_b->Attach(channel);

        container.Add(dev_a);
        container.Add(dev_b);

        return container;
    }

    NetDeviceContainer MtuNetHelper::InstallPointNetDevice(Ptr<Node> a, Ptr<Node> b)
    {
        NetDeviceContainer devices;

        Ptr<PointToPointNetDevice> devA = m_pointFactory.Create<PointToPointNetDevice>();
        devA->SetAddress(Mac48Address::Allocate());
        // Ptr<Queue<Packet>> queue_A = m_normalQueueFactory.Create<DropTailQueue<Packet>>();
        // queue_A->SetMaxPackets(100);
        Ptr<MultiQueue> queue_A = m_queueFactory.Create<MultiQueue>();
        queue_A->SetMaxPackets(100);
        devA->SetQueue(queue_A);

        a->AddDevice(devA);

        Ptr<PointToPointNetDevice> devB = m_pointFactory.Create<PointToPointNetDevice>();
        devB->SetAddress(Mac48Address::Allocate());
        // Ptr<Queue<Packet>> queue_B = m_normalQueueFactory.Create<DropTailQueue<Packet>>();
        // queue_B->SetMaxPackets(100);
        Ptr<MultiQueue> queue_B = m_queueFactory.Create<MultiQueue>();
        queue_B->SetMaxPackets(100);
        devB->SetQueue(queue_B);
        b->AddDevice(devB);

        // bool useNormalChannel = true;
        // Ptr<PointToPointChannel> channel = 0;

        // if (MpiInterface::IsEnabled())
        // {
        //     uint32_t n1SystemId = loss_node->GetSystemId();
        //     uint32_t n2SystemId = normal_node->GetSystemId();
        //     uint32_t currSystemId = MpiInterface::GetSystemId();
        //     if (n1SystemId != currSystemId || n2SystemId != currSystemId)
        //     {
        //         useNormalChannel = false;
        //     }
        // }
        // if (useNormalChannel)
        // {
        //     channel = m_channelFactory.Create<PointToPointChannel>();
        // }
        // else
        // {
        //     channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel>();
        //     Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver>();
        //     Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver>();
        //     mpiRecA->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_a));
        //     mpiRecB->SetReceiveCallback(MakeCallback(&PointToPointNetDevice::Receive, dev_b));
        //     dev_a->AggregateObject(mpiRecA);
        //     dev_b->AggregateObject(mpiRecB);
        // }

        // dev_a->Attach(channel);
        // dev_b->Attach(channel);

        // loss_node->AddDevice(dev_a);
        // loss_node->AddDevice(dev_b);

        // container.Add(dev_a);
        // container.Add(dev_b);
        Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel>();
        devA->Attach(channel);
        devB->Attach(channel);

        devices.Add(devA);
        devices.Add(devB);

        return devices;
    }

} // namespace ns3
