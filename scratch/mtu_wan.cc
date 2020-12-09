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
#define END_TIME 1000

// 128M
#define BUFFER_SIZE 134217728
#define MTU 9000
#define MSS MTU - 40
#define TCP_PROTOCOL "ns3::TcpNewReno"

#define PORT_START 1000
#define PORT_END 65535

// #define LOSS_RATE 0.0
// #define PROPOGATION_DELAY "100us"
// #define ES_BANDWIDTH "1Gbps"
// #define SS_BANDWIDTH "1Gbps"
// #define LOAD 0.8

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Wan");

int main(int argc, char *argv[])
{
    LogComponentEnable("Wan", LOG_INFO);

    // 脚本传输的参数
    // propogation delay refers to per hop
    std::string PROPOGATION_DELAY = "2ms";
    std::string ES_BANDWIDTH = "1Gbps";
    std::string SS_BANDWIDTH = "1Gbps";
    double LOSS_RATE = 0.0;
    double delay_tx = 0.0;
    double delay_rx = 0.0;
    double LOAD = 0.8;

    CommandLine cmd;
    cmd.AddValue("PROPOGATION_DELAY", "延迟", PROPOGATION_DELAY);
    cmd.AddValue("LOSS_RATE", "丢包率", LOSS_RATE);
    cmd.AddValue("ES_BANDWIDTH", "端到交换机的链路带宽", ES_BANDWIDTH);
    cmd.AddValue("SS_BANDWIDTH", "交换机之间的链路带宽", SS_BANDWIDTH);
    cmd.AddValue("LOAD", "链路的负载状况", LOAD);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(MSS));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(TCP_PROTOCOL));
    Config::SetDefault("ns3::PointToPointNetDevice::Mtu", UintegerValue(MTU));
    // Set the default rto
    uint64_t rtt = Time(PROPOGATION_DELAY).GetNanoSeconds() * 8;
    double rto = Time(PROPOGATION_DELAY).GetSeconds() * 8 * 5;
    std::cout << Time(PROPOGATION_DELAY).GetSeconds() << " " << rto << std::endl;
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(rto)));

    // generate cdf table
    std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    struct cdf_table *cdfTable = new cdf_table();
    MtuUtility::init_cdf(cdfTable);
    MtuUtility::load_cdf(cdfTable, cdfFileName.c_str());

    // fct filename
    std::string FCT_fileName = std::string("FCT(wan)_").append(PROPOGATION_DELAY).append(std::string("_")).append(ES_BANDWIDTH).append(std::string("_"));
    FCT_fileName = FCT_fileName.append(std::to_string(LOSS_RATE)).append(std::string("_")).append(std::to_string(LOAD));

    NodeContainer from_servers, dst_servers, switches;
    from_servers.Create(4);
    switches.Create(3);
    dst_servers.Create(4);

    InternetStackHelper stackHelper;
    stackHelper.Install(from_servers);
    stackHelper.Install(dst_servers);
    stackHelper.Install(switches);

    MtuNetHelper ESNetHelper;
    ESNetHelper.SetDeviceAttribute("DataRate", StringValue(ES_BANDWIDTH));
    ESNetHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));
    ESNetHelper.data_fileName = FCT_fileName;
    ESNetHelper.rtt = rtt;

    MtuNetHelper SSNetHelper;
    SSNetHelper.SetDeviceAttribute("DataRate", StringValue(ES_BANDWIDTH));
    SSNetHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));
    SSNetHelper.data_fileName = FCT_fileName;
    SSNetHelper.rtt = rtt;

    NetDeviceContainer devices_e0s0 = ESNetHelper.InstallNormalNetDevices(from_servers.Get(0), switches.Get(0));
    NetDeviceContainer devices_e1s0 = ESNetHelper.InstallNormalNetDevices(from_servers.Get(1), switches.Get(0));
    NetDeviceContainer devices_e2s0 = ESNetHelper.InstallNormalNetDevices(from_servers.Get(2), switches.Get(0));
    NetDeviceContainer devices_e3s0 = ESNetHelper.InstallNormalNetDevices(from_servers.Get(3), switches.Get(0));

    NetDeviceContainer devices_s0s1 = SSNetHelper.InstallNormalNetDevices(switches.Get(0), switches.Get(1));
    NetDeviceContainer devices_s1s2 = SSNetHelper.InstallMtuNetDevices(switches.Get(1), switches.Get(2), LOSS_RATE);

    NetDeviceContainer devices_s2e4 = ESNetHelper.InstallNormalNetDevices(switches.Get(2), dst_servers.Get(0));
    NetDeviceContainer devices_s2e5 = ESNetHelper.InstallNormalNetDevices(switches.Get(2), dst_servers.Get(1));
    NetDeviceContainer devices_s2e6 = ESNetHelper.InstallNormalNetDevices(switches.Get(2), dst_servers.Get(2));
    NetDeviceContainer devices_s2e7 = ESNetHelper.InstallNormalNetDevices(switches.Get(2), dst_servers.Get(3));

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer e0s0 = ipv4Helper.Assign(devices_e0s0);
    ipv4Helper.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer e1s0 = ipv4Helper.Assign(devices_e1s0);
    ipv4Helper.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer e2s0 = ipv4Helper.Assign(devices_e2s0);
    ipv4Helper.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer e3s0 = ipv4Helper.Assign(devices_e3s0);
    ipv4Helper.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s0s1 = ipv4Helper.Assign(devices_s0s1);
    ipv4Helper.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_s1s2 = ipv4Helper.Assign(devices_s1s2);
    ipv4Helper.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer s2e4 = ipv4Helper.Assign(devices_s2e4);
    ipv4Helper.SetBase("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer s2e5 = ipv4Helper.Assign(devices_s2e5);
    ipv4Helper.SetBase("10.1.9.0", "255.255.255.0");
    Ipv4InterfaceContainer s2e6 = ipv4Helper.Assign(devices_s2e6);
    ipv4Helper.SetBase("10.1.10.0", "255.255.255.0");
    Ipv4InterfaceContainer s2e7 = ipv4Helper.Assign(devices_s2e7);
    // NetDeviceContainer dst;
    // dst.Add(devices_s2e4);
    // dst.Add(devices_s2e5);
    // dst.Add(devices_s2e6);
    // dst.Add(devices_s2e7);
    // Ipv4InterfaceContainer dst_add = ipv4Helper.Assign(dst);

    double request_rate = LOAD * DataRate(ES_BANDWIDTH).GetBitRate() / (8 * MtuUtility::avg_cdf(cdfTable));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::vector<Ipv4Address> dstAddress;
    dstAddress.push_back(s2e4.GetAddress(1));
    dstAddress.push_back(s2e5.GetAddress(1));
    dstAddress.push_back(s2e6.GetAddress(1));
    dstAddress.push_back(s2e7.GetAddress(1));

    uint32_t flowCount = 0;
    uint64_t bandwidth = DataRate(ES_BANDWIDTH).GetBitRate();
    double delay_prop = double(Time(PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    double delay_process = 0;
    double end_gen_time = 64535.0 / request_rate / 32;

    ESNetHelper.InstallAllApplicationsInWAN(from_servers, dst_servers, request_rate, cdfTable, dstAddress,
                                            flowCount, PORT_START, PORT_END, START_TIME, END_TIME, end_gen_time, bandwidth,
                                            switches.GetN(), delay_prop, delay_process, delay_tx, delay_rx);

    std::cout << flowCount << std::endl;
    Simulator::Stop(Seconds(END_TIME));
    Simulator::Run();

    Simulator::Destroy();
    MtuUtility::free_cdf(cdfTable);

    return 0;
}