#include "mtu-bulksend-application.h"
#include "ns3/log.h"
#include "priority-tag.h"
#include "ns3/pointer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MtuBulkSendApplication");
NS_OBJECT_ENSURE_REGISTERED(MtuBulkSendApplication);

TypeId MtuBulkSendApplication::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MtuBulkSendApplication")
                            .SetParent<Application>()
                            .SetGroupName("mtu")
                            .AddConstructor<MtuBulkSendApplication>()
                            .AddAttribute("SendSize", "The amount of data to send each time.",
                                          UintegerValue(512),
                                          MakeUintegerAccessor(&MtuBulkSendApplication::m_sendSize),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("Remote", "The address of the destination",
                                          AddressValue(),
                                          MakeAddressAccessor(&MtuBulkSendApplication::m_peer),
                                          MakeAddressChecker())
                            .AddAttribute("MaxBytes",
                                          "The total number of bytes to send. "
                                          "Once these bytes are sent, "
                                          "no data  is sent again. The value zero means "
                                          "that there is no limit.",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&MtuBulkSendApplication::m_maxBytes),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("Protocol", "The type of protocol to use.",
                                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                                          MakeTypeIdAccessor(&MtuBulkSendApplication::m_tid),
                                          MakeTypeIdChecker())
                            .AddAttribute("FlowLevelSize", "distinguish flow size", UintegerValue(0),
                                          MakeUintegerAccessor(&MtuBulkSendApplication::m_flowSizeLevel),
                                          MakeTypeIdChecker())
                            .AddTraceSource("Tx", "A new packet is created and is sent",
                                            MakeTraceSourceAccessor(&MtuBulkSendApplication::m_txTrace),
                                            "ns3::Packet::TracedCallback");

    return tid;
}

MtuBulkSendApplication::MtuBulkSendApplication()
    : m_socket(0),
      m_connected(false),
      m_totBytes(0),
      m_priority(0)
{
    NS_LOG_FUNCTION(this);
}

MtuBulkSendApplication::~MtuBulkSendApplication()
{
    NS_LOG_FUNCTION(this);
}

void MtuBulkSendApplication::SetMaxBytes(uint32_t maxBytes)
{
    NS_LOG_FUNCTION(this);
    m_maxBytes = maxBytes;
}

Ptr<Socket> MtuBulkSendApplication::GetSocket(void) const
{
    return m_socket;
}

void MtuBulkSendApplication::SetPriority(uint32_t priority)
{
    m_priority = priority;
}

uint32_t MtuBulkSendApplication::GetPriority() const
{
    return m_priority;
}

void MtuBulkSendApplication::SetFlowSizeLevel(uint32_t flowSizeLevel)
{
    m_flowSizeLevel = flowSizeLevel;
}

uint32_t MtuBulkSendApplication::GetFlowSizeLevel(void) const
{
    return m_flowSizeLevel;
}

void MtuBulkSendApplication::SetSequenceNumber(int flowCount)
{
    seq_num = flowCount;
}

int MtuBulkSendApplication::GetSequenceNumber() const
{
    return seq_num;
}

void MtuBulkSendApplication::SetSegmentSize(uint32_t segSize)
{
    m_segmentSize = segSize;
}

uint32_t MtuBulkSendApplication::GetSegmentSize(void)
{
    return m_segmentSize;
}

void MtuBulkSendApplication::SetNdBandwidth(uint64_t bandwidth)
{
    m_bandwidth = bandwidth;
}

uint64_t MtuBulkSendApplication::GetNdBandwidth()
{
    return m_bandwidth;
}

void MtuBulkSendApplication::DoDispose(void)
{
    m_socket = 0;
    Application::DoDispose();
}

static void bufferTrace(uint32_t oldValue, uint32_t newValue)
{
    std::cout << "im buffer" << newValue << std::endl;
}

