package slottedaloha;

simple Host
{
    parameters:
        double sendProbability;
        int packetLength @unit(bit);
        double slotTime @unit(s);
        @display("i=device/pc");
    gates:
        output out;
        input in;
}

simple Channel
{
    parameters:
        int numHosts;
        int packetLength @unit(bit);
        double bandwidth @unit(bps);
        double slotTime @unit(s);
        @display("i=block/switch");
    gates:
        input in[numHosts];
        output out[numHosts];
}

network SlottedAlohaNetwork
{
    parameters:
        int numHosts;
        int packetLength @unit(bit);
        double bandwidth @unit(bps);
        double slotTime @unit(s);

    submodules:
        host[numHosts]: Host {
            parameters:
                sendProbability = default(0.1);
                packetLength = parent.packetLength;
                slotTime = parent.slotTime;
        };

        medium: Channel {
            parameters:
                numHosts = parent.numHosts;
                packetLength = parent.packetLength;
                bandwidth = parent.bandwidth;
                slotTime = parent.slotTime;
        };

    connections allowunconnected:
        for i=0..numHosts-1 {
            host[i].out --> medium.in[i];
            medium.out[i] --> host[i].in;
        }
}