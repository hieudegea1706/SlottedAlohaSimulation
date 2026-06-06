/*
 * Server.h  –  Slotted ALOHA: Bộ thu trung tâm (OOP Header)
 *
 * Thay thế Channel.cc cũ. Nhận gói từ các Host qua cổng @directIn (sendDirect).
 * Sử dụng phát hiện va chạm theo slot (slot-based collision detection):
 *   0 gói/slot  → Idle slot
 *   1 gói/slot  → Success slot
 *  ≥2 gói/slot  → Collision slot
 *
 * Tính toán và xuất các chỉ số hiệu năng qua recordScalar() khi kết thúc mô phỏng.
 * Trực quan hóa trạng thái kênh qua refreshDisplay() (màu icon thay đổi theo trạng thái).
 */

#ifndef __SLOTTEDALOHA_SERVER_H_
#define __SLOTTEDALOHA_SERVER_H_

#include <omnetpp.h>
#include <vector>
#include <cmath>

using namespace omnetpp;

namespace slottedaloha {

/**
 * Bộ thu trung tâm trong mạng Slotted ALOHA.
 * Quản lý kênh truyền dùng chung theo phương pháp đếm gói trong slot.
 */
class Server : public cSimpleModule
{
  private:
    // === Tham số hệ thống ===
    int       numHosts;         // số lượng trạm (dùng cho ghi cfg_ scalar)
    int       pkLenBits_val;    // kích thước gói (bit)
    double    txRate;           // tốc độ kênh (bps)
    simtime_t slotTime;         // T = pkLenBits / txRate (tính trong initialize)

    // === Bộ đệm slot ===
    cMessage *endSlotEvent = nullptr;
    std::vector<cPacket *> packetsInCurrentSlot; // gói tích lũy trong slot hiện tại

    // === Trạng thái kênh (cập nhật real-time khi gói đến) ===
    enum ChannelState { IDLE_STATE = 0, TRANSMISSION = 1, COLLISION = 2 };
    ChannelState channelState;

    // === Bộ đếm theo slot ===
    long totalSlots        = 0;
    long idleSlots         = 0;
    long successSlots      = 0;
    long collisionSlots    = 0;

    // === Bộ đếm theo gói ===
    long totalAttempts     = 0;   // tổng số lượt thử gửi
    long successfulPackets = 0;   // gói truyền thành công
    long collidedPackets   = 0;   // gói bị va chạm

    // === Tín hiệu Qtenv ===
    simsignal_t channelStateSignal;
    simsignal_t receiveBeginSignal;
    simsignal_t receiveSignal;
    simsignal_t collisionSignal;
    simsignal_t collisionLengthSignal;
    intval_t receiveCounter = 0;

    // === Thống kê thời gian (cho signals) ===
    simtime_t recvStartTime;    // thời điểm bắt đầu nhận gói đầu tiên trong slot
    intval_t  currentCollisionNumFrames = 0;

  public:
    virtual ~Server();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;
};

}; // namespace slottedaloha

#endif // __SLOTTEDALOHA_SERVER_H_
