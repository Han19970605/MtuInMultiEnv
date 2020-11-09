#ifndef MTU_BULK_SEND_APPLICATION_H
#define MTU_BULK_SEND_APPLICATION_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/address.h"
#include "ns3/socket.h"

using namespace ns3;

// class Address;
// class Socket;

class MtuBulkSendApplication : public Application
{
public:
    static TypeId GetTypeId(void);

    MtuBulkSendApplication();

    virtual ~MtuBulkSendApplication();

    void SetMaxBytes(uint32_t maxBytes);

    Ptr<Socket> GetSocket(void) const;

    void SetPriority(uint32_t priority);

    uint32_t GetPriority(void) const;

    //0 for small flow 1 for medium flow 2 for big flow
    void SetFlowSizeLevel(uint32_t flowSizeLevel);

    uint32_t GetFlowSizeLevel(void) const;

    //use flow count number as sequence number
    void SetSequenceNumber(int seq_number);

    int GetSequenceNumber(void) const;

    void SetSegmentSize(uint32_t segSize);

    uint32_t GetSegmentSize(void);

    void SetNdBandwidth(uint64_t bandwidth);

    uint64_t GetNdBandwidth();

protected:
    virtual void DoDispose(void);

private:
    virtual void StartApplication(void); // Called at time specified by Start

    virtual void StopApplication(void); // Called at time specified by Stop

    void SendData();

    Ptr<Socket> m_socket; //!< Associated socket
    Address m_peer;       //!< Peer address
    bool m_connected;     //!< True if connected
    uint32_t m_sendSize;  //!< Size of data to send each time
    uint64_t m_maxBytes;  //!< Limit total number of bytes sent
    uint64_t m_totBytes;  //!< Total bytes sent so far
    TypeId m_tid;         //!< The type of protocol to use.

    // 0-100k 0 100k-50M 1 50M- 2
    uint32_t m_flowSizeLevel;
    uint32_t m_priority;
    uint32_t m_segmentSize;
    // sequence number
    int seq_num;
    // startTime
    uint64_t t_start;
    // bandwidth in netdevice
    uint64_t m_bandwidth;

    /// Traced Callback: sent packets
    TracedCallback<Ptr<const Packet>>
        m_txTrace;

private:
    /**
   * \brief Connection Succeeded (called by Socket through a callback)
   * \param socket the connected socket
   */
    void ConnectionSucceeded(Ptr<Socket> socket);
    /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
    void ConnectionFailed(Ptr<Socket> socket);
    /**
   * \brief Send more data as soon as some has been transmitted.
   */
    void DataSend(Ptr<Socket>, uint32_t); // for socket's SetSendCallback
};

#endif