void MtuBulkSendApplication::StartApplication(void)
{

    t_start = Simulator::Now().GetNanoSeconds();
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        Ptr<TcpSocketBase> tcp_socket = DynamicCast<TcpSocketBase>(m_socket);
        tcp_socket->SetAttribute("SegmentSize", UintegerValue(m_sendSize));
        // tcp_socket
        m_socket = tcp_socket;
        // the buffer for socket
        // Ptr<TcpSocketBase> tcp_socketbase = DynamicCast<TcpSocketBase>(tcp_socket);
        // tcp_socketbase->GetTxBuffer()->TraceConnectWithoutContext("BufferSize", MakeCallback(&bufferTrace));

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM && m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET)
        {
            NS_FATAL_ERROR("Using BulkSend with an incompatible socket type. "
                           "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
        }

        if (Inet6SocketAddress::IsMatchingType(m_peer))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }
        else if (InetSocketAddress::IsMatchingType(m_peer))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }

        m_socket->Connect(m_peer);
        m_socket->ShutdownRecv();

        m_socket->SetConnectCallback(
            MakeCallback(&MtuBulkSendApplication::ConnectionSucceeded, this),
            MakeCallback(&MtuBulkSendApplication::ConnectionFailed, this));
        m_socket->SetSendCallback(
            MakeCallback(&MtuBulkSendApplication::DataSend, this));
    }

    // std::cout << "connected" << m_connected << std::endl;

    if (m_connected)
    {
        SendData();
    }
}

void MtuBulkSendApplication::StopApplication(void) // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);

    if (m_socket != 0)
    {
        m_socket->Close();
        m_connected = false;
    }
    else
    {
        NS_LOG_WARN("BulkSendApplication found null socket to close in StopApplication");
    }
}

void MtuBulkSendApplication::SendData()
{
    // std::cout << "send out data packet" << std::endl;
    NS_LOG_FUNCTION(this);
    while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
        //set m_sendsize according to flowsizelevel

        uint64_t toSend = m_sendSize;
        //
        if (m_maxBytes > 0)
        {
            // toSend = std::min(toSend, m_maxBytes - m_totBytes);
            toSend = std::min(toSend, m_maxBytes - m_totBytes);
        }
        NS_LOG_LOGIC("sending packet at " << Simulator::Now());
        Ptr<Packet> packet = Create<Packet>(toSend);

        PriorityTag tag;
        // if (m_totBytes == 0)
        m_priority = 2;
        tag.SetTimeStamp(t_start);
        tag.SetPriorityTag(m_priority);
        tag.SetFLowsizeTag(m_maxBytes);
        tag.SetTotalTag(m_totBytes);
        tag.SetSeqTag(seq_num);
        tag.SetBandwidth(m_bandwidth);
        packet->AddPacketTag(tag);

        // std::cout << "im from tag " << tag.GetPriorityTag() << std::endl;
        // PriorityTag tag2;
        // packet->PeekPacketTag(tag2);
        // std::cout << "from tag2 maxbytes " << m_maxBytes << "tag " << tag2.GetFlowsizeTag() << std::endl;

        int actual = m_socket->Send(packet);
        if (actual > 0)
        {
            m_totBytes += actual;
            m_txTrace(packet);
        }

        // We exit this loop when actual < toSend as the send side
        // buffer is full. The "DataSent" callback will pop when
        // some buffer space has freed ip.
        if ((unsigned)actual != toSend)
        {
            break;
        }

        // Check if time to close (all sent)
        if (m_totBytes == m_maxBytes && m_connected)
        {
            m_socket->Close();
            m_connected = false;
        }
    }
}

void MtuBulkSendApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("BulkSendApplication Connection succeeded");
    m_connected = true;
    SendData();
}

void MtuBulkSendApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("BulkSendApplication, Connection Failed");
}

void MtuBulkSendApplication::DataSend(Ptr<Socket>, uint32_t)
{

    NS_LOG_FUNCTION(this);

    if (m_connected)
    { // Only send new data if the connection has completed
        SendData();
    }
}