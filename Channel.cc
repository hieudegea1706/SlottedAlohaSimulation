/*
 * Channel.cc
 *
 *  Created on: May 26, 2026
 *      Author: HIEU
 */
#include <omnetpp.h>
#include <vector>

using namespace omnetpp;

namespace slottedaloha {

class Channel : public cSimpleModule
{
  private:
    cMessage *endSlotEvent = nullptr;
    std::vector<cPacket*> packetsInCurrentSlot;

    int numHosts;
    int packetLength;
    double bandwidth;
    simtime_t slotTime;

    long totalSlots = 0;
    long idleSlots = 0;
    long successSlots = 0;
    long collisionSlots = 0;

    long totalAttempts = 0;
    long successfulPackets = 0;
    long collidedPackets = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~Channel();
};

Define_Module(Channel);

void Channel::initialize()
{
    numHosts = par("numHosts");
    packetLength = par("packetLength");
    bandwidth = par("bandwidth");
    slotTime = par("slotTime");

    endSlotEvent = new cMessage("endSlotEvent");

    // Channel kiểm tra kết quả ở cuối mỗi slot
    scheduleAt(simTime() + slotTime, endSlotEvent);
}

void Channel::handleMessage(cMessage *msg)
{
    if (msg == endSlotEvent)
    {
        totalSlots++;

        int n = packetsInCurrentSlot.size();
        totalAttempts += n;

        if (n == 0)
        {
            idleSlots++;
        }
        else if (n == 1)
        {
            successSlots++;
            successfulPackets++;

            // gửi ACK tượng trưng về một host bất kỳ
            delete packetsInCurrentSlot[0];
        }
        else
        {
            collisionSlots++;
            collidedPackets += n;

            for (auto pkt : packetsInCurrentSlot)
                delete pkt;
        }

        packetsInCurrentSlot.clear();

        scheduleAt(simTime() + slotTime, endSlotEvent);
    }
    else
    {
        cPacket *pkt = check_and_cast<cPacket *>(msg);
        packetsInCurrentSlot.push_back(pkt);
    }
}

void Channel::finish()
{
    double offeredLoad = totalSlots > 0 ? (double)totalAttempts / totalSlots : 0;
    double throughput = totalSlots > 0 ? (double)successfulPackets / totalSlots : 0;
    double channelUtilization = totalSlots > 0 ? (double)successSlots / totalSlots : 0;
    double collisionRate = totalSlots > 0 ? (double)collisionSlots / totalSlots : 0;
    double idleRate = totalSlots > 0 ? (double)idleSlots / totalSlots : 0;
    double successRatio = totalAttempts > 0 ? (double)successfulPackets / totalAttempts : 0;

    recordScalar("totalSlots", totalSlots);
    recordScalar("idleSlots", idleSlots);
    recordScalar("successSlots", successSlots);
    recordScalar("collisionSlots", collisionSlots);

    recordScalar("totalAttempts", totalAttempts);
    recordScalar("successfulPackets", successfulPackets);
    recordScalar("collidedPackets", collidedPackets);

    recordScalar("offeredLoad_G", offeredLoad);
    recordScalar("throughput_S", throughput);
    recordScalar("channelUtilization", channelUtilization);
    recordScalar("collisionRate", collisionRate);
    recordScalar("idleRate", idleRate);
    recordScalar("successRatio", successRatio);
}

Channel::~Channel()
{
    cancelAndDelete(endSlotEvent);

    for (auto pkt : packetsInCurrentSlot)
        delete pkt;
}

}



