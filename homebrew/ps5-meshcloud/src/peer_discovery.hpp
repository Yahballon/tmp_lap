// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "admission.hpp"
#include "wire.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <openssl/evp.h>
struct sqlite3;
namespace meshcloud::discovery {
// Inspired by Bitcoin addr/getaddr/addrman, NOT Bitcoin wire compatible.
// Scope is deliberately restricted to local private IPv4 in this version.
// Never gossip a private LAN address over a future Internet relay.
constexpr std::size_t record_bytes=122;
constexpr std::size_t max_gossip_records=16;
constexpr std::size_t max_known_peers=512;
constexpr std::uint64_t record_lifetime=7ULL*24*3600;
struct SignedAddress {
 admission::PublicKey identity{};
 std::array<std::uint8_t,4> ipv4{};
 std::uint16_t port=0;
 std::uint64_t published=0;
 std::uint64_t expires=0;
 admission::Signature signature{};
};
// Each node signs its OWN opt-in advertised endpoint. Such an address does
// NOT prove a ticket, storage capacity or connection reachability.
bool sign(SignedAddress& address,EVP_PKEY* owner_private);
bool verify(const SignedAddress& address,std::uint64_t now);
// Only non-public LAN addresses are allowed until a privacy-safe relay mode
// exists. No hostname lookup, private IP gossip to the global Internet,
// local-link multicast, wildcard or public IPv4.
bool parse_lan_ipv4(const std::string& text,std::array<std::uint8_t,4>& out);
std::string ipv4_text(const SignedAddress& address);
bool encode_record(const SignedAddress& address,std::vector<std::uint8_t>& out);
bool decode_record(const std::uint8_t* input,std::size_t length,SignedAddress& out);
bool encode_gossip(const std::vector<SignedAddress>& records,
                   std::vector<std::uint8_t>& frame);
bool decode_gossip(const std::vector<std::uint8_t>& frame,
                   std::vector<SignedAddress>& records);
// Disk-backed local equivalent of Bitcoin's peers.dat/addrman, bounded and
// crash-safe through SQLite. Records can only be imported from an already
// authenticated peer channel, or directly after locally signing ourselves.
class AddressBook {
public:
 explicit AddressBook(std::filesystem::path directory);
 ~AddressBook();
 AddressBook(const AddressBook&)=delete;
 AddressBook& operator=(const AddressBook&)=delete;
 bool open();
 bool add(const SignedAddress& record,std::uint64_t now);
 bool list(std::uint64_t now,std::vector<SignedAddress>& out,
           std::size_t limit=max_gossip_records);
 bool expire(std::uint64_t now);
 const std::string& error() const noexcept {return error_;}
private:
 std::filesystem::path directory_;
 sqlite3* db_=nullptr;
 std::string error_;
};
// Only used AFTER wire::authenticated_parse(Operation::Discovery...) succeeded.
// Request must contain zero or ONE self-signed address, and any address must
// match the authenticated TLS Ed25519 identity. No guest discovery.
bool authorized_getaddr(const std::vector<std::uint8_t>& request,
    const admission::PublicKey& authenticated_tls_peer,AddressBook& book,
    std::uint64_t now,std::vector<std::uint8_t>& reply);
// Import an authenticated getaddr response from an audited TLS session.
bool authorized_addr_reply(const std::vector<std::uint8_t>& response,
    AddressBook& book,std::uint64_t now);
} // namespace meshcloud::discovery
