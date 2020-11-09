#ifndef PRIORITY_TAG_H
#define PRIORITY_TAG_H

#include "ns3/tag.h"
namespace ns3
{
    class PriorityTag : public Tag
    {
    public:
        PriorityTag();
        ~PriorityTag();
        static TypeId GetTypeId(void);
        void SetPriorityTag(uint32_t priorityTag);
        uint32_t GetPriorityTag(void);
        void SetSeqTag(uint32_t seqNumber);
        uint32_t GetSeqTag();
        void SetFLowsizeTag(uint64_t flowsizeTag);
        uint64_t GetFlowsizeTag();
        void SetTotalTag(uint64_t totalTag);
        uint64_t GetTotalTag();
        void SetTimeStamp(uint64_t timestamp);
        uint64_t GetTimeStamp();
        void SetBandwidth(uint64_t bandwidth);
        uint64_t GetBandwidth();

        virtual uint32_t GetSerializedSize(void) const;
        virtual TypeId GetInstanceTypeId(void) const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual void Print(std::ostream &os) const;

    private:
        //assigned priority
        uint32_t m_priorityTag;
        //sequence number
        uint32_t m_seqNumber;
        //flowsize
        uint64_t m_sizeTag;
        //sent bytes
        uint64_t m_totalTag;
        //timestamp
        uint64_t m_timestamp;
        //bandwidth
        uint64_t m_nd_bandwidth;
    };
} // namespace ns3

#endif