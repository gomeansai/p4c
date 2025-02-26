#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct my_headers_t {
    ethernet_t ethernet;
}

struct local_metadata_t {
    bit<16> f16;
    bit<16> m16;
    bit<16> d16;
    int<16> x16;
    int<16> a16;
    int<16> b16;
}

parser parser_impl(packet_in packet, out my_headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress_impl(inout my_headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control egress_impl(inout my_headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control verify_checksum_impl(inout my_headers_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum_impl(inout my_headers_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control deparser_impl(packet_out packet, in my_headers_t hdr) {
    apply {
    }
}

V1Switch<my_headers_t, local_metadata_t>(parser_impl(), verify_checksum_impl(), ingress_impl(), egress_impl(), compute_checksum_impl(), deparser_impl()) main;

