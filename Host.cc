/*
 * Host.cc  –  Slotted ALOHA: Poisson arrival model
 *
 * Mỗi host phát sinh gói theo tiến trình Poisson với tốc độ λ_host = 1/iaTime.
 * Gói phát sinh tại thời điểm bất kỳ trong slot nhưng được gửi vào đầu slot
 * tiếp theo (slot-boundary alignment) đúng chuẩn Slotted ALOHA.
 *
 * Công thức tải:
 *   T        = pkLenBits / txRate  = 960 / 9600 = 0.1 s
 *   G_total  = numHosts × (T / iaTime)
 *
 *   LightLoad:  iaTime=20s  → G = 20 × (0.1/20)  = 0.1
 *   MediumLoad: iaTime=2s   → G = 20 × (0.1/2)   = 1.0
 *   HighLoad:   iaTime=0.5s → G = 20 × (0.1/0.5) = 4.0
 */
#include <omnetpp.h>

using namespace omnetpp;

namespace slottedaloha {

class Host : public cSimpleModule
{
  private:
    // Tham số Poisson
    double    iaTime;           // inter-arrival time trung bình (s)
    int       pkLenBits;        // kích thước gói (bit)
    double    txRate;           // tốc độ kênh (bps)
    simtime_t slotTime;         // T = pkLenBits / txRate (s) — tính trong initialize()

    // Sự kiện nội bộ
    cMessage *arrivalEvent = nullptr;   // "gói Poisson mới đến"
    cMessage *sendEvent    = nullptr;   // "gửi vào đầu slot tiếp theo"

    // Thống kê
    long generatedPackets = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~Host();
};

Define_Module(Host);

// ---------------------------------------------------------------
void Host::initialize()
{
    iaTime    = par("iaTime").doubleValue();    // đơn vị: s
    pkLenBits = par("pkLenBits");               // đơn vị: bit
    txRate    = par("txRate").doubleValue();    // đơn vị: bps

    // slotTime = T = L / R
    slotTime  = pkLenBits / txRate;             // = 960 / 9600 = 0.1 s

    arrivalEvent = new cMessage("arrivalEvent");
    sendEvent    = new cMessage("sendEvent");

    // Lịch gói đầu tiên — offset ngẫu nhiên tránh tất cả host bắt đầu cùng lúc
    scheduleAt(simTime() + exponential(iaTime), arrivalEvent);
}

// ---------------------------------------------------------------
void Host::handleMessage(cMessage *msg)
{
    if (msg == arrivalEvent)
    {
        // ---- Gói Poisson mới đến ----
        // Căn chỉnh về đầu slot tiếp theo
        simtime_t now          = simTime();
        long      slotIndex    = (long)(now.dbl() / slotTime.dbl());  // số slot nguyên
        simtime_t nextSlotTime = SimTime((slotIndex + 1) * slotTime.dbl());

        // Lịch sự kiện gửi vào đầu slot tiếp theo
        // (nếu sendEvent đã được lịch → hủy để tránh trùng)
        if (sendEvent->isScheduled())
            cancelEvent(sendEvent);
        scheduleAt(nextSlotTime, sendEvent);

        // Lịch gói Poisson tiếp theo
        scheduleAt(now + exponential(iaTime), arrivalEvent);
    }
    else if (msg == sendEvent)
    {
        // ---- Gửi gói vào đầu slot ----
        cPacket *pkt = new cPacket("data");
        pkt->setBitLength(pkLenBits);
        send(pkt, "out");
        generatedPackets++;
    }
    else
    {
        delete msg;  // tin hiệu từ channel (ACK tượng trưng) — bỏ qua
    }
}

// ---------------------------------------------------------------
void Host::finish()
{
    recordScalar("generatedPackets", generatedPackets);
}

// ---------------------------------------------------------------
Host::~Host()
{
    cancelAndDelete(arrivalEvent);
    cancelAndDelete(sendEvent);
}

} // namespace slottedaloha
