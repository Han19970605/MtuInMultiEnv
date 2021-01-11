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
#include <fstream>

#define START_TIME 0.0

// 128M
#define BUFFER_SIZE 134217728
#define MTU 9000
#define MSS MTU - 40
// #define TCP_PROTOCOL "ns3::TcpNewReno"

#define PORT_START 1000
#define PORT_END 49152

double END_TIME = 10;
extern int adjust_interval;
extern std::string BANDWIDTH_LINK;
extern uint64_t bandwidth;
extern std::string TCP_PROTOCOL;
extern double LOAD;
extern double LOSS_RATE;
extern std::string PROPOGATION_DELAY;
extern int numOfSwitches;
extern uint32_t mode;
// extern std::map<int, int> netdeviceQ_length;

struct flow
{
    double startTime;
    uint64_t flowSize;
};
extern std::vector<flow> flowInfo;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DataCenter");

int main(int argc, char *argv[])
{
    LogComponentEnable("DataCenter", LOG_INFO);

    // cmd传参 全局变量用于ertern
    // std::string PROPOGATION_DELAY = "10us";
    // double LOSS_RATE = 0;
    // double LOAD = 0.3;

    CommandLine cmd;
    cmd.AddValue("DELAY", "延迟", PROPOGATION_DELAY);
    cmd.AddValue("LOSS_RATE", "丢包率", LOSS_RATE);
    cmd.AddValue("BANDWIDTH_LINK", "数据中心链路带宽", BANDWIDTH_LINK);
    cmd.AddValue("LOAD", "链路的负载状况", LOAD);
    cmd.AddValue("MODE", "是否开启优先级队列和变化", mode);
    // cmd.AddValue("END_TIME", "结束模拟的时间", END_TIME);
    cmd.Parse(argc, argv);

    // 参数配置
    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(BUFFER_SIZE));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(MSS));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(TCP_PROTOCOL));
    Config::SetDefault("ns3::PointToPointNetDevice::Mtu", UintegerValue(MTU));

    // set the rto
    double rto = Time(PROPOGATION_DELAY).GetSeconds() * 8 * 10;
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(rto)));

    //generate cdf table
    std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    struct cdf_table *cdfTable = new cdf_table();
    MtuUtility::init_cdf(cdfTable);
    MtuUtility::load_cdf(cdfTable, cdfFileName.c_str());

    //read the flow infomation
    std::string flowInfo_file = "./genFlow/dcgen_1wflow_";
    flowInfo_file.append(BANDWIDTH_LINK);
    flowInfo_file.append("_");
    flowInfo_file.append(std::to_string(LOAD));
    flowInfo_file.append(".csv");
    std::ifstream flowCsv(flowInfo_file, std::ios::in);
    std::string line;
    // std::cout << flowInfo << std::endl;
    while (getline(flowCsv, line))
    {
        std::stringstream ss(line);
        flow f;
        std::string value;
        std::vector<std::string> info;
        while (getline(ss, value, ','))
        {
            info.push_back(value);
        }
        f.startTime = atof(info[0].c_str());
        f.flowSize = atoi(info[1].c_str());
        flowInfo.push_back(f);
    }

    //leaf spine topology
    NodeContainer spines,
        leafs, ends;
    spines.Create(4);
    leafs.Create(4);
    ends.Create(32);

    /**
     * add the id of each node
     * 0-31 for endhost 32-35 for spines 36-39 for leafs
     */
    uint32_t k = 0;
    for (uint32_t i = 0; i < ends.GetN(); i++)
    {
        ends.Get(i)->SetAttribute("Id", UintegerValue(k + i));
    }

    k += ends.GetN();
    for (uint32_t i = 0; i < spines.GetN(); i++)
    {
        spines.Get(i)->SetAttribute("Id", UintegerValue(k + i));
    }
    k += spines.GetN();
    for (uint32_t i = 0; i < leafs.GetN(); i++)
    {
        leafs.Get(i)->SetAttribute("Id", UintegerValue(k + i));
    }

    //install stack
    InternetStackHelper stackHelper;
    stackHelper.Install(spines);
    stackHelper.Install(leafs);
    stackHelper.Install(ends);

    MtuNetHelper netHelper;

    // FCT file name
    std::string FCT_fileName = std::string("FCT(dc)_").append(PROPOGATION_DELAY).append(std::string("_")).append(BANDWIDTH_LINK).append(std::string("_"));
    FCT_fileName = FCT_fileName.append(std::to_string(LOSS_RATE)).append(std::string("_")).append(std::to_string(LOAD)).append(std::string("_")).append(std::to_string(mode));
    std::cout << mode << " " << FCT_fileName << std::endl;

    uint64_t rtt = Time(PROPOGATION_DELAY).GetNanoSeconds() * 8 + (double(1500 * 8) * 10000 / double(bandwidth) * 100000) * (numOfSwitches + 1);
    netHelper.SetDeviceAttribute("DataRate", StringValue(BANDWIDTH_LINK));
    netHelper.SetChannelAttribute("Delay", StringValue(PROPOGATION_DELAY));
    netHelper.data_fileName = FCT_fileName;
    netHelper.rtt = rtt;

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

    ipv4Helper.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e0l0 = ipv4Helper.Assign(devices_e0l0);
    ipv4Helper.SetBase("10.2.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e1l0 = ipv4Helper.Assign(devices_e1l0);
    ipv4Helper.SetBase("10.2.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e2l0 = ipv4Helper.Assign(devices_e2l0);
    ipv4Helper.SetBase("10.2.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e3l0 = ipv4Helper.Assign(devices_e3l0);
    ipv4Helper.SetBase("10.2.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e4l0 = ipv4Helper.Assign(devices_e4l0);
    ipv4Helper.SetBase("10.2.6.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e5l0 = ipv4Helper.Assign(devices_e5l0);
    ipv4Helper.SetBase("10.2.7.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e6l0 = ipv4Helper.Assign(devices_e6l0);
    ipv4Helper.SetBase("10.2.8.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e7l0 = ipv4Helper.Assign(devices_e7l0);

    ipv4Helper.SetBase("10.2.9.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e8l1 = ipv4Helper.Assign(devices_e8l1);
    ipv4Helper.SetBase("10.2.10.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e9l1 = ipv4Helper.Assign(devices_e9l1);
    ipv4Helper.SetBase("10.2.11.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e10l1 = ipv4Helper.Assign(devices_e10l1);
    ipv4Helper.SetBase("10.2.12.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e11l1 = ipv4Helper.Assign(devices_e11l1);
    ipv4Helper.SetBase("10.2.13.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e12l1 = ipv4Helper.Assign(devices_e12l1);
    ipv4Helper.SetBase("10.2.14.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e13l1 = ipv4Helper.Assign(devices_e13l1);
    ipv4Helper.SetBase("10.2.15.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e14l1 = ipv4Helper.Assign(devices_e14l1);
    ipv4Helper.SetBase("10.2.16.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e15l1 = ipv4Helper.Assign(devices_e15l1);

    ipv4Helper.SetBase("10.2.17.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e16l2 = ipv4Helper.Assign(devices_e16l2);
    ipv4Helper.SetBase("10.2.18.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e17l2 = ipv4Helper.Assign(devices_e17l2);
    ipv4Helper.SetBase("10.2.19.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e18l2 = ipv4Helper.Assign(devices_e18l2);
    ipv4Helper.SetBase("10.2.20.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e19l2 = ipv4Helper.Assign(devices_e19l2);
    ipv4Helper.SetBase("10.2.21.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e20l2 = ipv4Helper.Assign(devices_e20l2);
    ipv4Helper.SetBase("10.2.22.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e21l2 = ipv4Helper.Assign(devices_e21l2);
    ipv4Helper.SetBase("10.2.23.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e22l2 = ipv4Helper.Assign(devices_e22l2);
    ipv4Helper.SetBase("10.2.24.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e23l2 = ipv4Helper.Assign(devices_e23l2);

    ipv4Helper.SetBase("10.2.25.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e24l3 = ipv4Helper.Assign(devices_e24l3);
    ipv4Helper.SetBase("10.2.26.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e25l3 = ipv4Helper.Assign(devices_e25l3);
    ipv4Helper.SetBase("10.2.27.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e26l3 = ipv4Helper.Assign(devices_e26l3);
    ipv4Helper.SetBase("10.2.28.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e27l3 = ipv4Helper.Assign(devices_e27l3);
    ipv4Helper.SetBase("10.2.29.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e28l3 = ipv4Helper.Assign(devices_e28l3);
    ipv4Helper.SetBase("10.2.30.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e29l3 = ipv4Helper.Assign(devices_e29l3);
    ipv4Helper.SetBase("10.2.31.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e30l3 = ipv4Helper.Assign(devices_e30l3);
    ipv4Helper.SetBase("10.2.32.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_e31l3 = ipv4Helper.Assign(devices_e31l3);

    double request_rate = LOAD * DataRate(BANDWIDTH_LINK).GetBitRate() / (8 * MtuUtility::avg_cdf(cdfTable));
    std::cout << request_rate << std::endl;
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::vector<Ipv4Address> dstAddress;
    dstAddress.push_back(interface_e0l0.GetAddress(0));
    dstAddress.push_back(interface_e1l0.GetAddress(0));
    dstAddress.push_back(interface_e2l0.GetAddress(0));
    dstAddress.push_back(interface_e3l0.GetAddress(0));
    dstAddress.push_back(interface_e4l0.GetAddress(0));
    dstAddress.push_back(interface_e5l0.GetAddress(0));
    dstAddress.push_back(interface_e6l0.GetAddress(0));
    dstAddress.push_back(interface_e7l0.GetAddress(0));
    dstAddress.push_back(interface_e8l1.GetAddress(0));
    dstAddress.push_back(interface_e9l1.GetAddress(0));
    dstAddress.push_back(interface_e10l1.GetAddress(0));
    dstAddress.push_back(interface_e11l1.GetAddress(0));
    dstAddress.push_back(interface_e12l1.GetAddress(0));
    dstAddress.push_back(interface_e13l1.GetAddress(0));
    dstAddress.push_back(interface_e14l1.GetAddress(0));
    dstAddress.push_back(interface_e15l1.GetAddress(0));
    dstAddress.push_back(interface_e16l2.GetAddress(0));
    dstAddress.push_back(interface_e17l2.GetAddress(0));
    dstAddress.push_back(interface_e18l2.GetAddress(0));
    dstAddress.push_back(interface_e19l2.GetAddress(0));
    dstAddress.push_back(interface_e20l2.GetAddress(0));
    dstAddress.push_back(interface_e21l2.GetAddress(0));
    dstAddress.push_back(interface_e22l2.GetAddress(0));
    dstAddress.push_back(interface_e23l2.GetAddress(0));
    dstAddress.push_back(interface_e24l3.GetAddress(0));
    dstAddress.push_back(interface_e25l3.GetAddress(0));
    dstAddress.push_back(interface_e26l3.GetAddress(0));
    dstAddress.push_back(interface_e27l3.GetAddress(0));
    dstAddress.push_back(interface_e28l3.GetAddress(0));
    dstAddress.push_back(interface_e29l3.GetAddress(0));
    dstAddress.push_back(interface_e30l3.GetAddress(0));
    dstAddress.push_back(interface_e31l3.GetAddress(0));

    //forwarding delay in switches, half for each netdevice
    //1968 nanoseconds for 10G prots, 4587 nanoseconds for GE ports, 5928nanoseconds for 100M ports
    // uint32_t delay = 4587;

    uint32_t flowCount = 0;
    double delay_prop = double(Time(PROPOGATION_DELAY).GetMicroSeconds()) / 1000;
    std::cout << delay_prop << std::endl;
    double delay_process, delay_tx, delay_rx = 0;
    double end_gen_time = 10000.0 / request_rate / 32;
    END_TIME = end_gen_time * 10;
    uint64_t bandwidth = DataRate(BANDWIDTH_LINK).GetBitRate();

    netHelper.InstallAllApplications(ends, ends, request_rate, cdfTable, dstAddress, flowCount, PORT_START, PORT_END, START_TIME, END_TIME, END_TIME,
                                     delay_prop, delay_process);
    std::cout << "flow count is" << flowCount << std::endl;

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    Simulator::Stop(Seconds(END_TIME));
    Simulator::Run();

    flowMonitor->CheckForLostPackets();
    flowMonitor->SerializeToXmlFile("data/leaf-spine/multiqueue.xml", true, true);

    Simulator::Destroy();
    MtuUtility::free_cdf(cdfTable);

    return 0;
}
