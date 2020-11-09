#ifndef MULTIQUEUE_H
#define MULTIQUEUE_H

#include "ns3/queue.h"
// #include "ns3/log.h"
#include <queue>

namespace ns3
{

    class MultiQueue : public Queue<Packet>
    {
    public:
        static TypeId GetTypeId(void);

        MultiQueue(/* args */);
        virtual ~MultiQueue();

        virtual bool Enqueue(Ptr<Packet> item);
        virtual Ptr<Packet> Dequeue(void);
        virtual Ptr<Packet> Remove(void);
        virtual Ptr<const Packet> Peek(void) const;
        void SetNumPriority(uint16_t pri_number, std::vector<double> weight);
        // return the size in bytes
        uint32_t GetTotalNumber(void);

    private:
        //number of priorities
        //3 or 8
        uint16_t pri_number;

        //priority queues
        std::vector<std::queue<Ptr<Packet>>> m_queues;
        std::vector<double> m_weight;
    };

} // namespace ns3

#endif