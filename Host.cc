/*
 * Host.cc
 *
 *  Created on: May 26, 2026
 *      Author: HIEU
 */
#include <omnetpp.h>

using namespace omnetpp;

namespace slottedaloha {

class Host : public cSimpleModule
{
  private:
    cMessage *slotEvent = nullptr;
    double sendProbability;
    int packetLength;
    simtime_t slotTime;

    long generatedPackets = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~Host();
};

Define_Module(Host);

void Host::initialize()
{
    sendProbability = par("sendProbability");
    packetLength = par("packetLength");
    slotTime = par("slotTime");

    slotEvent = new cMessage("slotEvent");

    // Mỗi host bắt đầu đúng tại biên slot
    scheduleAt(simTime() + uniform(0, slotTime.dbl()), slotEvent);
}

void Host::handleMessage(cMessage *msg)
{
    if (msg == slotEvent)
    {
        if (uniform(0, 1) < sendProbability)
        {
            cPacket *pkt = new cPacket("data");
            pkt->setBitLength(packetLength);
            send(pkt, "out");
            generatedPackets++;
        }

        scheduleAt(simTime() + slotTime, slotEvent);
    }
    else
    {
        delete msg;
    }
}

void Host::finish()
{
    recordScalar("generatedPackets", generatedPackets);
}

Host::~Host()
{
    cancelAndDelete(slotEvent);
}

}



