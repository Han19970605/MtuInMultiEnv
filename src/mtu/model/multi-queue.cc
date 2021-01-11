#include "multi-queue.h"
// #include "ns3/log.h"
#include "priority-tag.h"
#include "ns3/mtu-utility.h"

namespace ns3
{
    // NS_LOG_COMPONENT_DEFINE("MultiQueue");
    NS_OBJECT_ENSURE_REGISTERED(MultiQueue);

    TypeId MultiQueue::GetTypeId(void)
    {
        TypeId tid = TypeId("ns3::MultiQueue")
                         .SetParent<Queue<Packet>>()
                         .SetGroupName("mtu")
                         .AddConstructor<MultiQueue>();
        return tid;
    }

    MultiQueue::MultiQueue()
        : pri_number(3)
    {
        // NS_LOG_FUNCTION(this);
        std::vector<double> weight;
        weight.push_back(0.7);
        weight.push_back(0.3);

        m_weight = weight;
        // std::cout << "refereced here" << std::endl;
        SetNumPriority(3, weight);
    }

    MultiQueue::~MultiQueue()
    {
        // NS_LOG_FUNCTION(this);
    }

    void MultiQueue::SetNumPriority(uint16_t number, std::vector<double> weight)
    {

        //update the number of priorities
        pri_number = number;
        uint16_t cp = number - 1;
        //update the weight vector
        if (number < 1)
        {
            // NS_LOG_ERROR("The number of prioroty queues cannot be less than 1");
            return;
        }
        if (weight.size() != cp)
        {
            // NS_LOG_ERROR("Number of the weights is wrong");
            return;
        }
        pri_number = number;
        m_weight = weight;

        uint16_t current_number = m_queues.size();
        if (current_number < pri_number)
        {
            for (uint16_t i = current_number; i < (pri_number - current_number); i++)
            {
                std::queue<Ptr<Packet>> single_queue;
                m_queues.push_back(single_queue);
            }
        }
        else if (current_number > pri_number)
        {
            std::vector<Ptr<Packet>> vecTemp;
            uint16_t indexStart = pri_number;
            for (uint16_t i = indexStart; i < current_number; i++)
            {
                std::queue<Ptr<Packet>> queTemp = m_queues[i];
                while (queTemp.size() > 0)
                {
                    vecTemp.push_back(queTemp.front());
                    queTemp.pop();
                }
            }
            m_queues.resize(pri_number);
            std::queue<Ptr<Packet>> queBack = m_queues.back();
            for (uint32_t i = 0; i < vecTemp.size(); i++)
            {
                queBack.push(vecTemp[i]);
            }
        }
    }

    bool MultiQueue::Enqueue(Ptr<Packet> packet)
    {
        //peek the priority
        PriorityTag tag;
        bool tagFound = packet->PeekPacketTag(tag);
        // std::cout << "enqueue here" << std::endl;
        // FindFirstMatchingByteTag(tag);
        if (tagFound)
        {
            uint16_t priority = tag.GetPriorityTag();
            // std::cout << "found tag " << priority << " " << tag.GetFlowsizeTag() << std::endl;

            //save to the corresponding queue
            if (priority >= m_queues.size())
            {
                m_queues.back().push(packet);
                return true;
            }
            else
            {
                m_queues[priority].push(packet);
                return true;
            }
        }
        else
        {
            //if not found --> highest priority
            m_queues[0].push(packet);
            // m_queues[m_queues.size() - 1].push(packet);

            return true;
        }
    } // namespace ns3

    Ptr<Packet> MultiQueue::Dequeue()
    {
        Ptr<Packet> packet;
        if (m_queues[0].size() > 0)
        {
            //packets exists in highest priority
            packet = m_queues[0].front();
            m_queues[0].pop();
        }
        else
        {
            //no packets in highest priority
            std::vector<int> nonEmpty;
            //pick non empty queues
            for (uint16_t i = 1; i <= m_weight.size(); i++)
            {
                if (m_queues[i].size() > 0)
                {
                    nonEmpty.push_back(i);
                }
            }
            if (nonEmpty.size() == 0)
                return 0;

            std::vector<double> vecAcc;
            double acc = 0;
            for (uint16_t i = 0; i < nonEmpty.size(); i++)
            {
                acc += m_weight[nonEmpty[i]];
                vecAcc.push_back(acc);
            }
            //normalize
            for (uint16_t i = 0; i < vecAcc.size(); i++)
            {
                vecAcc[i] /= acc;
            }
            double randomValue = MtuUtility::gen_random();
            uint32_t index;
            for (uint16_t i = 0; i < vecAcc.size(); i++)
            {
                index = i;
                if (randomValue < vecAcc[i] && m_queues[i].size() > 0)
                {
                    break;
                }
            }

            uint32_t que_index = nonEmpty[index];
            packet = m_queues[que_index].front();
            m_queues[que_index].pop();
            // std::cout << "queue index " << que_index << "size of queue 1 " << m_queues[1].size() << std::endl;
        }
        return packet;
    }

    Ptr<const Packet> MultiQueue::Peek(void) const
    {
        // NS_LOG_FUNCTION(this);
        for (int i = 0; i < pri_number; i++)
        {
            if (m_queues[i].size() != 0)
            {
                return m_queues[i].front();
            }
        }
        return 0;
    }

    Ptr<Packet> MultiQueue::Remove()
    {
        if (GetTotalNumber() == 0)
        {
            std::cout << "No packets in queue" << std::endl;
            return 0;
        }
        Ptr<Packet> packet;
        for (uint16_t i = 0; i < m_queues.size(); i++)
        {
            if (m_queues[i].size() != 0)
            {
                packet = m_queues[i].front();
            }
        }
        return packet;
    }

    uint32_t MultiQueue::GetTotalNumber()
    {
        uint32_t total_num = 0;

        for (uint16_t i = 0; i < m_queues.size(); i++)
        {
            std::queue<Ptr<Packet>> q = m_queues[i];
            while (q.size() != 0)
            {
                total_num += q.front()->GetSize();
                q.pop();
            }
        }
        // for (uint16_t i = 0; i < m_queues.size(); i++)
        // {
        //     total_num += m_queues[i].size();
        // }
        return total_num;
    }

    uint32_t MultiQueue::GetPktsAhead(uint32_t priority)
    {
        uint32_t bytesAhead = 0;
        if (priority >= m_queues.size())
            return GetTotalNumber();
        else
            for (uint32_t i = 0; i <= priority; i++)
            {
                std::queue<Ptr<Packet>> q = m_queues[i];
                while (q.size() != 0)
                {
                    bytesAhead += q.front()->GetSize();
                    q.pop();
                }
            }

        return bytesAhead;
    }
} // namespace ns3
