package slottedaloha;

// ===========================================================
//  Server  –  Bộ thu trung tâm trong mạng Slotted ALOHA
//
//  Thay thế Channel.cc cũ. Nhận gói qua cổng @directIn (sendDirect).
//  Sử dụng phát hiện va chạm theo slot (slot-based collision detection):
//    0 gói/slot  → Idle
//    1 gói/slot  → Success
//   ≥2 gói/slot  → Collision
//
//  Tính toán và xuất các chỉ số hiệu năng qua recordScalar() khi kết thúc.
// ===========================================================
simple Server
{
    parameters:
        @display("i=device/antennatower_l");

        // --- Tín hiệu thống kê (cho Qtenv / Analysis Tool) ---
        @signal[channelState](type="long");
        @signal[receiveBegin](type="long");
        @signal[receive](type="long");
        @signal[collision](type="long");
        @signal[collisionLength](type="simtime_t");

        @statistic[serverChannelState](
            source="channelState";
            title="Channel state";
            enum="IDLE=0,TRANSMISSION=1,COLLISION=2";
            record=vector?);
        @statistic[receiveBegin](
            source="receiveBegin";
            record=vector?;
            interpolationmode=sample-hold;
            title="receive begin");
        @statistic[channelUtilization](
            source="timeavg(receive)";
            record=last;
            interpolationmode=linear;
            title="channel utilization");
        @statistic[collisionMultiplicity](
            source=collision;
            record=vector?,histogram;
            title="collision multiplicity");
        @statistic[collisionLength](
            record=vector?,histogram,mean,sum,max;
            title="collision length");
        @statistic[receivedFrames](
            source="sum(receive)";
            record=last;
            title="received frames");
        @statistic[collidedFrames](
            source="sum(collision)";
            record=last;
            title="collided frames");

        // --- Tham số hệ thống (nhận từ Network để tính slotTime = L/R) ---
        int numHosts;                           // số trạm (dùng để ghi cfg_ scalar)
        int pkLenBits @unit(bit);               // kích thước gói (bit)
        double txRate @unit(bps);               // tốc độ kênh (bps)

        // --- Tọa độ không gian (đặt Server ở trung tâm bản đồ) ---
        double x @unit(m) = default(500m);
        double y @unit(m) = default(500m);

        // --- Tham số hoạt ảnh ---
        double animationHoldTimeOnCollision @unit(s) = default(0s);

    gates:
        input in @directIn;                     // nhận gói trực tiếp từ các Host qua sendDirect()
}
