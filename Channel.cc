/*
 * Channel.cc  –  Slotted ALOHA: kênh dùng chung, phân slot
 *
 * Channel đếm số gói nhận được trong mỗi slot:
 *   0 gói  → Idle slot
 *   1 gói  → Success slot
 *  ≥2 gói  → Collision slot
 *
 * Chỉ số cuối mô phỏng (finish()):
 *   G  = totalAttempts  / totalSlots  (offered load thực tế)
 *   S  = successSlots   / totalSlots  (throughput)
 *   CR = collisionSlots / totalSlots  (collision rate)
 *   IR = idleSlots      / totalSlots  (idle rate)
 *   SR = successfulPackets / totalAttempts (success ratio)
 *
 * Lý thuyết Slotted ALOHA: S = G · e^(-G)
 */
#include <omnetpp.h>
#include <vector>
#include <cmath>

using namespace omnetpp;

namespace slottedaloha {

class Channel : public cSimpleModule
{
  private:
    cMessage *endSlotEvent = nullptr;
    std::vector<cPacket *> packetsInCurrentSlot;

    // Tham số hệ thống
    int    numHosts;
    int    pkLenBits;
    double txRate;
    simtime_t slotTime;   // T = pkLenBits / txRate

    // Bộ đếm theo slot
    long totalSlots       = 0;
    long idleSlots        = 0;
    long successSlots     = 0;
    long collisionSlots   = 0;

    // Bộ đếm theo gói
    long totalAttempts    = 0;   // tổng số gói cố gắng gửi
    long successfulPackets= 0;   // gói truyền thành công
    long collidedPackets  = 0;   // gói bị va chạm

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~Channel();
};

Define_Module(Channel);

// ---------------------------------------------------------------
void Channel::initialize()
{
    numHosts  = par("numHosts");
    pkLenBits = par("pkLenBits");
    txRate    = par("txRate").doubleValue();

    // slotTime = T = L / R
    slotTime  = (double)pkLenBits / txRate;   // = 960 / 9600 = 0.1 s

    EV << "[Channel] slotTime = " << slotTime << " s\n";

    endSlotEvent = new cMessage("endSlotEvent");
    scheduleAt(simTime() + slotTime, endSlotEvent);
}

// ---------------------------------------------------------------
void Channel::handleMessage(cMessage *msg)
{
    if (msg == endSlotEvent)
    {
        // ===== Kết thúc slot: phân loại kết quả =====
        totalSlots++;
        int n = (int)packetsInCurrentSlot.size();
        totalAttempts += n;

        if (n == 0)
        {
            // --- Idle ---
            idleSlots++;
        }
        else if (n == 1)
        {
            // --- Success ---
            successSlots++;
            successfulPackets++;
            delete packetsInCurrentSlot[0];
        }
        else
        {
            // --- Collision ---
            collisionSlots++;
            collidedPackets += n;
            for (auto pkt : packetsInCurrentSlot)
                delete pkt;
        }

        packetsInCurrentSlot.clear();

        // Lịch slot kế tiếp
        scheduleAt(simTime() + slotTime, endSlotEvent);
    }
    else
    {
        // ===== Gói đến từ một Host =====
        cPacket *pkt = check_and_cast<cPacket *>(msg);
        packetsInCurrentSlot.push_back(pkt);
    }
}

// ---------------------------------------------------------------
void Channel::finish()
{
    if (totalSlots == 0) {
        EV << "[Channel] Không có slot nào được ghi nhận!\n";
        return;
    }

    // ---- Tính các chỉ số ----
    double G  = (double)totalAttempts   / totalSlots;   // Offered Load
    double S  = (double)successSlots    / totalSlots;   // Throughput
    double CR = (double)collisionSlots  / totalSlots;   // Collision Rate
    double IR = (double)idleSlots       / totalSlots;   // Idle Rate
    double SR = (totalAttempts > 0) ? (double)successfulPackets / totalAttempts : 0.0;

    double S_theory = G * std::exp(-G);                 // Lý thuyết: S = G·e^(-G)

    // ---- Ghi scalar ----
    // Tham số cấu hình (để Python đọc được khi phân nhóm phase)
    recordScalar("cfg_numHosts",  numHosts);
    recordScalar("cfg_pkLenBits", pkLenBits);
    recordScalar("cfg_txRate",    txRate);
    recordScalar("cfg_slotTime",  slotTime.dbl());

    // Bộ đếm thô
    recordScalar("totalSlots",        totalSlots);
    recordScalar("idleSlots",         idleSlots);
    recordScalar("successSlots",      successSlots);
    recordScalar("collisionSlots",    collisionSlots);
    recordScalar("totalAttempts",     totalAttempts);
    recordScalar("successfulPackets", successfulPackets);
    recordScalar("collidedPackets",   collidedPackets);

    // Chỉ số hiệu năng (dùng để vẽ đồ thị)
    recordScalar("offeredLoad_G",     G);
    recordScalar("throughput_S",      S);
    recordScalar("collisionRate",     CR);
    recordScalar("idleRate",          IR);
    recordScalar("successRatio",      SR);
    recordScalar("throughput_theory", S_theory);


    // ---- In tóm tắt ra console ----
    EV << "\n========== Slotted ALOHA – Kết Quả Mô Phỏng ==========\n";
    EV << "  slotTime      = " << slotTime       << " s\n";
    EV << "  totalSlots    = " << totalSlots      << "\n";
    EV << "  G (sim)       = " << G               << "\n";
    EV << "  S (sim)       = " << S               << "\n";
    EV << "  S (theory)    = " << S_theory        << "\n";
    EV << "  Collision Rate= " << CR              << "\n";
    EV << "  Idle Rate     = " << IR              << "\n";
    EV << "  Success Ratio = " << SR              << "\n";
    EV << "=======================================================\n";
}

// ---------------------------------------------------------------
Channel::~Channel()
{
    cancelAndDelete(endSlotEvent);
    for (auto pkt : packetsInCurrentSlot)
        delete pkt;
}

} // namespace slottedaloha
