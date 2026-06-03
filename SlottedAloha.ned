package slottedaloha;

// ============================================================
//  Host: phát sinh gói theo tiến trình Poisson
//        iaTime = exponential(mean) → G = numHosts * T / mean
// ============================================================
simple Host
{
    parameters:
        double iaTime @unit(s);        // inter-arrival time trung bình (Poisson)
        int pkLenBits @unit(bit);   // kích thước gói = 960 bit
        double txRate @unit(bps);   // tốc độ kênh   = 9600 bps
        @display("i=device/pc");
    gates:
        output out;
        input in;
}

// ============================================================
//  Channel: kênh dùng chung, phân slot, phát hiện va chạm
// ============================================================
simple Channel
{
    parameters:
        int numHosts;
        int pkLenBits @unit(bit);
        double txRate @unit(bps);
        @display("i=block/switch");
    gates:
        input in[numHosts];
        output out[numHosts];
}

// ============================================================
//  Network
// ============================================================
network SlottedAlohaNetwork
{
    parameters:
        int numHosts = default(20);
        int pkLenBits @unit(bit) = default(960bit);
        double txRate @unit(bps) = default(9600bps);

        @display("bgb=347,234");
    submodules:
        host[numHosts]: Host {
            parameters:
                pkLenBits = parent.pkLenBits;
                txRate = parent.txRate;
                // iaTime được đặt trong omnetpp.ini cho từng kịch bản
                @display("p=88,100");
        }

        medium: Channel {
            parameters:
                numHosts = parent.numHosts;
                pkLenBits = parent.pkLenBits;
                txRate = parent.txRate;
                @display("p=220,100");
        }

    connections allowunconnected:
        for i=0..numHosts-1 {
            host[i].out --> medium.in[i];
            medium.out[i] --> host[i].in;
        }
}
