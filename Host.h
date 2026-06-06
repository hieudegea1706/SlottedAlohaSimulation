/*
 * Host.h  –  Slotted ALOHA: Trạm phát gói (OOP Header)
 *
 * Kế thừa cSimpleModule, bám sát kiến trúc samples/aloha của OMNeT++ 6.4.0.
 * Sử dụng sendDirect() để gửi gói trực tiếp đến Server (không cần gate kết nối).
 * Hoạt ảnh trực quan: vòng sóng RF (cRingFigure + cOvalFigure) trên Qtenv canvas.
 */

#ifndef __SLOTTEDALOHA_HOST_H_
#define __SLOTTEDALOHA_HOST_H_

#include <omnetpp.h>
#include <vector>

using namespace omnetpp;

namespace slottedaloha {

/**
 * Trạm phát gói trong mạng Slotted ALOHA.
 * Phát sinh gói theo tiến trình Poisson (exponential inter-arrival),
 * căn chỉnh thời gian gửi về đầu slot tiếp theo (Slotted ALOHA).
 */
class Host : public cSimpleModule
{
  private:
    // === Tham số lưu lượng ===
    double    txRate;           // tốc độ kênh (bps)
    cPar     *pkLenBits;        // kích thước gói (bit) – cPar* cho volatile int
    double    iaTime_mean;      // mean của phân phối Poisson (hằng số, đọc 1 lần)
    simtime_t slotTime;         // T = pkLenBits / txRate (tính trong initialize)
    bool      isSlotted;        // true nếu slotTime > 0

    // === Truyền sóng vô tuyến ===
    cModule  *server;           // con trỏ tới module Server (tìm qua getModuleByPath)
    simtime_t radioDelay;       // độ trễ lan truyền = dist(Host,Server) / c
    const double propagationSpeed = 299792458.0; // tốc độ ánh sáng (m/s)

    // === Trạng thái và sự kiện ===
    cMessage *endTxEvent = nullptr;
    enum { IDLE = 0, TRANSMIT = 1 } state;
    simsignal_t stateSignal;
    int pkCounter;

    // === Tọa độ không gian ===
    double x, y, z;             // đơn vị: m

    // === Tham số hoạt ảnh ===
    const double ringMaxRadius     = 2000;  // bán kính tối đa vòng sóng (m)
    const double circlesMaxRadius  = 1000;  // bán kính tối đa vòng gợn (m)
    double idleAnimationSpeed;
    double transmissionEdgeAnimationSpeed;
    double midtransmissionAnimationSpeed;

    // === Đối tượng hình đồ họa (Qtenv Canvas) ===
    cPacket *lastPacket = nullptr;                      // bản sao gói cuối (cho animation)
    mutable cRingFigure *transmissionRing = nullptr;    // vòng sóng chính
    mutable std::vector<cOvalFigure *> transmissionCircles; // vòng gợn sóng

  public:
    virtual ~Host();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    /** Tính thời điểm gửi gói tiếp theo (căn lề slot nếu isSlotted). */
    simtime_t getNextTransmissionTime();
};

}; // namespace slottedaloha

#endif // __SLOTTEDALOHA_HOST_H_
