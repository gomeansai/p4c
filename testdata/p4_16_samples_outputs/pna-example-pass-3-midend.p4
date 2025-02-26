#include <core.p4>
#include <pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

struct empty_metadata_t {
}

struct main_metadata_t {
    bit<16> port;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    udp_t      udp;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    @hidden action pnaexamplepass3l75() {
        meta.port = hdr.udp.src_port;
        recirculate();
    }
    @hidden table tbl_pnaexamplepass3l75 {
        actions = {
            pnaexamplepass3l75();
        }
        const default_action = pnaexamplepass3l75();
    }
    apply {
        if (istd.pass != 3w1) {
            tbl_pnaexamplepass3l75.apply();
        }
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        pkt.extract<udp_t>(hdr.udp);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnaexamplepass3l112() {
        hdr.udp.src_port = hdr.udp.src_port + 16w1;
        recirculate();
    }
    @hidden table tbl_pnaexamplepass3l112 {
        actions = {
            pnaexamplepass3l112();
        }
        const default_action = pnaexamplepass3l112();
    }
    apply {
        if ((bit<8>)(bit<3>)istd.pass <= 8w0x4) {
            tbl_pnaexamplepass3l112.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexamplepass3l125() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
        pkt.emit<udp_t>(hdr.udp);
    }
    @hidden table tbl_pnaexamplepass3l125 {
        actions = {
            pnaexamplepass3l125();
        }
        const default_action = pnaexamplepass3l125();
    }
    apply {
        tbl_pnaexamplepass3l125.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

