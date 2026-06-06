package slottedaloha;

// ===========================================================
//  SlottedAloha  –  Mạng mô phỏng Slotted ALOHA (12 kịch bản)
//
//  Kiến trúc:
//    - server: Server  (bộ thu trung tâm, đặt tại tọa độ 500, 500)
//    - host[numHosts]: Host  (các trạm phân bố ngẫu nhiên trên 1000×1000 m)
//    - Kết nối: Host dùng sendDirect() → không cần kết nối tường minh
//
//  Nền địa hình: background/terrain (từ thư viện icon OMNeT++)
// ===========================================================
network SlottedAloha
{
    parameters:
        int    numHosts  = default(20);
        int    pkLenBits @unit(bit) = default(960bit);
        double txRate    @unit(bps) = default(9600bps);

        @display("bgi=background/terrain,s;bgb=1000,1000");

    submodules:
        server: Server {
            numHosts  = parent.numHosts;
            pkLenBits = parent.pkLenBits;
            txRate    = parent.txRate;
            @display("p=500,500");
        }

        host[numHosts]: Host {
            txRate    = parent.txRate;
            pkLenBits = parent.pkLenBits;
            // x, y được đặt trong omnetpp.ini: uniform(0m, 1000m)
            // Vị trí trên canvas được cập nhật bởi Host::initialize()
        }

    connections allowunconnected:
        // Host sử dụng sendDirect() để gửi gói trực tiếp tới server.gate("in")
        // Không cần kết nối tường minh giữa Host và Server.
}
