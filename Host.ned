package slottedaloha;

// ===========================================================
//  Host  –  Trạm phát gói trong mạng Slotted ALOHA
//
//  Mô hình lưu lượng: Poisson arrivals với tốc độ λ = 1/iaTime.
//  Gói phát sinh tại thời điểm bất kỳ nhưng được gửi vào đầu
//  slot tiếp theo (slot-boundary alignment) theo chuẩn Slotted ALOHA.
//
//  Liên lạc với Server qua sendDirect() (không cần gate kết nối tường minh).
//  Độ trễ truyền sóng: radioDelay = dist(Host, Server) / c  (speed of light)
// ===========================================================
simple Host
{
    parameters:
        // --- Tín hiệu thống kê (cho Qtenv / Analysis Tool) ---
        @signal[state](type="long");
        @statistic[radioState](
            source="state";
            title="Radio state";
            enum="IDLE=0,TRANSMIT=1";
            record=vector?);

        // --- Tham số lưu lượng ---
        double txRate @unit(bps);               // tốc độ kênh truyền
        volatile int pkLenBits @unit(bit);      // kích thước gói (bit)
        double iaTime @unit(s);                 // inter-arrival time trung bình (hằng số mean, Poisson sampling trong C++)

        // --- Tọa độ không gian ---
        double x @unit(m);                      // tọa độ X trên mặt phẳng (m)
        double y @unit(m);                      // tọa độ Y trên mặt phẳng (m)
        double z @unit(m) = default(0m);        // tọa độ Z (độ cao, metadata – không dùng cho canvas 2D)

        // --- Tham số hoạt ảnh (Qtenv) ---
        double idleAnimationSpeed = default(1);
        double transmissionEdgeAnimationSpeed = default(1e-6);
        double midTransmissionAnimationSpeed = default(1e-1);
        bool controlAnimationSpeed = default(true);

        @display("i=device/laptop");
}
