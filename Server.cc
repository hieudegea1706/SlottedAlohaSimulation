/*
 * Server.cc  –  Slotted ALOHA: Bộ thu trung tâm (OOP Implementation)
 *
 * Thay thế Channel.cc trong kiến trúc cũ.
 *
 * Nguyên lý hoạt động:
 *   - Nhận gói từ các Host qua sendDirect() → gate "in" (@directIn)
 *   - gate("in")->setDeliverImmediately(true) đảm bảo nhận ngay khi bit đầu đến
 *   - Tích lũy gói vào packetsInCurrentSlot trong mỗi slot
 *   - Tại cuối slot (endSlotEvent): phân loại Idle/Success/Collision
 *   - refreshDisplay(): đổi màu icon theo trạng thái kênh tức thời
 *
 * Chỉ số hiệu năng xuất ra qua recordScalar() khi finish():
 *   G  = offeredLoad_G    (offered load thực đo)
 *   S  = throughput_S     (throughput)
 *   CR = collisionRate    (tỉ lệ slot va chạm)
 *   IR = idleRate         (tỉ lệ slot rảnh)
 *   SR = successRatio     (tỉ lệ gói thành công / tổng gói gửi)
 *
 * Lý thuyết Slotted ALOHA: S = G · e^(-G),  S_max = 1/e ≈ 0.368 tại G=1
 */

#include "Server.h"

