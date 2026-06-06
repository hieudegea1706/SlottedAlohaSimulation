/*
 * Host.cc  –  Slotted ALOHA: Trạm phát gói (OOP Implementation)
 *
 * Bám sát kiến trúc samples/aloha của OMNeT++ 6.4.0.
 *
 * Công thức tải:
 *   slotTime T  = pkLenBits / txRate          [s/slot]
 *   λ_host      = 1 / iaTime                  [gói/s]
 *   G_total     = numHosts × T / iaTime       [gói/slot]
 *
 * Kịch bản mẫu (N=20, L=960bit, R=9600bps → T=0.1s):
 *   LightLoad  : iaTime=20s  → G=0.1
 *   MediumLoad : iaTime=2s   → G=1.0  ← điểm tối ưu (S_max = 1/e ≈ 0.368)
 *   HighLoad   : iaTime=0.5s → G=4.0
 */

#include <algorithm>
#include <cmath>
#include "Host.h"

namespace slottedaloha {

Define_Module(Host);

// ---------------------------------------------------------------
Host::~Host()
{
    delete lastPacket;
    cancelAndDelete(endTxEvent);
}

// ---------------------------------------------------------------
void Host::initialize()
{
    // --- Đăng ký tín hiệu thống kê ---
    stateSignal = registerSignal("state");

    // --- Tìm module Server (anh em trong cùng network) ---
    server = getModuleByPath("^.server");
    if (!server)
        throw cRuntimeError("Host [%d]: Không tìm thấy module 'server'! "
                            "Kiểm tra tên submodule trong SlottedAloha.ned.", getIndex());

    // --- Đọc tham số ---
    txRate      = par("txRate").doubleValue();          // bps
    pkLenBits   = &par("pkLenBits");                   // cPar* (volatile int)
    iaTime_mean = par("iaTime").doubleValue();          // s (hằng số mean của Poisson)

    // slotTime tính từ pkLenBits/txRate (KHÔNG đọc từ NED để tránh lỗi cấu hình)
    slotTime  = (double)pkLenBits->intValue() / txRate; // = 960/9600 = 0.1 s
    isSlotted = slotTime > 0;

    // --- Tọa độ không gian ---
    x = par("x").doubleValue();
    y = par("y").doubleValue();
    z = par("z").doubleValue();

    double serverX = server->par("x").doubleValue();
    double serverY = server->par("y").doubleValue();

    // Tính độ trễ lan truyền sóng vô tuyến (tốc độ ánh sáng)
    double dist = std::sqrt((x - serverX)*(x - serverX) + (y - serverY)*(y - serverY));
    radioDelay  = dist / propagationSpeed;

    // --- Tham số hoạt ảnh Qtenv ---
    idleAnimationSpeed              = par("idleAnimationSpeed").doubleValue();
    transmissionEdgeAnimationSpeed  = par("transmissionEdgeAnimationSpeed").doubleValue();
    midtransmissionAnimationSpeed   = par("midTransmissionAnimationSpeed").doubleValue();

    // --- Đặt vị trí icon trên Qtenv canvas (x, y theo tọa độ m) ---
    getDisplayString().setTagArg("p", 0, x);
    getDisplayString().setTagArg("p", 1, y);

    // --- Khởi tạo trạng thái ---
    state     = IDLE;
    pkCounter = 0;
    emit(stateSignal, (long)state);
    WATCH((int&)state);
    WATCH(pkCounter);

    endTxEvent = new cMessage("send/endTx");
    scheduleAt(getNextTransmissionTime(), endTxEvent);

    EV << "[Host " << getIndex() << "] initialized: "
       << "pos=(" << x << ", " << y << ", " << z << ") m, "
       << "dist_to_server=" << dist << " m, "
       << "radioDelay=" << radioDelay << " s, "
       << "slotTime=" << slotTime << " s, "
       << "iaTime_mean=" << iaTime_mean << " s\n";
}

// ---------------------------------------------------------------
void Host::handleMessage(cMessage *msg)
{
    ASSERT(msg == endTxEvent);

    if (hasGUI())
        getParentModule()->getCanvas()->setAnimationSpeed(transmissionEdgeAnimationSpeed, this);

    if (state == IDLE) {
        // ====== Phát sinh gói mới và gửi đến Server ======

        char pkname[48];
        snprintf(pkname, sizeof(pkname), "pk-h%d-#%d", getIndex(), pkCounter++);
        EV << "[Host " << getIndex() << "] generating packet " << pkname << "\n";

        state = TRANSMIT;
        emit(stateSignal, (long)state);

        // Tạo gói tin
        cPacket *pk = new cPacket(pkname);
        pk->setBitLength(pkLenBits->intValue());
        simtime_t duration = (double)pk->getBitLength() / txRate;

        // Gửi trực tiếp tới Server qua @directIn gate
        // radioDelay = độ trễ lan truyền; duration = thời gian chiếm kênh
        sendDirect(pk, radioDelay, duration, server->gate("in"));

        // Lên lịch kết thúc truyền gói
        scheduleAt(simTime() + duration, endTxEvent);

        // Cập nhật bản sao gói cho hoạt ảnh (phải sau sendDirect vì pk vẫn truy cập được)
        if (transmissionRing != nullptr) {
            delete lastPacket;
            transmissionRing->setVisible(false);
            transmissionRing->setAssociatedObject(nullptr);
            for (auto c : transmissionCircles) {
                c->setVisible(false);
                c->setAssociatedObject(nullptr);
            }
        }
        lastPacket = pk->dup();
    }
    else if (state == TRANSMIT) {
        // ====== Kết thúc truyền → chờ gói tiếp theo ======
        EV << "[Host " << getIndex() << "] end of transmission\n";
        state = IDLE;
        emit(stateSignal, (long)state);
        scheduleAt(getNextTransmissionTime(), endTxEvent);
    }
    else {
        throw cRuntimeError("Host [%d]: invalid state %d", getIndex(), (int)state);
    }
}

// ---------------------------------------------------------------
simtime_t Host::getNextTransmissionTime()
{
    // Lấy mẫu ngẫu nhiên từ phân phối Poisson (exponential inter-arrival)
    // iaTime_mean là hằng số (không volatile), exponential() sampling thực hiện tại đây
    simtime_t t = simTime() + exponential(iaTime_mean);

    if (!isSlotted)
        return t;   // Pure Aloha: gửi ngay

    // Slotted Aloha: căn lề về đầu slot tiếp theo
    // Sử dụng ceil(t / slotTime) * slotTime  (công thức chuẩn từ samples/aloha)
    return slotTime * std::ceil(t.dbl() / slotTime.dbl());
}

// ---------------------------------------------------------------
void Host::finish()
{
    recordScalar("generatedPackets", pkCounter);
}

// ---------------------------------------------------------------
void Host::refreshDisplay() const
{
    cCanvas *canvas = getParentModule()->getCanvas();
    const int    numCircles      = 20;
    const double circleLineWidth = 10;

    // Tạo các đối tượng đồ họa lần đầu (lazy initialization)
    if (!transmissionRing) {
        auto color = cFigure::GOOD_DARK_COLORS[getId() % cFigure::NUM_GOOD_DARK_COLORS];

        // --- Vòng sóng chính (ring) ---
        transmissionRing = new cRingFigure(
            ("Host" + std::to_string(getIndex()) + "Ring").c_str());
        transmissionRing->setOutlined(false);
        transmissionRing->setFillColor(color);
        transmissionRing->setFillOpacity(0.25);
        transmissionRing->setFilled(true);
        transmissionRing->setVisible(false);
        transmissionRing->setZIndex(-1);
        canvas->addFigure(transmissionRing);

        // --- Các vòng gợn sóng (ripples) ---
        for (int i = 0; i < numCircles; ++i) {
            auto circle = new cOvalFigure(
                ("Host" + std::to_string(getIndex()) + "Circle" + std::to_string(i)).c_str());
            circle->setFilled(false);
            circle->setLineColor(color);
            circle->setLineOpacity(0.75);
            circle->setLineWidth(circleLineWidth);
            circle->setZoomLineWidth(true);
            circle->setVisible(false);
            circle->setZIndex(-0.5);
            transmissionCircles.push_back(circle);
            canvas->addFigure(circle);
        }
    }

    // --- Cập nhật hình ảnh vòng sóng ---
    if (lastPacket) {
        // Liên kết đối tượng packet với figure để theo dõi
        if (transmissionRing->getAssociatedObject() != lastPacket) {
            transmissionRing->setAssociatedObject(lastPacket);
            for (auto c : transmissionCircles)
                c->setAssociatedObject(lastPacket);
        }

        simtime_t now           = simTime();
        simtime_t frontTravelTime = now - lastPacket->getSendingTime();
        simtime_t backTravelTime  = now - (lastPacket->getSendingTime() + lastPacket->getDuration());

        // Chuyển đổi thời gian → khoảng cách (m)
        double frontRadius = std::min(ringMaxRadius, frontTravelTime.dbl() * propagationSpeed);
        double backRadius  = backTravelTime.dbl() * propagationSpeed;
        double circleRadiusIncrement = circlesMaxRadius / numCircles;

        // --- Vòng sóng chính ---
        double opacity = 1.0;
        if (backRadius > ringMaxRadius) {
            // Sóng đã lan ra quá xa → ẩn
            transmissionRing->setVisible(false);
            transmissionRing->setAssociatedObject(nullptr);
            for (auto c : transmissionCircles) {
                c->setVisible(false);
                c->setAssociatedObject(nullptr);
            }
        }
        else {
            transmissionRing->setVisible(true);
            transmissionRing->setBounds(cFigure::Rectangle(
                x - frontRadius, y - frontRadius,
                2*frontRadius, 2*frontRadius));
            transmissionRing->setInnerRadius(
                std::max(0.0, std::min(ringMaxRadius, backRadius)));
            if (backRadius > 0)
                opacity = std::max(0.0, 1.0 - backRadius / circlesMaxRadius);
        }
        transmissionRing->setLineOpacity(opacity);
        transmissionRing->setFillOpacity(opacity / 5.0);

        // --- Vòng gợn sóng ---
        double radius0 = std::fmod(frontTravelTime.dbl() * propagationSpeed, circleRadiusIncrement);
        for (int i = 0; i < (int)transmissionCircles.size(); ++i) {
            double circleRadius = std::min(ringMaxRadius, radius0 + i * circleRadiusIncrement);
            if (circleRadius < frontRadius - circleRadiusIncrement/2.0
                && circleRadius > backRadius + circleLineWidth/2.0) {
                transmissionCircles[i]->setVisible(true);
                transmissionCircles[i]->setBounds(cFigure::Rectangle(
                    x - circleRadius, y - circleRadius,
                    2*circleRadius, 2*circleRadius));
                transmissionCircles[i]->setLineOpacity(
                    std::max(0.0, 0.2 - 0.2 * (circleRadius / circlesMaxRadius)));
            }
            else {
                transmissionCircles[i]->setVisible(false);
            }
        }

        // --- Tốc độ hoạt ảnh thích ứng ---
        double animSpeed = idleAnimationSpeed;
        if ((frontRadius >= 0 && frontRadius < circlesMaxRadius)
            || (backRadius >= 0 && backRadius < circlesMaxRadius))
            animSpeed = transmissionEdgeAnimationSpeed;
        if (frontRadius > circlesMaxRadius && backRadius < 0)
            animSpeed = midtransmissionAnimationSpeed;
        if (hasGUI())
            canvas->setAnimationSpeed(animSpeed, this);
    }
    else {
        // Không có gói đang truyền → ẩn tất cả hình ảnh
        if (transmissionRing->getAssociatedObject() != nullptr) {
            transmissionRing->setVisible(false);
            transmissionRing->setAssociatedObject(nullptr);
            for (auto c : transmissionCircles) {
                c->setVisible(false);
                c->setAssociatedObject(nullptr);
            }
            if (hasGUI())
                canvas->setAnimationSpeed(idleAnimationSpeed, this);
        }
    }

    // --- Màu sắc và nhãn trạng thái Host ---
    getDisplayString().setTagArg("t", 2, "#808000"); // màu chữ
    if (state == IDLE) {
        getDisplayString().setTagArg("i", 1, "");    // icon bình thường
        getDisplayString().setTagArg("t", 0, "");    // không nhãn
    }
    else { // TRANSMIT
        getDisplayString().setTagArg("i", 1, "yellow"); // icon vàng
        getDisplayString().setTagArg("t", 0, "TX");     // nhãn "TX"
    }
}

}; // namespace slottedaloha
