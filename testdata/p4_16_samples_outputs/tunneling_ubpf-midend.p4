#include <core.p4>
#include <ubpf_model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_h {
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

header mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct Headers_t {
    Ethernet_h ethernet;
    mpls_h     mpls;
    IPv4_h     ipv4;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            16w0x8847: mpls;
            default: reject;
        }
    }
    state mpls {
        p.extract<mpls_h>(headers.mpls);
        transition ipv4;
    }
    state ipv4 {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("pipe.mpls_encap") action mpls_encap() {
        headers.mpls.setValid();
        headers.ethernet.etherType = 16w0x8847;
        headers.mpls.label = 20w20;
        headers.mpls.tc = 3w5;
        headers.mpls.stack = 1w1;
        headers.mpls.ttl = 8w64;
    }
    @name("pipe.mpls_decap") action mpls_decap() {
        headers.mpls.setInvalid();
        headers.ethernet.etherType = 16w0x800;
    }
    @name("pipe.upstream_tbl") table upstream_tbl_0 {
        key = {
            headers.mpls.label: exact @name("headers.mpls.label") ;
        }
        actions = {
            mpls_decap();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("pipe.downstream_tbl") table downstream_tbl_0 {
        key = {
            headers.ipv4.dstAddr: exact @name("headers.ipv4.dstAddr") ;
        }
        actions = {
            mpls_encap();
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        if (headers.mpls.isValid()) {
            upstream_tbl_0.apply();
        } else {
            downstream_tbl_0.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action tunneling_ubpf134() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<mpls_h>(headers.mpls);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_tunneling_ubpf134 {
        actions = {
            tunneling_ubpf134();
        }
        const default_action = tunneling_ubpf134();
    }
    apply {
        tbl_tunneling_ubpf134.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

