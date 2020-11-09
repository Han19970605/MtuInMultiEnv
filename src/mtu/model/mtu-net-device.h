#ifndef MTUNETDEVICE_H
#define MTUNETDEVICE_H

/*
MTUnetdevice:
1. is inherited from pointtopointnetdevice
2. adds multi-queue net device (one with the highest priority, and others in the weighted scheduling)
3. concects to the corresponding node, and gets the tag information
4. Add loss rate
*/

#include "ns3/point-to-point-net-device.h"
#include "ns3/ptr.h"
#include "ns3/queue.h"

namespace ns3
{

// class PointToPointNetDevice;
class Address;

class MtuNetDevice : public PointToPointNetDevice
{

public:
	static TypeId GetTypeId(void);

	MtuNetDevice();

	virtual ~MtuNetDevice();

	void setLossRate(double lossRate);

	double getLossRate(void) const;

	// void setDelay(uint32_t delay);

	// uint32_t getDelay(void) const;

	virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

	// void DoSend(const Address &address, Ptr<Packet> packet, uint16_t protocolNumber);

	std::string data_fileName;
	uint64_t rtt;

private:
	double loss_rate;

	//unit: nanoseconds
	// uint32_t m_delay;
};
} // namespace ns3

#endif