namespace slottedaloha {

Define_Module(Server);

// ---------------------------------------------------------------
Server::~Server()
{
    cancelAndDelete(endSlotEvent);
    for (auto pkt : packetsInCurrentSlot)
        delete pkt;
}

// ---------------------------------------------------------------
void Server::initialize()
{
    // --- Đăng ký tín hiệu thống kê ---
    channelStateSignal   = registerSignal("channelState");
    receiveBeginSignal   = registerSignal("receiveBegin");
    receiveSignal        = registerSignal("receive");
    collisionSignal      = registerSignal("collision");
    collisionLengthSignal= registerSignal("collisionLength");

    // Phát tín hiệu khởi tạo
    emit(receiveSignal,    0L);
    emit(receiveBeginSignal, 0L);

    // --- Đọc tham số ---
    numHosts     = par("numHosts");
    pkLenBits_val= par("pkLenBits");
    txRate       = par("txRate").doubleValue();

    // slotTime = T = L/R (tính nội bộ, đồng nhất với Host.cc)
    slotTime = (double)pkLenBits_val / txRate;

    EV << "[Server] slotTime = " << slotTime << " s  (= "
       << pkLenBits_val << " bit / " << txRate << " bps)\n";

    // --- Đặt cổng @directIn giao nhận tức thì (bit đầu tiên đến) ---
    gate("in")->setDeliverImmediately(true);

    // --- Khởi tạo trạng thái ---
    channelState             = IDLE_STATE;
    currentCollisionNumFrames= 0;
    receiveCounter           = 0;
    emit(channelStateSignal, (long)IDLE_STATE);

    // --- Đặt vị trí Server trên Qtenv canvas ---
    getDisplayString().setTagArg("p", 0, par("x").doubleValue());
    getDisplayString().setTagArg("p", 1, par("y").doubleValue());

    // --- Lên lịch sự kiện kết thúc slot đầu tiên ---
    endSlotEvent = new cMessage("endSlotEvent");
    scheduleAt(simTime() + slotTime, endSlotEvent);
}

// ---------------------------------------------------------------
void Server::handleMessage(cMessage *msg)
{
    if (msg == endSlotEvent) {
        // ================================================================
        //  Kết thúc slot: phân loại và cập nhật bộ đếm
        // ================================================================
        totalSlots++;
        int n = (int)packetsInCurrentSlot.size();
        totalAttempts += n;
        emit(receiveBeginSignal, receiveCounter);

        if (n == 0) {
            // --- Idle slot ---
            idleSlots++;

            // Phát tín hiệu kết thúc nếu trước đó có reception (không áp dụng ở đây)
            channelState = IDLE_STATE;
            emit(channelStateSignal, (long)IDLE_STATE);
        }
        else if (n == 1) {
            // --- Success slot ---
            successSlots++;
            successfulPackets++;

            // Phát tín hiệu receive: bắt đầu tại recvStartTime, kết thúc bây giờ
            cTimestampedValue rcvStart(recvStartTime, (intval_t)1);
            emit(receiveSignal, &rcvStart);
            emit(receiveSignal, 0L);

            delete packetsInCurrentSlot[0];

            channelState = IDLE_STATE;
            emit(channelStateSignal, (long)IDLE_STATE);
        }
        else {
            // --- Collision slot (n >= 2) ---
            collisionSlots++;
            collidedPackets += n;

            // Phát tín hiệu va chạm
            simtime_t collisionDuration = simTime() - recvStartTime;
            cTimestampedValue colStart(recvStartTime, (intval_t)currentCollisionNumFrames);
            emit(collisionSignal, &colStart);
            emit(collisionLengthSignal, collisionDuration);

            for (auto pkt : packetsInCurrentSlot)
                delete pkt;

            channelState = IDLE_STATE;
            emit(channelStateSignal, (long)IDLE_STATE);
        }

        packetsInCurrentSlot.clear();
        receiveCounter         = 0;
        currentCollisionNumFrames = 0;

        // Lên lịch slot tiếp theo
        scheduleAt(simTime() + slotTime, endSlotEvent);
    }
    else {
        // ================================================================
        //  Nhận gói từ Host (qua sendDirect / @directIn gate)
        //  setDeliverImmediately(true) → đây là bit ĐẦU TIÊN của gói
        // ================================================================
        cPacket *pkt = check_and_cast<cPacket *>(msg);

        emit(receiveBeginSignal, ++receiveCounter);

        if (packetsInCurrentSlot.empty()) {
            // Gói đầu tiên trong slot → bắt đầu reception
            recvStartTime = simTime();
            channelState  = TRANSMISSION;
            emit(channelStateSignal, (long)TRANSMISSION);
            EV << "[Server] Slot " << totalSlots + 1 << ": started receiving "
               << pkt->getName() << "\n";
        }
        else {
            // Gói thứ 2 trở đi → COLLISION
            if (currentCollisionNumFrames == 0)
                currentCollisionNumFrames = 2;
            else
                currentCollisionNumFrames++;

            channelState = COLLISION;
            emit(channelStateSignal, (long)COLLISION);

            EV << "[Server] Slot " << totalSlots + 1 << ": COLLISION! ("
               << currentCollisionNumFrames << " frames)\n";

            // Hiển thị bubble animation khi có va chạm
            if (hasGUI()) {
                char buf[64];
                snprintf(buf, sizeof(buf), "Collision! (%lld frames)", (long long)currentCollisionNumFrames);
                bubble(buf);
                getParentModule()->getCanvas()->holdSimulationFor(
                    par("animationHoldTimeOnCollision").doubleValue());
            }
        }

        packetsInCurrentSlot.push_back(pkt);
    }
}

// ---------------------------------------------------------------
void Server::refreshDisplay() const
{
    // --- Màu sắc icon theo trạng thái kênh hiện tại ---
    switch (channelState) {
        case IDLE_STATE:
            getDisplayString().setTagArg("i2", 0, "status/off");    // icon xám
            getDisplayString().setTagArg("t",  0, "");
            break;
        case TRANSMISSION:
            getDisplayString().setTagArg("i2", 0, "status/yellow");  // icon vàng
            getDisplayString().setTagArg("t",  0, "RECV");
            getDisplayString().setTagArg("t",  2, "#808000");
            break;
        case COLLISION:
            getDisplayString().setTagArg("i2", 0, "status/red");     // icon đỏ
            getDisplayString().setTagArg("t",  0, "COLLISION");
            getDisplayString().setTagArg("t",  2, "#800000");
            break;
    }
}

// ---------------------------------------------------------------
void Server::finish()
{
    if (totalSlots == 0) {
        EV << "[Server] Không có slot nào được ghi nhận!\n";
        return;
    }

    // ---- Tính các chỉ số hiệu năng ----
    double G       = (double)totalAttempts    / totalSlots;   // Offered Load
    double S       = (double)successSlots     / totalSlots;   // Throughput
    double CR      = (double)collisionSlots   / totalSlots;   // Collision Rate
    double IR      = (double)idleSlots        / totalSlots;   // Idle Rate
    double SR      = (totalAttempts > 0)
                     ? (double)successfulPackets / totalAttempts
                     : 0.0;                                   // Success Ratio
    double S_theory= G * std::exp(-G);                        // Lý thuyết: S = G·e^(-G)
    double error_pct = (S_theory > 0)
                     ? std::abs(S - S_theory) / S_theory * 100.0
                     : 0.0;

    // ---- Ghi scalar (giữ toàn bộ scalar cũ để Python script tương thích) ----

    // Tham số cấu hình
    recordScalar("cfg_numHosts",  numHosts);
    recordScalar("cfg_pkLenBits", pkLenBits_val);
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
    EV << "\n========== Slotted ALOHA – Kết Quả Mô Phỏng ==========\n"
       << "  slotTime       = " << slotTime       << " s\n"
       << "  totalSlots     = " << totalSlots      << "\n"
       << "  totalAttempts  = " << totalAttempts   << "\n"
       << "  idleSlots      = " << idleSlots       << "  (IR = " << IR  << ")\n"
       << "  successSlots   = " << successSlots    << "  (S  = " << S   << ")\n"
       << "  collisionSlots = " << collisionSlots  << "  (CR = " << CR  << ")\n"
       << "  S + CR + IR    = " << (S + CR + IR)   << "  (phải = 1.0)\n"
       << "  G (sim)        = " << G               << "\n"
       << "  S (sim)        = " << S               << "\n"
       << "  S (theory)     = " << S_theory        << "\n"
       << "  Error          = " << error_pct       << " %\n"
       << "=======================================================\n";
}

}; // namespace slottedaloha
