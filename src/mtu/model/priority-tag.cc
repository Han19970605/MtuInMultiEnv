#include "priority-tag.h"

namespace ns3
{
    // NS_LOG_COMPONENT_DEFINE("PriorityTag");
    NS_OBJECT_ENSURE_REGISTERED(PriorityTag);

    TypeId PriorityTag::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::PriorityTag")
                                .SetParent<Tag>()
                                .SetGroupName("mtu")
                                .AddConstructor<PriorityTag>();
        return tid;
    }

    PriorityTag::PriorityTag()
        : m_priorityTag(0), m_seqNumber(0), m_sizeTag(0), m_timestamp(0), m_totalTag(0)
    {
        // NS_LOG_FUNCTION(this);
    }

    PriorityTag::~PriorityTag()
    {
    }

    void PriorityTag::SetPriorityTag(uint32_t PriorityTag)
    {
        m_priorityTag = PriorityTag;
    }

    uint32_t PriorityTag::GetPriorityTag()
    {
        return m_priorityTag;
    }

    void PriorityTag::SetSeqTag(uint32_t seqNumber)
    {
        m_seqNumber = seqNumber;
    }

    uint32_t PriorityTag::GetSeqTag()
    {
        return m_seqNumber;
    }

    void PriorityTag::SetFLowsizeTag(uint64_t flowsizeTag)
    {
        m_sizeTag = flowsizeTag;
    }

    uint64_t PriorityTag::GetFlowsizeTag()
    {
        return m_sizeTag;
    }

    void PriorityTag::SetTotalTag(uint64_t totalTag)
    {
        m_totalTag = totalTag;
        // std::cout << m_totalTag << " " << totalTag << std::endl;
    }

    uint64_t PriorityTag::GetTotalTag()
    {
        return m_totalTag;
    }

    void PriorityTag::SetTimeStamp(uint64_t timestamp)
    {
        m_timestamp = timestamp;
    }

    uint64_t PriorityTag::GetTimeStamp()
    {
        return m_timestamp;
    }

    void PriorityTag::SetBandwidth(uint64_t bandwidth)
    {
        m_nd_bandwidth = bandwidth;
    }

    uint64_t PriorityTag::GetBandwidth()
    {
        return m_nd_bandwidth;
    }

    uint32_t PriorityTag::GetSerializedSize() const
    {
        return sizeof(uint32_t) * 2 + sizeof(uint64_t) * 4;
        // return sizeof(uint32_t);
    }

    TypeId PriorityTag::GetInstanceTypeId(void) const
    {
        return GetTypeId();
    }

    void PriorityTag::Serialize(TagBuffer i) const
    {
        i.WriteU32(m_priorityTag);
        i.WriteU32(m_seqNumber);
        i.WriteU64(m_sizeTag);
        i.WriteU64(m_totalTag);
        i.WriteU64(m_timestamp);
        i.WriteU64(m_nd_bandwidth);
    }

    void PriorityTag::Deserialize(TagBuffer i)
    {
        m_priorityTag = i.ReadU32();
        m_seqNumber = i.ReadU32();
        m_sizeTag = i.ReadU64();
        m_totalTag = i.ReadU64();
        m_timestamp = i.ReadU64();
        m_nd_bandwidth = i.ReadU64();
    }

    void PriorityTag::Print(std::ostream &os) const
    {
        os << "Priority Tag = " << m_priorityTag << " Flowsize Tag = " << m_sizeTag;
    }

} // namespace ns3
