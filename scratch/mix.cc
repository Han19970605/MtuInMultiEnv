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
#define END_TIME 60

// 128M
#define BUFFER_SIZE 134217728
#define MTU 9000
#define MSS MTU - 40

#define TCP_PROTOCOL "ns3::TcpNewReno"
#define NUM_WAN_SWITCHES 4

#define PORT_START 1000
#define PORT_END 60000

// #define PROPOGATION_DELAY "100us"
// #define WAN_PROPOGATION_DELAY "2ms"
// #define BANDWIDTH_LINK "1Gbps"
// #define ES_BANDWIDTH "1Gbps"
// #define SS_BANDWIDTH "1Gbps"
// #define LOAD 0.8
// #define LOSS_RATE 0.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Mix");

/**
 * \brief in the current simulation, there are 24 srvers in DC and 8 servers in WAN, the spine switches
 *        are all connected to the first WAN switch(s0), there are four WAN switches in the topology
 * 
 *        referenced to ANNULUS, the traffic in dc and WAN are 5:1, the from servers are the servers in DC and the dst servers are 
*/

int main(int argc, char *argv[])
{
    LogComponentEnable("Mix", LOG_INFO);

    // cmd 参数
    std::string PROPOGATION_DELAY = "100us";
    std::string WAN_PROPOGATION_DELAY = "2ms";
    std::string BANDWIDTH_LINK = "1Gbps";
    std::string ES_BANDWIDTH = "1Gbps";
    std::string SS_BANDWIDTH = "1Gbps";
    double LOAD = 0.8;
    double LOSS_RATE = 0.0;

    CommandLine cmd;
    cmd.AddValue("DC_DELAY", "数据中心链路延迟", PROPOGATION_DELAY);
    cmd.AddValue("WAN_DELAY", "广域网时延", WAN_PROPOGATION_DELAY);
    cmd.AddValue("LOSS_RATE", "丢包率", LOSS_RATE);
    cmd.AddValue("ES_BANDWIDTH", "端到交换机的链路带宽", ES_BANDWIDTH);
    cmd.AddValue("SS_BANDWIDTH", "交换机之间的链路带宽", SS_BANDWIDTH);
    cmd.AddValue("BANDWIDTH_LINK", "数据中心的链路带宽", BANDWIDTH_LINK);
    cmd.AddValue("LOAD", "链路的负载状况", LOAD);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(MSS));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(TCP_PROTOCOL));
    Config::SetDefault("ns3::PointToPointNetDevice::Mtu", UintegerValue(MTU));

    uint64_t rtt = Time(PROPOGATION_DELAY).GetNanoSeconds() * 4 + Time(WAN_PROPOGATION_DELAY).GetNanoSeconds() * 8;
    double rto = (Time(PROPOGATION_DELAY).GetSeconds() * 2 + Time(WAN_PROPOGATION_DELAY).GetSeconds() * 4) * 10;
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(rto)));

    //generate cdf table
    std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    struct cdf_table *cdfTable = new cdf_table();
    MtuUtility::init_cdf(cdfTable);
    MtuUtility::load_cdf(cdfTable, cdfFileName.c_str());

    // filename
    std::string FCT_fileName = std::string("./data/FCT(mix)_").append(PROPOGATION_DELAY).append(WAN_PROPOGATION_DELAY).append(std::string("_")).append(ES_BANDWIDTH).append(std::string("_"));
    FCT_fileName = FCT_fileName.append(std::string("_")).append(BANDWIDTH_LINK).append(std::string("_")).append(std::to_string(LOSS_RATE)).append(std::string("_")).append(std::to_string(LOAD)).append(std::string(".csv"));

    //Datacenter topology
    NodeContainer spines, leafs, ends;
    spines.Create(4);
    leafs.Create(4);
    ends.Create(32);

    //wan topology
    NodeContainer ends_wan, switches_wan;
    ends_wan.Create(8);
    switches_wan.Create(NUM_WAN_SWITCHES);

    //install stack
    InternetStackHelper stackHelper;
    stackHelper.Install(spines);
    stackHelper.Install(leafs);
    stackHelper.Install(ends);
    stackHelper.Install(ends_wan);
    stackHelper.Install(switches_wan);

    //forwarding delay in switches, half for each netdevice
    //1968 nanoseconds for 10G prots, 4587 nanoseconds for GE ports, 5928nanoseconds for 100M ports
    // netHelper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    uint32_t delay = 4587;
    MtuNetHelper netHelper;
    netHelper.data_fileName = FCT_fileName;
    netHelper.rtt = rtt;

    netHelper.SetDeviceAttribute("DataRate", StringValue(BANDWIDTH_LINK));
    netHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));

    NetDeviceContainer devices_e0l0 = netHelper.InstallNormalNetDevices(ends.Get(0), leafs.Get(0));
    NetDeviceContainer devices_e1l0 = netHelper.InstallNormalNetDevices(ends.Get(1), leafs.Get(0));
    NetDeviceContainer devices_e2l0 = netHelper.InstallNormalNetDevices(ends.Get(2), leafs.Get(0));
    NetDeviceContainer devices_e3l0 = netHelper.InstallNormalNetDevices(ends.Get(3), leafs.Get(0));
    NetDeviceContainer devices_e4l0 = netHelper.InstallNormalNetDevices(ends.Get(4), leafs.Get(0));
    NetDeviceContainer devices_e5l0 = netHelper.InstallNormalNetDevices(ends.Get(5), leafs.Get(0));
    NetDeviceContainer devices_e6l0 = netHelper.InstallNormalNetDevices(ends.Get(6), leafs.Get(0));
    NetDeviceContainer devices_e7l0 = netHelper.InstallNormalNetDevices(ends.Get(7), leafs.Get(0));

    NetDeviceContainer devices_e8l1 = netHelper.InstallNormalNetDevices(ends.Get(8), leafs.Get(1));
    NetDeviceContainer devices_e9l1 = netHelper.InstallNormalNetDevices(ends.Get(9), leafs.Get(1));
    NetDeviceContainer devices_e10l1 = netHelper.InstallNormalNetDevices(ends.Get(10), leafs.Get(1));
    NetDeviceContainer devices_e11l1 = netHelper.InstallNormalNetDevices(ends.Get(11), leafs.Get(1));
    NetDeviceContainer devices_e12l1 = netHelper.InstallNormalNetDevices(ends.Get(12), leafs.Get(1));
    NetDeviceContainer devices_e13l1 = netHelper.InstallNormalNetDevices(ends.Get(13), leafs.Get(1));
    NetDeviceContainer devices_e14l1 = netHelper.InstallNormalNetDevices(ends.Get(14), leafs.Get(1));
    NetDeviceContainer devices_e15l1 = netHelper.InstallNormalNetDevices(ends.Get(15), leafs.Get(1));

    NetDeviceContainer devices_e16l2 = netHelper.InstallNormalNetDevices(ends.Get(16), leafs.Get(2));
    NetDeviceContainer devices_e17l2 = netHelper.InstallNormalNetDevices(ends.Get(17), leafs.Get(2));
    NetDeviceContainer devices_e18l2 = netHelper.InstallNormalNetDevices(ends.Get(18), leafs.Get(2));
    NetDeviceContainer devices_e19l2 = netHelper.InstallNormalNetDevices(ends.Get(19), leafs.Get(2));
    NetDeviceContainer devices_e20l2 = netHelper.InstallNormalNetDevices(ends.Get(20), leafs.Get(2));
    NetDeviceContainer devices_e21l2 = netHelper.InstallNormalNetDevices(ends.Get(21), leafs.Get(2));
    NetDeviceContainer devices_e22l2 = netHelper.InstallNormalNetDevices(ends.Get(22), leafs.Get(2));
    NetDeviceContainer devices_e23l2 = netHelper.InstallNormalNetDevices(ends.Get(23), leafs.Get(2));

    NetDeviceContainer devices_e24l3 = netHelper.InstallNormalNetDevices(ends.Get(24), leafs.Get(3));
    NetDeviceContainer devices_e25l3 = netHelper.InstallNormalNetDevices(ends.Get(25), leafs.Get(3));
    NetDeviceContainer devices_e26l3 = netHelper.InstallNormalNetDevices(ends.Get(26), leafs.Get(3));
    NetDeviceContainer devices_e27l3 = netHelper.InstallNormalNetDevices(ends.Get(27), leafs.Get(3));
    NetDeviceContainer devices_e28l3 = netHelper.InstallNormalNetDevices(ends.Get(28), leafs.Get(3));
    NetDeviceContainer devices_e29l3 = netHelper.InstallNormalNetDevices(ends.Get(29), leafs.Get(3));
    NetDeviceContainer devices_e30l3 = netHelper.InstallNormalNetDevices(ends.Get(30), leafs.Get(3));
    NetDeviceContainer devices_e31l3 = netHelper.InstallNormalNetDevices(ends.Get(31), leafs.Get(3));

    NetDeviceContainer devices_l0s0 = netHelper.InstallMtuNetDevices(leafs.Get(0), spines.Get(0), LOSS_RATE);
    NetDeviceContainer devices_l0s1 = netHelper.InstallMtuNetDevices(leafs.Get(0), spines.Get(1), LOSS_RATE);
    NetDeviceContainer devices_l0s2 = netHelper.InstallMtuNetDevices(leafs.Get(0), spines.Get(2), LOSS_RATE);
    NetDeviceContainer devices_l0s3 = netHelper.InstallMtuNetDevices(leafs.Get(0), spines.Get(3), LOSS_RATE);

    NetDeviceContainer devices_l1s0 = netHelper.InstallMtuNetDevices(leafs.Get(1), spines.Get(0), LOSS_RATE);
    NetDeviceContainer devices_l1s1 = netHelper.InstallMtuNetDevices(leafs.Get(1), spines.Get(1), LOSS_RATE);
    NetDeviceContainer devices_l1s2 = netHelper.InstallMtuNetDevices(leafs.Get(1), spines.Get(2), LOSS_RATE);
    NetDeviceContainer devices_l1s3 = netHelper.InstallMtuNetDevices(leafs.Get(1), spines.Get(3), LOSS_RATE);

    NetDeviceContainer devices_l2s0 = netHelper.InstallMtuNetDevices(leafs.Get(2), spines.Get(0), LOSS_RATE);
    NetDeviceContainer devices_l2s1 = netHelper.InstallMtuNetDevices(leafs.Get(2), spines.Get(1), LOSS_RATE);
    NetDeviceContainer devices_l2s2 = netHelper.InstallMtuNetDevices(leafs.Get(2), spines.Get(2), LOSS_RATE);
    NetDeviceContainer devices_l2s3 = netHelper.InstallMtuNetDevices(leafs.Get(2), spines.Get(3), LOSS_RATE);

    NetDeviceContainer devices_l3s0 = netHelper.InstallMtuNetDevices(leafs.Get(3), spines.Get(0), LOSS_RATE);
    NetDeviceContainer devices_l3s1 = netHelper.InstallMtuNetDevices(leafs.Get(3), spines.Get(1), LOSS_RATE);
    NetDeviceContainer devices_l3s2 = netHelper.InstallMtuNetDevices(leafs.Get(3), spines.Get(2), LOSS_RATE);
    NetDeviceContainer devices_l3s3 = netHelper.InstallMtuNetDevices(leafs.Get(3), spines.Get(3), LOSS_RATE);

    //nethelper in wan es:end-switch ss switch-switch
    MtuNetHelper ESHelper;
    ESHelper.SetDeviceAttribute("DataRate", StringValue(ES_BANDWIDTH));
    ESHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));
    ESHelper.data_fileName = FCT_fileName;
    ESHelper.rtt = rtt;

    NetDeviceContainer devices_s3e0 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(0));
    NetDeviceContainer devices_s3e1 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(1));
    NetDeviceContainer devices_s3e2 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(2));
    NetDeviceContainer devices_s3e3 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(3));
    NetDeviceContainer devices_s3e4 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(4));
    NetDeviceContainer devices_s3e5 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(5));
    NetDeviceContainer devices_s3e6 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(6));
    NetDeviceContainer devices_s3e7 = ESHelper.InstallNormalNetDevices(switches_wan.Get(3), ends_wan.Get(7));

    MtuNetHelper SSHelper;
    SSHelper.SetDeviceAttribute("DataRate", StringValue(ES_BANDWIDTH));
    SSHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));
    SSHelper.data_fileName = FCT_fileName;

    NetDeviceContainer devices_s2s3 = SSHelper.InstallMtuNetDevices(switches_wan.Get(2), switches_wan.Get(3), LOSS_RATE);
    NetDeviceContainer devices_s1s2 = SSHelper.InstallNormalNetDevices(switches_wan.Get(2), switches_wan.Get(3));
    NetDeviceContainer devices_s0s1 = SSHelper.InstallNormalNetDevices(switches_wan.Get(2), switches_wan.Get(3));

    NetDeviceContainer devices_s0s0 = SSHelper.InstallNormalNetDevices(spines.Get(0), switches_wan.Get(0));
    NetDeviceContainer devices_s1s0 = SSHelper.InstallNormalNetDevices(spines.Get(1), switches_wan.Get(0));
    NetDeviceContainer devices_s2s0 = SSHelper.InstallNormalNetDevices(spines.Get(2), switches_wan.Get(0));
    NetDeviceContainer devices_s3s0 = SSHelper.InstallNormalNetDevices(spines.Get(3), switches_wan.Get(0));

    Ipv4AddressHelper ipv4Helper;
    //assign ip for spines and leafs

    ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l0s0 = ipv4Helper.Assign(devices_l0s0);
    ipv4Helper.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l0s1 = ipv4Helper.Assign(devices_l0s1);
    ipv4Helper.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l0s2 = ipv4Helper.Assign(devices_l0s2);
    ipv4Helper.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l0s3 = ipv4Helper.Assign(devices_l0s3);

    ipv4Helper.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l1s0 = ipv4Helper.Assign(devices_l1s0);
    ipv4Helper.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l1s1 = ipv4Helper.Assign(devices_l1s1);
    ipv4Helper.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l1s2 = ipv4Helper.Assign(devices_l1s2);
    ipv4Helper.SetBase("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l1s3 = ipv4Helper.Assign(devices_l1s3);

    ipv4Helper.SetBase("10.1.9.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l2s0 = ipv4Helper.Assign(devices_l2s0);
    ipv4Helper.SetBase("10.1.10.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l2s1 = ipv4Helper.Assign(devices_l2s1);
    ipv4Helper.SetBase("10.1.11.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l2s2 = ipv4Helper.Assign(devices_l2s2);
    ipv4Helper.SetBase("10.1.12.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l2s3 = ipv4Helper.Assign(devices_l2s3);

    ipv4Helper.SetBase("10.1.13.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l3s0 = ipv4Helper.Assign(devices_l3s0);
    ipv4Helper.SetBase("10.1.14.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l3s1 = ipv4Helper.Assign(devices_l3s1);
    ipv4Helper.SetBase("10.1.15.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l3s2 = ipv4Helper.Assign(devices_l3s2);
    ipv4Helper.SetBase("10.1.16.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l3s3 = ipv4Helper.Assign(devices_l3s3);

    NetDeviceContainer devices_l0;
    devices_l0.Add(devices_e0l0);
    devices_l0.Add(devices_e1l0);
    devices_l0.Add(devices_e2l0);
    devices_l0.Add(devices_e3l0);
    devices_l0.Add(devices_e4l0);
    devices_l0.Add(devices_e5l0);
    devices_l0.Add(devices_e6l0);
    devices_l0.Add(devices_e7l0);
    ipv4Helper.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l0 = ipv4Helper.Assign(devices_l0);

    NetDeviceContainer devices_l1;
    devices_l1.Add(devices_e8l1);
    devices_l1.Add(devices_e9l1);
    devices_l1.Add(devices_e10l1);
    devices_l1.Add(devices_e11l1);
    devices_l1.Add(devices_e12l1);
    devices_l1.Add(devices_e13l1);
    devices_l1.Add(devices_e14l1);
    devices_l1.Add(devices_e15l1);
    ipv4Helper.SetBase("10.2.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l1 = ipv4Helper.Assign(devices_l1);

    NetDeviceContainer devices_l2;
    devices_l2.Add(devices_e16l2);
    devices_l2.Add(devices_e17l2);
    devices_l2.Add(devices_e18l2);
    devices_l2.Add(devices_e19l2);
    devices_l2.Add(devices_e20l2);
    devices_l2.Add(devices_e21l2);
    devices_l2.Add(devices_e22l2);
    devices_l2.Add(devices_e23l2);
    ipv4Helper.SetBase("10.2.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l2 = ipv4Helper.Assign(devices_l2);

    NetDeviceContainer devices_l3;
    devices_l3.Add(devices_e24l3);
    devices_l3.Add(devices_e25l3);
    devices_l3.Add(devices_e26l3);
    devices_l3.Add(devices_e27l3);
    devices_l3.Add(devices_e28l3);
    devices_l3.Add(devices_e29l3);
    devices_l3.Add(devices_e30l3);
    devices_l3.Add(devices_e31l3);
    ipv4Helper.SetBase("10.2.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_l3 = ipv4Helper.Assign(devices_l3);

    // assign ip for WAN
    ipv4Helper.SetBase("10.3.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s0s0 = ipv4Helper.Assign(devices_s0s0);
    Ipv4InterfaceContainer interface_s1s0 = ipv4Helper.Assign(devices_s1s0);
    Ipv4InterfaceContainer interface_s2s0 = ipv4Helper.Assign(devices_s2s0);
    Ipv4InterfaceContainer interface_s3s0 = ipv4Helper.Assign(devices_s3s0);
    ipv4Helper.SetBase("10.3.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s0s1 = ipv4Helper.Assign(devices_s0s1);
    ipv4Helper.SetBase("10.3.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s1s2 = ipv4Helper.Assign(devices_s1s2);
    ipv4Helper.SetBase("10.3.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s2s3 = ipv4Helper.Assign(devices_s2s3);
    ipv4Helper.SetBase("10.3.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s3e0 = ipv4Helper.Assign(devices_s3e0);
    Ipv4InterfaceContainer interface_s3e1 = ipv4Helper.Assign(devices_s3e1);
    Ipv4InterfaceContainer interface_s3e2 = ipv4Helper.Assign(devices_s3e2);
    Ipv4InterfaceContainer interface_s3e3 = ipv4Helper.Assign(devices_s3e3);
    Ipv4InterfaceContainer interface_s3e4 = ipv4Helper.Assign(devices_s3e4);
    Ipv4InterfaceContainer interface_s3e5 = ipv4Helper.Assign(devices_s3e5);
    Ipv4InterfaceContainer interface_s3e6 = ipv4Helper.Assign(devices_s3e6);
    Ipv4InterfaceContainer interface_s3e7 = ipv4Helper.Assign(devices_s3e7);

    double request_rate = LOAD * DataRate(BANDWIDTH_LINK).GetBitRate() / (8 * MtuUtility::avg_cdf(cdfTable));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // all the dst
    // datacenter dst address
    std::vector<Ipv4Address> dstAddress;
    for (uint32_t i = 0; i < interface_l0.GetN(); i += 2)
    {
        dstAddress.push_back(interface_l0.GetAddress(i));
    }
    for (uint32_t i = 0; i < interface_l1.GetN(); i += 2)
    {
        dstAddress.push_back(interface_l1.GetAddress(i));
    }
    for (uint32_t i = 0; i < interface_l2.GetN(); i += 2)
    {
        dstAddress.push_back(interface_l2.GetAddress(i));
    }
    for (uint32_t i = 0; i < interface_l3.GetN(); i += 2)
    {
        dstAddress.push_back(interface_l3.GetAddress(i));
    }

    // WAN dst address
    dstAddress.push_back(interface_s3e0.GetAddress(1));
    dstAddress.push_back(interface_s3e1.GetAddress(1));
    dstAddress.push_back(interface_s3e2.GetAddress(1));
    dstAddress.push_back(interface_s3e3.GetAddress(1));
    dstAddress.push_back(interface_s3e4.GetAddress(1));
    dstAddress.push_back(interface_s3e5.GetAddress(1));
    dstAddress.push_back(interface_s3e6.GetAddress(1));
    dstAddress.push_back(interface_s3e7.GetAddress(1));

    // data in DC and out of DC 5:1
    NodeContainer dstNodes;
    dstNodes.Add(ends);
    dstNodes.Add(ends_wan);

    uint32_t flowCount = 0;
    uint64_t bandwidth = DataRate(BANDWIDTH_LINK).GetBitRate();
    uint64_t bandwidth_wan = DataRate(ES_BANDWIDTH).GetBitRate();
    double delay_prop = double(Time(PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    double delay_prop_wan = double(Time(WAN_PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    double delay_process, delay_tx, delay_rx = 0;

    netHelper.InstallAllApplicationsInMix(ends, dstNodes, request_rate, cdfTable, dstAddress, flowCount, PORT_START, PORT_END, START_TIME, END_TIME, END_TIME, bandwidth,
                                          bandwidth_wan, delay_prop, delay_prop_wan, delay_process, delay_process,
                                          switches_wan.GetN(), delay_tx, delay_rx, "dc");

    Simulator::Stop(Seconds(END_TIME));
    Simulator::Run();

    Simulator::Destroy();
    MtuUtility::free_cdf(cdfTable);

    return 0;
}