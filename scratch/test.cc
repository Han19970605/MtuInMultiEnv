#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mtu-module.h"
#include "ns3/mtu-utility.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/flow-monitor-module.h"

#define START_TIME 0.0
#define END_TIME 20.0

// 128M
#define BUFFER_SIZE 134217728
#define MTU 9000
#define MSS MTU - 40
#define LOAD 0.8

#define TCP_PROTOCOL "ns3::TcpNewReno"
#define PROPOGATION_DELAY "100us"
#define BANDWIDTH_LINK "1Gbps"
#define DATA_RATE "1Gbps"

#define PORT_START 1000
#define PORT_END 60000

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DataCenter");

int main(int argc, char *argv[])
{
    LogComponentEnable("DataCenter", LOG_INFO);

    //generate cdf table
    std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    struct cdf_table *cdfTable = new cdf_table();
    MtuUtility::init_cdf(cdfTable);
    MtuUtility::load_cdf(cdfTable, cdfFileName.c_str());

    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(MSS));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(TCP_PROTOCOL));

    NodeContainer nodes, switches;
    nodes.Create(2);
    switches.Create(1);

    InternetStackHelper stackHelper;
    stackHelper.Install(nodes);
    stackHelper.Install(switches);

    NodeContainer h0s0 = NodeContainer(nodes.Get(0), switches.Get(0));
    NodeContainer h1s0 = NodeContainer(nodes.Get(1), switches.Get(0));
    // PointToPointHelper helper;
    // helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxPackets", UintegerValue(100));
    // helper.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    // helper.SetChannelAttribute("Delay", StringValue("2ms"));

    // NetDeviceContainer d_h0s0 = helper.Install(h0s0);
    // NetDeviceContainer d_h1s0 = helper.Install(h1s0);

    MtuNetHelper helper;
    // helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxPackets", UintegerValue(100));
    helper.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    helper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    helper.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d_h0s0 = helper.InstallMtuNetDevices(nodes.Get(0), switches.Get(0), 0);
    NetDeviceContainer d_h1s0 = helper.InstallMtuNetDevices(nodes.Get(1), switches.Get(0), 0);

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_h0s0 = ipv4Helper.Assign(d_h0s0);
    ipv4Helper.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_h1s0 = ipv4Helper.Assign(d_h1s0);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NodeContainer h0 = NodeContainer(nodes.Get(0));
    NodeContainer h1 = NodeContainer(nodes.Get(1));
    std::vector<Ipv4Address> address;
    address.push_back(interface_h1s0.GetAddress(0));
    uint32_t flowCount = 0;
    uint64_t bandwidth = DataRate(BANDWIDTH_LINK).GetBitRate();
    double delay_prop = double(Time(PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    double delay_process, delay_tx, delay_rx = 0;
    helper.InstallAllApplicationsInDC(h0, h1, 50, cdfTable, address, flowCount, 1000, 60000, START_TIME, END_TIME, END_TIME,
                                      bandwidth, delay_prop, delay_process, delay_tx, delay_rx);

    std::cout << flowCount << std::endl;
    // PacketSinkHelper sink{
    //     //protocol type
    //     "ns3::TcpSocketFactory",
    //     //monitor ip address
    //     InetSocketAddress(Ipv4Address::GetAny(), 2000)};
    // ApplicationContainer sinkApps = sink.Install(h1s0.Get(0));
    // sinkApps.Start(Seconds(START_TIME));
    // sinkApps.Stop(Seconds(END_TIME));

    // //send application
    // BulkSendHelper source(
    //     //protocol type
    //     "ns3::TcpSocketFactory",
    //     //ip address and port num
    //     InetSocketAddress(interface_h1s0.GetAddress(0), 2000));
    // source.SetAttribute(
    //     "SendSize",
    //     UintegerValue(8960));
    // ApplicationContainer sourceApps = source.Install(h0s0.Get(0));
    // sourceApps.Start(Seconds(START_TIME));
    // sourceApps.Stop(Seconds(END_TIME));

    // generate routing table

    Simulator::Stop(Seconds(END_TIME));
    Simulator::Run();

    NS_LOG_INFO("Finish testincast");

    return 0;
    // //generate cdf table
    // std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    // struct cdf_table *cdfTable = new cdf_table();
    // MtuUtility::init_cdf(cdfTable);
    // MtuUtility::load_cdf(cdfTable, cdfFileName.c_str());

    // //leaf spine topology
    // NodeContainer spines, leafs, ends;
    // spines.Create(4);
    // leafs.Create(4);
    // ends.Create(32);

    // //install stack
    // InternetStackHelper stackHelper;
    // stackHelper.Install(spines);
    // stackHelper.Install(leafs);
    // stackHelper.Install(ends);

    // //forwarding delay in switches, half for each netdevice
    // //1968 nanoseconds for 10G prots, 4587 nanoseconds for GE ports, 5928nanoseconds for 100M ports
    // uint32_t delay = 4587;
    // double loss_rate = 0.0;
    // MtuNetHelper netHelper;
    // // netHelper.SetQueue("ns3::MultiQueue", "MaxSize", UintegerValue(100));
    // netHelper.SetDeviceAttribute("DataRate", StringValue(DATA_RATE));
    // netHelper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    // netHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));

    // NetDeviceContainer devices_e0l0 = netHelper.InstallPointNetDevice(ends.Get(0), leafs.Get(0));
    // NetDeviceContainer devices_e1l0 = netHelper.InstallPointNetDevice(ends.Get(1), leafs.Get(0));
    // NetDeviceContainer devices_e2l0 = netHelper.InstallPointNetDevice(ends.Get(2), leafs.Get(0));
    // NetDeviceContainer devices_e3l0 = netHelper.InstallPointNetDevice(ends.Get(3), leafs.Get(0));
    // NetDeviceContainer devices_e4l0 = netHelper.InstallPointNetDevice(ends.Get(4), leafs.Get(0));
    // NetDeviceContainer devices_e5l0 = netHelper.InstallPointNetDevice(ends.Get(5), leafs.Get(0));
    // NetDeviceContainer devices_e6l0 = netHelper.InstallPointNetDevice(ends.Get(6), leafs.Get(0));
    // NetDeviceContainer devices_e7l0 = netHelper.InstallPointNetDevice(ends.Get(7), leafs.Get(0));

    // NetDeviceContainer devices_e8l1 = netHelper.InstallPointNetDevice(ends.Get(8), leafs.Get(1));
    // NetDeviceContainer devices_e9l1 = netHelper.InstallPointNetDevice(ends.Get(9), leafs.Get(1));
    // NetDeviceContainer devices_e10l1 = netHelper.InstallPointNetDevice(ends.Get(10), leafs.Get(1));
    // NetDeviceContainer devices_e11l1 = netHelper.InstallPointNetDevice(ends.Get(11), leafs.Get(1));
    // NetDeviceContainer devices_e12l1 = netHelper.InstallPointNetDevice(ends.Get(12), leafs.Get(1));
    // NetDeviceContainer devices_e13l1 = netHelper.InstallPointNetDevice(ends.Get(13), leafs.Get(1));
    // NetDeviceContainer devices_e14l1 = netHelper.InstallPointNetDevice(ends.Get(14), leafs.Get(1));
    // NetDeviceContainer devices_e15l1 = netHelper.InstallPointNetDevice(ends.Get(15), leafs.Get(1));

    // NetDeviceContainer devices_e16l2 = netHelper.InstallPointNetDevice(ends.Get(16), leafs.Get(2));
    // NetDeviceContainer devices_e17l2 = netHelper.InstallPointNetDevice(ends.Get(17), leafs.Get(2));
    // NetDeviceContainer devices_e18l2 = netHelper.InstallPointNetDevice(ends.Get(18), leafs.Get(2));
    // NetDeviceContainer devices_e19l2 = netHelper.InstallPointNetDevice(ends.Get(19), leafs.Get(2));
    // NetDeviceContainer devices_e20l2 = netHelper.InstallPointNetDevice(ends.Get(20), leafs.Get(2));
    // NetDeviceContainer devices_e21l2 = netHelper.InstallPointNetDevice(ends.Get(21), leafs.Get(2));
    // NetDeviceContainer devices_e22l2 = netHelper.InstallPointNetDevice(ends.Get(22), leafs.Get(2));
    // NetDeviceContainer devices_e23l2 = netHelper.InstallPointNetDevice(ends.Get(23), leafs.Get(2));

    // NetDeviceContainer devices_e24l3 = netHelper.InstallPointNetDevice(ends.Get(24), leafs.Get(3));
    // NetDeviceContainer devices_e25l3 = netHelper.InstallPointNetDevice(ends.Get(25), leafs.Get(3));
    // NetDeviceContainer devices_e26l3 = netHelper.InstallPointNetDevice(ends.Get(26), leafs.Get(3));
    // NetDeviceContainer devices_e27l3 = netHelper.InstallPointNetDevice(ends.Get(27), leafs.Get(3));
    // NetDeviceContainer devices_e28l3 = netHelper.InstallPointNetDevice(ends.Get(28), leafs.Get(3));
    // NetDeviceContainer devices_e29l3 = netHelper.InstallPointNetDevice(ends.Get(29), leafs.Get(3));
    // NetDeviceContainer devices_e30l3 = netHelper.InstallPointNetDevice(ends.Get(30), leafs.Get(3));
    // NetDeviceContainer devices_e31l3 = netHelper.InstallPointNetDevice(ends.Get(31), leafs.Get(3));

    // NetDeviceContainer devices_l0s0 = netHelper.InstallPointNetDevice(leafs.Get(0), spines.Get(0));
    // NetDeviceContainer devices_l0s1 = netHelper.InstallPointNetDevice(leafs.Get(0), spines.Get(1));
    // NetDeviceContainer devices_l0s2 = netHelper.InstallPointNetDevice(leafs.Get(0), spines.Get(2));
    // NetDeviceContainer devices_l0s3 = netHelper.InstallPointNetDevice(leafs.Get(0), spines.Get(3));

    // NetDeviceContainer devices_l1s0 = netHelper.InstallPointNetDevice(leafs.Get(1), spines.Get(0));
    // NetDeviceContainer devices_l1s1 = netHelper.InstallPointNetDevice(leafs.Get(1), spines.Get(1));
    // NetDeviceContainer devices_l1s2 = netHelper.InstallPointNetDevice(leafs.Get(1), spines.Get(2));
    // NetDeviceContainer devices_l1s3 = netHelper.InstallPointNetDevice(leafs.Get(1), spines.Get(3));

    // NetDeviceContainer devices_l2s0 = netHelper.InstallPointNetDevice(leafs.Get(2), spines.Get(0));
    // NetDeviceContainer devices_l2s1 = netHelper.InstallPointNetDevice(leafs.Get(2), spines.Get(1));
    // NetDeviceContainer devices_l2s2 = netHelper.InstallPointNetDevice(leafs.Get(2), spines.Get(2));
    // NetDeviceContainer devices_l2s3 = netHelper.InstallPointNetDevice(leafs.Get(2), spines.Get(3));

    // NetDeviceContainer devices_l3s0 = netHelper.InstallPointNetDevice(leafs.Get(3), spines.Get(0));
    // NetDeviceContainer devices_l3s1 = netHelper.InstallPointNetDevice(leafs.Get(3), spines.Get(1));
    // NetDeviceContainer devices_l3s2 = netHelper.InstallPointNetDevice(leafs.Get(3), spines.Get(2));
    // NetDeviceContainer devices_l3s3 = netHelper.InstallPointNetDevice(leafs.Get(3), spines.Get(3));

    // Ipv4AddressHelper ipv4Helper;
    // //assign ip for spines and leafs

    // ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l0s0 = ipv4Helper.Assign(devices_l0s0);
    // ipv4Helper.SetBase("10.1.2.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l0s1 = ipv4Helper.Assign(devices_l0s1);
    // ipv4Helper.SetBase("10.1.3.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l0s2 = ipv4Helper.Assign(devices_l0s2);
    // ipv4Helper.SetBase("10.1.4.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l0s3 = ipv4Helper.Assign(devices_l0s3);

    // ipv4Helper.SetBase("10.1.5.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l1s0 = ipv4Helper.Assign(devices_l1s0);
    // ipv4Helper.SetBase("10.1.6.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l1s1 = ipv4Helper.Assign(devices_l1s1);
    // ipv4Helper.SetBase("10.1.7.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l1s2 = ipv4Helper.Assign(devices_l1s2);
    // ipv4Helper.SetBase("10.1.8.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l1s3 = ipv4Helper.Assign(devices_l1s3);

    // ipv4Helper.SetBase("10.1.9.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l2s0 = ipv4Helper.Assign(devices_l2s0);
    // ipv4Helper.SetBase("10.1.10.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l2s1 = ipv4Helper.Assign(devices_l2s1);
    // ipv4Helper.SetBase("10.1.11.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l2s2 = ipv4Helper.Assign(devices_l2s2);
    // ipv4Helper.SetBase("10.1.12.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l2s3 = ipv4Helper.Assign(devices_l2s3);

    // ipv4Helper.SetBase("10.1.13.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l3s0 = ipv4Helper.Assign(devices_l3s0);
    // ipv4Helper.SetBase("10.1.14.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l3s1 = ipv4Helper.Assign(devices_l3s1);
    // ipv4Helper.SetBase("10.1.15.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l3s2 = ipv4Helper.Assign(devices_l3s2);
    // ipv4Helper.SetBase("10.1.16.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l3s3 = ipv4Helper.Assign(devices_l3s3);

    // NetDeviceContainer devices_l0;
    // devices_l0.Add(devices_e0l0);
    // devices_l0.Add(devices_e1l0);
    // devices_l0.Add(devices_e2l0);
    // devices_l0.Add(devices_e3l0);
    // devices_l0.Add(devices_e4l0);
    // devices_l0.Add(devices_e5l0);
    // devices_l0.Add(devices_e6l0);
    // devices_l0.Add(devices_e7l0);
    // ipv4Helper.SetBase("10.2.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l0 = ipv4Helper.Assign(devices_l0);

    // NetDeviceContainer devices_l1;
    // devices_l1.Add(devices_e8l1);
    // devices_l1.Add(devices_e9l1);
    // devices_l1.Add(devices_e10l1);
    // devices_l1.Add(devices_e11l1);
    // devices_l1.Add(devices_e12l1);
    // devices_l1.Add(devices_e13l1);
    // devices_l1.Add(devices_e14l1);
    // devices_l1.Add(devices_e15l1);
    // ipv4Helper.SetBase("10.2.2.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l1 = ipv4Helper.Assign(devices_l1);

    // NetDeviceContainer devices_l2;
    // devices_l2.Add(devices_e16l2);
    // devices_l2.Add(devices_e17l2);
    // devices_l2.Add(devices_e18l2);
    // devices_l2.Add(devices_e19l2);
    // devices_l2.Add(devices_e20l2);
    // devices_l2.Add(devices_e21l2);
    // devices_l2.Add(devices_e22l2);
    // devices_l2.Add(devices_e23l2);
    // ipv4Helper.SetBase("10.2.3.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l2 = ipv4Helper.Assign(devices_l2);

    // NetDeviceContainer devices_l3;
    // devices_l3.Add(devices_e24l3);
    // devices_l3.Add(devices_e25l3);
    // devices_l3.Add(devices_e26l3);
    // devices_l3.Add(devices_e27l3);
    // devices_l3.Add(devices_e28l3);
    // devices_l3.Add(devices_e29l3);
    // devices_l3.Add(devices_e30l3);
    // devices_l3.Add(devices_e31l3);
    // ipv4Helper.SetBase("10.2.4.0", "255.255.255.0");
    // Ipv4InterfaceContainer interface_l3 = ipv4Helper.Assign(devices_l3);

    // double request_rate = LOAD * DataRate(BANDWIDTH_LINK).GetBitRate() / (8 * MtuUtility::avg_cdf(cdfTable));
    // Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // std::vector<Ipv4Address> dstAddress;
    // for (uint32_t i = 0; i < interface_l0.GetN(); i += 2)
    // {
    //     dstAddress.push_back(interface_l0.GetAddress(i));
    // }
    // for (uint32_t i = 0; i < interface_l1.GetN(); i += 2)
    // {
    //     dstAddress.push_back(interface_l1.GetAddress(i));
    // }
    // for (uint32_t i = 0; i < interface_l2.GetN(); i += 2)
    // {
    //     dstAddress.push_back(interface_l2.GetAddress(i));
    // }
    // for (uint32_t i = 0; i < interface_l3.GetN(); i += 2)
    // {
    //     dstAddress.push_back(interface_l3.GetAddress(i));
    // }

    // uint32_t flowCount = 0;
    // uint64_t bandwidth = DataRate(BANDWIDTH_LINK).GetBitRate();
    // double delay_prop = double(Time(PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    // double delay_process, delay_tx, delay_rx = 0;

    // netHelper.InstallAllApplicationsInDC(ends, ends, request_rate, cdfTable, dstAddress, flowCount, PORT_START, PORT_END, START_TIME, END_TIME, END_TIME,
    //                                      bandwidth, delay_prop, delay_process, delay_tx, delay_rx);
    // //     netHelper.InstallAllApplications(ends, ends, request_rate, cdfTable,
    // //                                      dstAddress, flowCount, PORT_START, PORT_END, MSS, START_TIME, END_TIME, END_TIME, bandwidth, delay_prop);
    // std::cout << "flow count is" << flowCount << std::endl;

    // Ptr<FlowMonitor> flowMonitor;
    // FlowMonitorHelper flowHelper;
    // flowMonitor = flowHelper.InstallAll();
    // Simulator::Stop(Seconds(END_TIME));
    // Simulator::Run();

    // flowMonitor->CheckForLostPackets();
    // flowMonitor->SerializeToXmlFile("data/leaf-spine/multiqueue.xml", true, true);

    // Simulator::Destroy();
    // MtuUtility::free_cdf(cdfTable);

    return 0;
}
