#include <core.p4>
#include <dpdk/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<8>  x0;
    bit<16> x1_x2_x3;
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct user_meta_data_t {
    bit<128> k1;
    bit<128> k2;
    bit<48>  addr;
    bit<8>   x2;
}

parser MyIngressParser(packet_in pkt, out headers_t hdr, inout user_meta_data_t m, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(inout headers_t hdr, inout user_meta_data_t m, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name("MyIngressControl.MyIngressControl.flg") bit<8> flg;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngressControl.MyIngressControl.macswp") action macswp() {
        m.k1 = (flg == 8w0x2 ? m.k2 : m.k1);
        m.x2 = (flg == 8w0x2 ? hdr.ethernet.x0 : m.x2);
        hdr.ethernet.x0 = (flg == 8w0x2 ? m.x2 : hdr.ethernet.x0);
        m.x2 = (flg == 8w0x2 ? (bit<8>)(hdr.ethernet.ether_type >> 2) : m.x2);
        m.addr = (flg == 8w0x2 ? hdr.ethernet.dst_addr : m.addr);
        hdr.ethernet.dst_addr = (flg == 8w0x2 ? hdr.ethernet.src_addr : hdr.ethernet.dst_addr);
        hdr.ethernet.src_addr = (flg == 8w0x2 ? m.addr : hdr.ethernet.src_addr);
    }
    @name("MyIngressControl.MyIngressControl.stub") table stub {
        key = {
            m.k1: exact @name("m.k1") ;
        }
        actions = {
            macswp();
            @defaultonly NoAction_1();
        }
        size = 1000000;
        default_action = NoAction_1();
    }
    @hidden action psadpdklargestructfields34() {
        flg = 8w0;
    }
    @hidden table tbl_psadpdklargestructfields34 {
        actions = {
            psadpdklargestructfields34();
        }
        const default_action = psadpdklargestructfields34();
    }
    apply {
        tbl_psadpdklargestructfields34.apply();
        stub.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psadpdklargestructfields64() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psadpdklargestructfields64 {
        actions = {
            psadpdklargestructfields64();
        }
        const default_action = psadpdklargestructfields64();
    }
    apply {
        tbl_psadpdklargestructfields64.apply();
    }
}

parser MyEgressParser(packet_in pkt, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyEgressControl(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyEgressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIngressParser(), MyIngressControl(), MyIngressDeparser()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEgressParser(), MyEgressControl(), MyEgressDeparser()) ep;

PSA_Switch<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

