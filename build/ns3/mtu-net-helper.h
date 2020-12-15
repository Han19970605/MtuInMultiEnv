#ifndef MTU_NET_HELPER_H
#define MTU_NET_HELPER_H

#include "ns3/object.h"
#include "ns3/network-module.h"
#include "ns3/object-factory.h"
#include "ns3/mtu-utility.h"
#include "ns3/internet-module.h"

namespace ns3
{
    class MtuNetHelper : public Object
    {
    public:
        MtuNetHelper();
        ~MtuNetHelper();
        /**
         * Application install part
         * 
         * InstallAllApplications update on 12/15 
         * Adapt to both datacenter and wan, use the fixed generated flow data
        */
        ApplicationContainer InstallApplication(Ptr<Node> sendNode, Ptr<Node> sinkNode, Ipv4Address remoteAdress,
                                                uint16_t port, uint32_t maxBytes, uint32_t sendSize, uint32_t priority, uint32_t flowCount, uint64_t bandwidth);
        void InstallAllApplications(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                    std::vector<Ipv4Address> destAddresses, uint32_t &flowCount, int port_start, int port_end,
                                    double timesim_start, double timesim_end, double time_flow_launch_end, double delay_prop,
                                    double delay_process);

        void InstallAllApplicationsInDC(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                        std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                        double timesim_end, double time_flow_launch_end, uint64_t bandwidth, double delay_prop, double delay_process,
                                        double delay_tx, double delay_rx);
        void InstallAllApplicationsInWAN(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                         std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                         double timesim_end, double time_flow_launch_end, uint64_t bandwidth, int numOfSwitches, double delay_prop,
                                         double delay_process, double delay_tx, double delay_rx);
        /**
         * \param bandwidth_dc bandwidth in datacenter link
         * \param bandwidth_wan bandwidth in wan link
         * \param delay_prop_dc propogation delay in dc
         * \param delay_prop_wan propogation delay in wan
         * \param delay_process_wan process delay in wan switches
         * \param delay_process_dc process delay in dc switches
         * \param delay_tx the delay from application to the netdevice
         * \param delay_rx the delay in the rx_buffer in host
         * \param src_type "dc" or "wan" use to indicate whether should use the bandwidth of dc or wan to estimate the best MTU
        */
        void InstallAllApplicationsInMix(NodeContainer fromServers, NodeContainer destServers, double requestRate, struct cdf_table *cdfTable,
                                         std::vector<Ipv4Address> destAddress, uint32_t &flowCount, int port_start, int port_end, double timesim_start,
                                         double timesim_end, double time_flow_launch_end, uint64_t bandwidth_dc, uint64_t bandwidth_wan, double delay_prop_dc,
                                         double delay_prop_wan, double delay_process_wan,
                                         double delay_process_dc, int numOfSwitches, double delay_tx, double delay_rx, std::string src_type);

        void SetQueue(std::string type,
                      std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue(),
                      std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue(),
                      std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue(),
                      std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue());
        void SetDeviceAttribute(std::string n1, const AttributeValue &v1);
        void SetChannelAttribute(std::string n1, const AttributeValue &v1);

        /**
         * NetDevice installation part
         * \brief InstallNormalNetDevices and Install MtuNetDevices means install netdevice with or without loss
         *        installPointNetDevice means install the pointtopointNetDevice
         *
        */
        NetDeviceContainer InstallNormalNetDevices(Ptr<Node> a, Ptr<Node> b);
        NetDeviceContainer InstallMtuNetDevices(Ptr<Node> loss_node, Ptr<Node> normal_node, double loss_rate);

        NetDeviceContainer InstallPointNetDevice(Ptr<Node> a, Ptr<Node> b);

        // NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b, double loss_rate, uint32_t process_delay, bool type);
        // NetDeviceContainer InstallEnd2Switch(Ptr<Node> end, Ptr<Node> sw, uint32_t process_delay);
        // NetDeviceContainer InstallSwitch2Switch(Ptr<Node> sw1, Ptr<Node> sw2, uint32_t process_delay);

        std::string data_fileName;
        uint64_t rtt;

    private:
        ObjectFactory m_sendApplicationFactory;
        ObjectFactory m_sinkApplicationFactory;

        ObjectFactory m_mtuDeviceFactory;
        ObjectFactory m_pointFactory;

        ObjectFactory m_normalQueueFactory;
        ObjectFactory m_queueFactory;

        ObjectFactory m_channelFactory;
        ObjectFactory m_remoteChannelFactory;
    };
} // namespace ns3

#endif