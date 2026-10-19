// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "admission.hpp"
#include "exchange.hpp"
#include "peer_discovery.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <openssl/evp.h>

namespace meshcloud::p2p {
// Symmetric node: same credentials can DIAL or LISTEN; no central host.
// TLS 1.3 identity is Ed25519. Every allowed remote identity MUST be pinned
// before the first protocol byte is sent. After mutual TLS, the signed 10-GiB
// auditor ticket is bound to the TLS exporter + fresh nonce.
// FAIL CLOSED: no tickets, no pinned key, no quorum => NO GET/PUT.
struct NodeIdentity {
 EVP_PKEY* private_key=nullptr;  // caller retains ownership
 admission::MemberTicket ticket;
 admission::TrustPolicy auditors;
 std::vector<admission::PublicKey> allowed_peer_keys;
};
constexpr std::uint16_t default_port=39718;
constexpr std::size_t max_ticket_bytes=4096;
constexpr std::size_t max_frame_bytes=4*1024*1024+8192;
// Caller manages storage lifetime and only supplies already verified ciphertext.
// listen_once accepts one pinned and auditor-authorized peer request; never
// responds to unauthenticated peers. Binds to a supplied IPv4 address.
// LAN use: restrict bind address to the LAN interface, not 0.0.0.0.
bool listen_once(const std::string& bind_ipv4,std::uint16_t port,
                 const NodeIdentity& local,exchange::PeerGateway& gateway,
                 std::string& error,std::atomic<bool>* stop=nullptr,
                 discovery::AddressBook* address_book=nullptr);
bool dial_once(const std::string& remote_ipv4,std::uint16_t port,
               const NodeIdentity& local,
               const std::vector<std::uint8_t>& authenticated_wire_request,
               std::vector<std::uint8_t>& authenticated_wire_response,
               std::string& error);
// Send authenticated getaddr through the SAME TLS 1.3 + 13 GiB quorum gate.
// Explicit advertise_ipv4 opt-in; no automatic leakage of private addresses.
// Caller must first manually connect to a known member at least once.
bool discover_once(const std::string& remote_ipv4,std::uint16_t port,
                   const NodeIdentity& local,discovery::AddressBook& book,
                   const std::string& advertise_ipv4,std::string& error);
// Read-only session diagnostic. Does not expose CID search or file data.
// Returns true only after BOTH peer identities and independently signed
// auditor tickets have been validated; never has a guest fallback.
bool verify_tls_peer_credentials(EVP_PKEY* tls_cert_pubkey,
                 const admission::MemberTicket& ticket,
                 const std::vector<admission::PublicKey>& pinned);
} // namespace meshcloud::p2p
